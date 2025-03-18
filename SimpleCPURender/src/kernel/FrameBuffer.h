#pragma once

#include <vector>
#include <list>
#include <glm/glm.hpp>
#include "utility.h"
#include "Primitive.h"
#include "PixelBuffer.h"
#include "PerPixelListBuffer.h"


class FrameBuffer {
private:
    using IndexMapFunc = int (*)(int, int, int, int);
    using PerPixelListBuffer = PerPixelListBuffer_CAS;

public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //                        if false, the frame buffer will always ignore alpha channel,
    //                        which means fragment will be covered whenever it is in front of the existing fragment.
    FrameBuffer(int width, int height, bool enable_oit = false)
        : width(width), height(height), enable_oit(enable_oit) {
        pixel_buffer = new PixelBuffer(width, height);
        if (enable_oit) pplist_buffer = new PerPixelListBuffer(width, height, 8);
        Clear();
    }

    FrameBuffer(const FrameBuffer&) = delete;

    FrameBuffer& operator=(const FrameBuffer&) = delete;

    ~FrameBuffer() {
        delete pixel_buffer;
        if (enable_oit) delete pplist_buffer;
    }

    auto* GetAllocator(int idx) {
        return pplist_buffer->GetAllocator(idx);
    }

    // clear the frame buffer
    // @param[in] bg_color  background color
    void Clear(const glm::vec3& bg_color = glm::vec3(0.0f)) {
        pixel_buffer->Clear(bg_color);

        if (pplist_buffer_touched) {
            pplist_buffer->Clear();  
            pplist_buffer_touched = false;
        }
    }

    // handle a new fragment with the given color and depth at (x, y).
    // depend on whether OIT is enabled and the alpha value of the color,
    // this function may cover the existing fragment or insert the new fragment to the per-pixel linked list
    // @note assume the fragment has passed depth test
    template <class Allocator>
    void HandleNewFragment(const glm::vec4& color, float depth, int x, int y, Allocator* allocator) {
        if (!enable_oit || color.a > 0.9999f)
            CoverFragment(color, depth, x, y);
        else
            InsertFragmentSorted(color, depth, x, y, allocator);
    }

    // cover the fragment at (x, y) with the given color and depth
    void CoverFragment(const glm::vec4& color, float depth, int x, int y) {
        pixel_buffer->CoverAt_T(glm::vec3(color), depth, x, y);
    }

    // blend the per-pixel linked list to the color buffer;
    // meanwhile, clear the per-pixel linked list
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void Blend(int thread_num = 1) {
        if (!pplist_buffer_touched) return;

        if (thread_num == 1) {
            BlendProcess(0, width * height);
        }
        else {
            std::vector<int> split_points = RangeSplit(0, width * height, thread_num);
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_num; i++) {
                int pplist_begin = split_points[i];
                int pplist_end = split_points[i + 1];
                threads.emplace_back(&FrameBuffer::BlendProcess, this, pplist_begin, pplist_end);
            }
            for (std::thread& thread : threads) thread.join();
        }
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

    bool DepthTestAt(float depth, int x, int y) {
        return depth < GetDepthAt(x, y);
    }

    // get color with buffer memory order
    const glm::vec3& GetColorAt(int x, int y) const {
        return pixel_buffer->ColorAt(x, y);
    }

    // get color with buffer memory order
    float GetDepthAt(int x, int y) const {
        return pixel_buffer->DepthAt(x, y);
    }

private:
    // insert a fragment to the per-pixel linked list by depth descending order
    template <class Allocator>
    void InsertFragmentSorted(const glm::vec4& color, float depth, int x, int y, Allocator* allocator) {
        pplist_buffer->InsertSortedAt_T(Fragment{color, depth}, x, y, allocator);
        pplist_buffer_touched = true;
    }

    // blend the per-pixel linked list to `base_color`, given the range of y
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendProcess(int pplist_begin, int pplist_end) {
        for (int i = pplist_begin; i < pplist_end; i++) {
            pplist_buffer->BlendAt(pixel_buffer->ColorAt(i), pixel_buffer->DepthAt(i), i);
        }
    }

// protected:
public:
    const int width;
    const int height;
    const bool enable_oit;
    // const IndexMapFunc index_map_func;

    PixelBuffer* pixel_buffer; // per-pixel buffer
    PerPixelListBuffer* pplist_buffer;
    bool pplist_buffer_touched = false; // whether the pplist buffer has been touched since last clear
};

