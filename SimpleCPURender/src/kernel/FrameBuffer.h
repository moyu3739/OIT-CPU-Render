#pragma once

#include <vector>
#include <list>
#include <atomic>
#include <glm/glm.hpp>
#include "utility.h"
#include "Primitive.h"
#include "PixelBuffer.h"
#include "PerPixelListBuffer.h"
#include "ListAllocator.h"


class FrameBuffer {
private:
    using IndexMapFunc = int (*)(int, int, int, int);

public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //                        if false, the frame buffer will always ignore alpha channel,
    //                        which means fragment will be covered whenever it is in front of the existing fragment.
    FrameBuffer(int width, int height, int allocator_num,
                const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
                bool enable_oit = false)
        : width(width), height(height), enable_oit(enable_oit) {
        pixel_buffer = new PixelBuffer(width, height, bg_color, bg_depth);
        if (enable_oit) pplist_buffer = new PerPixelListBuffer(width, height, allocator_num);
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

    // handle a new fragment with the given color and depth at (x, y).
    // depend on whether OIT is enabled and the alpha value of the color,
    // this function may cover the existing fragment or insert the new fragment to the per-pixel linked list
    // @note assume the fragment has passed depth test
    void HandleNewFragment_T(const glm::vec4& color, float depth, int x, int y, int thread_id) {
        if (!enable_oit || color.a > 0.9999f)
            CoverFragment_T(color, depth, x, y);
        else
            InsertFragmentSorted_T(color, depth, x, y, thread_id);
    }

    // cover the fragment at (x, y) with the given color and depth
    void CoverFragment_T(const glm::vec4& color, float depth, int x, int y) {
        pixel_buffer->CoverAt_T(glm::vec3(color), depth, x, y);
    }

    // blend the per-pixel linked list to the color buffer
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void Blend(int thread_num = 1) {
        BlendSplit(thread_num);
        // BlendFetch(thread_num);
    }

    // blend the per-pixel linked list to the color buffer,
    // using paralleling method of trivial splitting list buffers equally
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void BlendSplit(int thread_num = 1) {
        if (!pplist_buffer_touched) return;

        if (thread_num == 1) {
            BlendProcessRange(0, width * height);
        }
        else {
            std::vector<int> split_points = RangeSplit(0, width * height, thread_num);
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_num; i++) {
                int pplist_begin = split_points[i];
                int pplist_end = split_points[i + 1];
                threads.emplace_back(&FrameBuffer::BlendProcessRange, this, pplist_begin, pplist_end);
            }
            for (std::thread& thread : threads) thread.join();
        }
    }

    // blend the per-pixel linked list to the color buffer,
    // using paralleling method of atomic fetching list by threads
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void BlendFetch(int thread_num = 1) {
        if (!pplist_buffer_touched) return;

        if (thread_num == 1) {
            BlendProcessRange(0, width * height);
        }
        else {
            std::atomic<int> counter;
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_num; i++) {
                threads.emplace_back(&FrameBuffer::BlendProcessFetch, this, &counter);
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
    void InsertFragmentSorted_T(const glm::vec4& color, float depth, int x, int y, int thread_id) {
        pplist_buffer->InsertSortedAt_T(Fragment{color, depth}, x, y, thread_id);
        pplist_buffer_touched = true;
    }

    // blend the per-pixel linked list to `base_color`, given the range of y
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendProcessRange(int pplist_begin, int pplist_end) {
        for (int i = pplist_begin; i < pplist_end; i++) {
            pplist_buffer->BlendAt(pixel_buffer->ColorAt(i), pixel_buffer->DepthAt(i), i);
        }
    }

    // blend the per-pixel linked list to `base_color`,
    // lists fetched atomically and orderly by threads themselves
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendProcessFetch(std::atomic<int>* counter) {
        while (true) {
            int i = counter->fetch_add(1, std::memory_order_relaxed);
            if (i >= width * height) break;
            pplist_buffer->BlendAt(pixel_buffer->ColorAt(i), pixel_buffer->DepthAt(i), i);
        }
    }

private:
    const int width;
    const int height;
    const bool enable_oit;

    PixelBuffer* pixel_buffer; // per-pixel buffer
    PerPixelListBuffer* pplist_buffer; // per-pixel linked list buffer
    bool pplist_buffer_touched = false; // whether the pplist buffer has been touched since last clear
};

