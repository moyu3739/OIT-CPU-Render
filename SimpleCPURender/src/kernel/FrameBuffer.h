#pragma once

#include <vector>
#include <list>
#include <atomic>
// #include <cstdlib>
#include <glm/glm.hpp>
#include "utility.h"
#include "Primitive.h"
#include "PixelBuffer.h"
#include "PerPixelListBuffer.h"
#include "ListAllocator.h"


struct Format {
    enum Order {
        TOP_DOWN,
        BOTTOM_UP
    };
    enum Channel {
        RGB,
        RGBA,
        ARGB,
        BGR,
        BGRA,
        ABGR
    };
    enum Type {
        UINT8,
        UINT16,
        FLOAT32,
        FLOAT64
    };

    static int GetChannelSize(Type type) {
        switch(type) {
            case UINT8: return 1;
            case UINT16: return 2;
            case FLOAT32: return 4;
            case FLOAT64: return 8;
        }
    }

    static int GetChannelNumber(Channel channel) {
        switch(channel) {
            case RGB:
            case BGR: return 3;
            case RGBA:
            case ARGB:
            case BGRA:
            case ABGR: return 4;
        }
    }

    static void WriteOneData(float x, void*& ptr, Type type) {
        switch(type) {
            case UINT8: {
                unsigned char* p = reinterpret_cast<unsigned char*>(ptr);
                *p = static_cast<unsigned char>(x * 255);
                ptr = reinterpret_cast<void*>(p + 1);
                break;
            }
            case UINT16: {
                unsigned short* p = reinterpret_cast<unsigned short*>(ptr);
                *p = static_cast<unsigned short>(x * 65535);
                ptr = reinterpret_cast<void*>(p + 1);
                break;
            }
            case FLOAT32: {
                float* p = reinterpret_cast<float*>(ptr);
                *p = x;
                ptr = reinterpret_cast<void*>(p + 1);
                break;
            }
            case FLOAT64: {
                double* p = reinterpret_cast<double*>(ptr);
                *p = static_cast<double>(x);
                ptr = reinterpret_cast<void*>(p + 1);
                break;
            }
        }
    }

    static void WriteOnePixel(const glm::vec3& color, void*& ptr, Channel channel, Type type) {
        switch(channel) {
            case RGB:
                WriteOneData(color.r, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.b, ptr, type);
                break;
            case RGBA:
                WriteOneData(color.r, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.b, ptr, type);
                WriteOneData(1.0f, ptr, type); // alpha channel
                break;
            case ARGB:
                WriteOneData(1.0f, ptr, type); // alpha channel
                WriteOneData(color.r, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.b, ptr, type);
                break;
            case BGR:
                WriteOneData(color.b, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.r, ptr, type);
                break;
            case BGRA:
                WriteOneData(color.b, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.r, ptr, type);
                WriteOneData(1.0f, ptr, type); // alpha channel
                break;
            case ABGR:
                WriteOneData(1.0f, ptr, type); // alpha channel
                WriteOneData(color.b, ptr, type);
                WriteOneData(color.g, ptr, type);
                WriteOneData(color.r, ptr, type);
                break;
        }
    }

    Order order;
    Channel channel;
    Type type;
};


class FrameBuffer {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] allocator_num  number of allocators for per-pixel linked list buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when backward blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    FrameBuffer(
        int width, int height, int allocator_num,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
        bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        width(width), height(height), enable_oit(enable_oit)
    {
        pixel_buffer = new PixelBuffer(width, height, bg_color, bg_depth);
        if (enable_oit) {
            if (use_backward_pplist)
                pplist_buffer = new BackwardPerPixelListBuffer(width, height, allocator_num, backward_blend_alpha_threshold);
            else
                pplist_buffer = new ForwardPerPixelListBuffer(width, height, allocator_num);
        }
        Clear();
    }

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    ~FrameBuffer() {
        delete pixel_buffer;
        if (enable_oit) delete pplist_buffer;
    }

    // clear the frame buffer
    void Clear() {
        pixel_buffer->Clear();

        if (pplist_buffer_touched) {
            pplist_buffer->Clear();
            pplist_buffer_touched = false;
        }
    }

    // insert a fragment with the given color and depth at (`x`, `y`).
    // depend on whether OIT is enabled and the alpha value of the color,
    // this function may cover the existing fragment or insert the fragment to the per-pixel linked list
    void InsertFragment_T(const glm::vec4& color, float depth, int x, int y, int thread_id) {
        if (!enable_oit || color.a > 0.9999f)
            CoverFragment_T(color, depth, x, y);
        else
            InsertFragmentToList_T(color, depth, x, y, thread_id);
    }

    // cover the fragment at (x, y) with the given color and depth
    void CoverFragment_T(const glm::vec4& color, float depth, int x, int y) {
        pixel_buffer->CoverAt_T(glm::vec3(color), depth, x, y);
    }

    // blend the per-pixel linked list to the color buffer
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void Blend(int thread_num = 1) {
        BlendSlice(thread_num);
        // BlendCounter(thread_num);
    }

    // blend the per-pixel linked list to the color buffer,
    // using paralleling method of trivial slicing up list buffers equally
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void BlendSlice(int thread_num = 1) {
        if (!pplist_buffer_touched) return;

        if (thread_num == 1) {
            BlendSliceProcess(0, width * height);
        }
        else {
            std::vector<int> split_points = ut::RangeSlice(0, width * height, thread_num);
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_num; i++) {
                int pplist_begin = split_points[i];
                int pplist_end = split_points[i + 1];
                threads.emplace_back(&FrameBuffer::BlendSliceProcess, this, pplist_begin, pplist_end);
            }
            for (std::thread& thread : threads) thread.join();
        }
    }

    // blend the per-pixel linked list to the color buffer,
    // using paralleling method of atomic counter, fetching list by threads
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void BlendCounter(int thread_num = 1) {
        if (!pplist_buffer_touched) return;

        if (thread_num == 1) {
            BlendSliceProcess(0, width * height);
        }
        else {
            std::atomic<int> counter;
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_num; i++) {
                threads.emplace_back(&FrameBuffer::BlendCounterProcess, this, &counter);
            }
            for (std::thread& thread : threads) thread.join();
        }
    }

    // do depth test at (x, y) with the given depth
    // @note  `x` from left to right, `y` from BOTTOM to top
    bool DepthTestAt(float depth, int x, int y) {
        return depth < GetDepthAt(x, y);
    }

    // get color with buffer memory order
    // @note  `x` from left to right, `y` from BOTTOM to top
    const glm::vec3& GetColorAt(int x, int y) const {
        return pixel_buffer->ColorAt(x, y);
    }

    // get color with buffer memory order
    // @note  `x` from left to right, `y` from BOTTOM to top
    float GetDepthAt(int x, int y) const {
        return pixel_buffer->DepthAt(x, y);
    }

    // get the color buffer directly (this function takes almost no time)
    // @note  the format of the color buffer is as below:
    // @note    - 'RGBA' 4 channels, 32 bits floating number per channel, 128 bits per pixel
    // @note    - 'A' channel can be any value, which should be IGNORED
    // @note    - buffer order is firstly from left to right, and then from BOTTOM to top
    void* GetColorBuffer() const {
        return pixel_buffer->GetColorBuffer();
    }

    // create a new empty color buffer with the given frame size and format
    // @param[in] channel  channel layout, 
    //              in {`Format::RGB`, `Format::RGBA`, `Format::ARGB`, `Format::BGR`, `Format::BGRA`, `Format::ABGR`}
    // @param[in] type  data type, in {`Format::UINT8`, `Format::UINT16`, `Format::FLOAT32`, `Format::FLOAT64`}
    static void* NewColorBuffer(int width, int height, Format::Channel channel, Format::Type type) {
        return new unsigned char[width * height * Format::GetChannelSize(type) * Format::GetChannelNumber(channel)];
    }

    // create a new empty color buffer with the given frame size and format
    // @param[in] format  structure of format: {order, channel, type}
    // @param[in] format.order  buffer memory order, in {`Format::TOP_DOWN`, `Format::BOTTOM_UP`}
    // @param[in] format.channel  channel layout,
    //              in {`Format::RGB`, `Format::RGBA`, `Format::ARGB`, `Format::BGR`, `Format::BGRA`, `Format::ABGR`}
    // @param[in] format.type  data type, in {`Format::UINT8`, `Format::UINT16`, `Format::FLOAT32`, `Format::FLOAT64`}
    static void* NewColorBuffer(int width, int height, const Format& format) {
        return NewColorBuffer(width, height, format.channel, format.type);
    }

    // delete the color buffer
    static void DeleteColorBuffer(void* ptr) {
        delete[] reinterpret_cast<unsigned char*>(ptr);
    }

    // write color data in the frame buffer to the given color buffer (pointer `ptr`) with the given format
    // @param[in] order  buffer memory order, in {`Format::TOP_DOWN`, `Format::BOTTOM_UP`}
    // @param[in] channel  channel layout,
    //              in {`Format::RGB`, `Format::RGBA`, `Format::ARGB`, `Format::BGR`, `Format::BGRA`, `Format::ABGR`}.
    //              alpha channel will be always set to 1.0f
    // @param[in] type  data type, in {`Format::UINT8`, `Format::UINT16`, `Format::FLOAT32`, `Format::FLOAT64`}
    void WriteColorBuffer(void* ptr, Format::Order order, Format::Channel channel, Format::Type type) const {
        switch(order) {
            case Format::TOP_DOWN:
                for (int y = height - 1; y >= 0; y--) {
                    for (int x = 0; x < width; x++) {
                        Format::WriteOnePixel(GetColorAt(x, y), ptr, channel, type);
                    }
                }
                break;
            case Format::BOTTOM_UP:
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        Format::WriteOnePixel(GetColorAt(x, y), ptr, channel, type);
                    }
                }
                break;
        }
    }

    // write color data in the frame buffer to the given color buffer (pointer `ptr`) with the given format
    // @param[in] format  structure of format: {order, channel, type}
    // @param[in] format.order  buffer memory order, in {`Format::TOP_DOWN`, `Format::BOTTOM_UP`}
    // @param[in] format.channel  channel layout,
    //              in {`Format::RGB`, `Format::RGBA`, `Format::ARGB`, `Format::BGR`, `Format::BGRA`, `Format::ABGR`}.
    //              alpha channel will be always set to 1.0f
    // @param[in] format.type  data type, in {`Format::UINT8`, `Format::UINT16`, `Format::FLOAT32`, `Format::FLOAT64`}
    void WriteColorBuffer(void* ptr, const Format& format) const {
        WriteColorBuffer(ptr, format.order, format.channel, format.type);
    }

private:
    // insert a fragment to the per-pixel linked list by depth descending order
    void InsertFragmentToList_T(const glm::vec4& color, float depth, int x, int y, int thread_id) {
        pplist_buffer->InsertSortedAt_T(Fragment{color, depth}, x, y, thread_id);
        pplist_buffer_touched = true;
    }

    // blend per-pixel linked lists in given range of index
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendSliceProcess(int pplist_begin, int pplist_end) {
        for (int i = pplist_begin; i < pplist_end; i++) {
            pplist_buffer->BlendAt(pixel_buffer->ColorAt(i), pixel_buffer->DepthAt(i), i);
        }
    }

    // blend per-pixel linked lists, fetched atomically and orderly by threads themselves
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendCounterProcess(std::atomic<int>* counter) {
        while (true) {
            int i = counter->fetch_add(1, std::memory_order_relaxed);
            if (i >= width * height) break;
            pplist_buffer->BlendAt(pixel_buffer->ColorAt(i), pixel_buffer->DepthAt(i), i);
        }
    }

public:
    const int width;
    const int height;
    const bool enable_oit;

private:
    PixelBuffer* pixel_buffer; // per-pixel buffer
    PerPixelListBuffer* pplist_buffer; // per-pixel linked list buffer
    bool pplist_buffer_touched = false; // whether the pplist buffer has been touched since last clear
};

