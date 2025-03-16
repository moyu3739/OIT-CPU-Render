#pragma once

#include <vector>
#include <list>
#include <glm/glm.hpp>
#include "utility.h"
#include "Primitive.h"
#include "AtomicPixel.h"
#include "AtomicFragmentList.h"


enum BufferVerticalOrder {
    TOP_DOWN,
    BOTTOM_UP
};

class FrameBuffer {
private:
    using IndexMapFunc = int (*)(int, int, int, int);
    using AtomicFragmentList = AtomicFragmentList_CAS;
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] bvo  buffer vertical order (`TOP_DOWN` or `BOTTOM_UP`)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //                        if false, the frame buffer will always ignore alpha channel,
    //                        which means fragment will be covered whenever it is in front of the existing fragment.
    FrameBuffer(int width, int height, BufferVerticalOrder bvo = TOP_DOWN, bool enable_oit = false)
        : width(width), height(height), enable_oit(enable_oit), index_map_func(GetIndexMapFunc(bvo)) {
        pixel_buffer = new AtomicPixel[width * height];
        if (enable_oit) pplist_buffer = new AtomicFragmentList[width * height];
        Clear();
    }

    FrameBuffer(const FrameBuffer&) = delete;

    FrameBuffer& operator=(const FrameBuffer&) = delete;

    ~FrameBuffer() {
        delete[] pixel_buffer;
        if (enable_oit) delete[] pplist_buffer;
    }

    // clear the frame buffer
    // @param[in] bg_color  background color
    void Clear(const glm::vec3& bg_color = glm::vec3(0.0f)) {
        if (pplist_buffer_touched)
            for (int i = 0; i < width * height; i++) {
                pixel_buffer[i].Set(bg_color, INFINITY);
                pplist_buffer[i].Clear();
            }
        else
            for (int i = 0; i < width * height; i++) {
                pixel_buffer[i].Set(bg_color, INFINITY);
            }

        pplist_buffer_touched = false;
    }

    // handle a new fragment with the given color and depth at (x, y).
    // depend on whether OIT is enabled and the alpha value of the color,
    // this function may cover the existing fragment or insert the new fragment to the per-pixel linked list
    // @note assume the fragment has passed depth test
    void HandleNewFragment(const glm::vec4& color, float depth, int x, int y) {
        if (!enable_oit || color.a > 0.9999f)
            CoverFragment(color, depth, x, y);
        else
            InsertFragmentSorted(color, depth, x, y);
    }

    // cover the fragment at (x, y) with the given color and depth
    void CoverFragment(const glm::vec4& color, float depth, int x, int y) {
        int idx = index_map_func(width, height, x, y);
        // pixel_buffer[idx].color = glm::vec3(color);
        // pixel_buffer[idx].depth = depth;
        pixel_buffer[idx].Cover(glm::vec3(color), depth);
    }

    // blend the per-pixel linked list to the color buffer;
    // meanwhile, clear the per-pixel linked list
    // @param[in] thread_num  number of threads to blend the per-pixel linked list
    void Blend(int thread_num = 1) {
        if (pplist_buffer_touched) {
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

            pplist_buffer_touched = false;
        }
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

    bool DepthTest(float depth, int x, int y) {
        return depth < DepthAtCoord(x, y);
    }

    // get color with buffer memory order
    const glm::vec3& GetColorAtIndex(int row, int col) const {
        return pixel_buffer[row * width + col].Color();
    }

    // get color with buffer memory order
    float GetDepthAtIndex(int row, int col) const {
        return pixel_buffer[row * width + col].Depth();
    }

    const glm::vec4* GetColorBuffer() const {
        return reinterpret_cast<const glm::vec4*>(pixel_buffer);
    }

    // get color with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const glm::vec3& ColorAtCoord(int x, int y) const {
        return pixel_buffer[index_map_func(width, height, x, y)].Color();
    }

    // get depth with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const float& DepthAtCoord(int x, int y) const {
        return pixel_buffer[index_map_func(width, height, x, y)].Depth();
    }

    // get fragment with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    Fragment FragmentAtCoord(int x, int y) const {
        int idx = index_map_func(width, height, x, y);
        return Fragment{glm::vec4(pixel_buffer[idx].Color(), 1.0f), pixel_buffer[idx].Depth()};
    }

private:
    // insert a fragment to the per-pixel linked list by depth descending order
    void InsertFragmentSorted(const glm::vec4& color, float depth, int x, int y) {
        InsertFragmentSorted(Fragment{color, depth}, x, y);
    }

    // insert a fragment to the per-pixel linked list by depth descending order
    void InsertFragmentSorted(const Fragment& fragment, int x, int y) {
        auto& pplist = pplist_buffer[index_map_func(width, height, x, y)];
        pplist.InsertSorted(fragment, x, y);
        pplist_buffer_touched = true;
    }

    static int MapIndexTopDown(int width, int height, int x, int y) {
        return (height - 1 - y) * width + x;
    }

    static int MapIndexBottomUp(int width, int height, int x, int y) {
        return y * width + x;
    }

    static IndexMapFunc GetIndexMapFunc(BufferVerticalOrder bvo) {
        switch (bvo) {
            case TOP_DOWN: return MapIndexTopDown;
            case BOTTOM_UP: return MapIndexBottomUp;
        }
    }

    // blend the per-pixel linked list to `base_color`, given the range of y
    // @param[in] pplist_begin  index of the first element in the per-pixel linked list
    // @param[in] pplist_end  index AFTER the last element in the per-pixel linked list
    void BlendProcess(int pplist_begin, int pplist_end) {
        for (int i = pplist_begin; i < pplist_end; i++) {
            pplist_buffer[i].Blend(pixel_buffer[i].Color(), pixel_buffer[i].Depth());
            pplist_buffer[i].Clear();
        }
    }

protected:
    const int width;
    const int height;
    const bool enable_oit;
    const IndexMapFunc index_map_func;

    AtomicPixel* pixel_buffer; // per-pixel buffer
    AtomicFragmentList* pplist_buffer; // per-pixel linked list
    bool pplist_buffer_touched = false; // whether the pplist buffer has been touched since last clear
};

