#pragma once

#include <vector>
#include <list>
#include <glm/glm.hpp>
#include "Primitive.h"


enum BufferVerticalOrder {
    TOP_DOWN,
    BOTTOM_UP
};

class FrameBuffer {
private:
    using IndexMapFunc = int (*)(int, int, int, int);
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] bvo  buffer vertical order (`TOP_DOWN` or `BOTTOM_UP`)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //                        if false, the frame buffer will always ignore alpha channel,
    //                        which means fragment will be covered whenever it is in front of the existing fragment.
    FrameBuffer(int width, int height, BufferVerticalOrder bvo = TOP_DOWN, bool enable_oit = false)
        : width(width), height(height), enable_oit(enable_oit), index_map_func(GetIndexMapFunc(bvo)) {
        color_buffer = new glm::vec3[width * height];
        depth_buffer = new float[width * height];
        if (enable_oit) pplist_buffer = new std::list<Fragment>[width * height];
        Clear();
    }

    FrameBuffer(const FrameBuffer&) = delete;

    FrameBuffer& operator=(const FrameBuffer&) = delete;

    ~FrameBuffer() {
        delete[] color_buffer;
        delete[] depth_buffer;
        if (enable_oit) delete[] pplist_buffer;
    }

    // clear the frame buffer
    // @param[in] bg_color  background color
    void Clear(const glm::vec3& bg_color = glm::vec3(0.0f)) {
        if (pplist_buffer_touched)
            for (int i = 0; i < width * height; i++) {
                color_buffer[i] = bg_color;
                depth_buffer[i] = INFINITY;
                pplist_buffer[i].clear();
            }
        else
            for (int i = 0; i < width * height; i++) {
                color_buffer[i] = bg_color;
                depth_buffer[i] = INFINITY;
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
            InsertFragment(color, depth, x, y);
    }

    // cover the fragment at (x, y) with the given color and depth
    void CoverFragment(const glm::vec4& color, float depth, int x, int y) {
        int idx = index_map_func(width, height, x, y);
        color_buffer[idx] = glm::vec3(color);
        depth_buffer[idx] = depth;
    }

    // blend the per-pixel linked list to the color buffer
    void Blend() {
        if (pplist_buffer_touched) {
            for (int i = 0; i < width * height; i++) {
                BlendPerPixelList(pplist_buffer[i], color_buffer[i], depth_buffer[i]);
                pplist_buffer[i].clear();
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
        return color_buffer[row * width + col];
    }

    // get color with buffer memory order
    float GetDepthAtIndex(int row, int col) const {
        return depth_buffer[row * width + col];
    }

    // get color with buffer memory order
    Fragment GetFragmentAtIndex(int row, int col) const {
        int idx = row * width + col;
        return Fragment{glm::vec4(color_buffer[idx], 1.0f), depth_buffer[idx]};
    }

    const glm::vec3* GetColorBuffer() const {
        return color_buffer;
    }

    // get color with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    glm::vec3& ColorAtCoord(int x, int y) {
        return color_buffer[index_map_func(width, height, x, y)];
    }

    // get color with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const glm::vec3& ColorAtCoord(int x, int y) const {
        return color_buffer[index_map_func(width, height, x, y)];
    }

    // get depth with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    float& DepthAtCoord(int x, int y) {
        return depth_buffer[index_map_func(width, height, x, y)];
    }

    // get depth with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const float& DepthAtCoord(int x, int y) const {
        return depth_buffer[index_map_func(width, height, x, y)];
    }

    // get fragment with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    Fragment FragmentAtCoord(int x, int y) const {
        int idx = index_map_func(width, height, x, y);
        return Fragment{glm::vec4(color_buffer[idx], 1.0f), depth_buffer[idx]};
    }

private:
    // insert a fragment to the per-pixel linked list by depth descending order
    void InsertFragment(const glm::vec4& color, float depth, int x, int y) {
        InsertFragment(Fragment{color, depth}, x, y);
    }

    // insert a fragment to the per-pixel linked list by depth descending order
    void InsertFragment(const Fragment& fragment, int x, int y) {
        auto& pplist = pplist_buffer[index_map_func(width, height, x, y)];
        auto iter = pplist.begin();
        while (iter != pplist.end() && iter->depth > fragment.depth) ++iter;
        pplist.insert(iter, fragment);
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

    // blend the per-pixel linked list to `base_color`
    static void BlendPerPixelList(const std::list<Fragment>& pplist, glm::vec3& base_color, float base_depth) {
        // skip fragments with depth larger than `base_depth`
        auto iter = pplist.begin();
        while (iter != pplist.end() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != pplist.end(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }

protected:
    const int width;
    const int height;
    const IndexMapFunc index_map_func;
    const bool enable_oit;

    glm::vec3* color_buffer;
    float* depth_buffer;
    std::list<Fragment>* pplist_buffer; // per-pixel linked list
    bool pplist_buffer_touched = false; // whether the pplist buffer has been touched
};

