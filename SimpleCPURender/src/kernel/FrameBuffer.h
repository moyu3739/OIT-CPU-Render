#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Primitive.h"


// class FrameBuffer{
// public:
//     FrameBuffer(int width, int height) {
//         data.resize(width, std::vector<Fragment>(height, {glm::vec4(0.0f), INFINITY}));
//     }

//     int GetWidth() const {
//         return data.size();
//     }

//     int GetHeight() const {
//         return data[0].size();
//     }

//     Fragment& At(int x, int y) {
//         return data[x][y];
//     }

//     const Fragment& At(int x, int y) const {
//         return data[x][y];
//     }

//     // clear the frame buffer
//     void Clear() {
//         for(auto& row : data){
//             for(auto& pixel : row){
//                 pixel.color = glm::vec4(0.0f);
//                 pixel.depth = INFINITY;
//             }
//         }
//     }

// // public:
// //     int width;
// //     int height;

// private:
//     std::vector<std::vector<Fragment>> data;
// };


enum BufferVerticalOrder {
    TOP_DOWN,
    BOTTOM_UP
};

class FrameBuffer {
private:
    using IndexMapFunc = int (*)(int, int, int, int);
public:
    FrameBuffer(int width, int height, BufferVerticalOrder bvo = TOP_DOWN)
        : width(width), height(height), index_map_func(GetIndexMapFunc(bvo)) {
        color = new glm::vec4[width * height];
        depth = new float[width * height];
        Clear();
    }

    ~FrameBuffer() {
        delete[] color;
        delete[] depth;
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

    // get color with buffer memory order
    const glm::vec4& GetColorAtIndex(int row, int col) const {
        return color[row * width + col];
    }

    // get color with buffer memory order
    float GetDepthAtIndex(int row, int col) const {
        return depth[row * width + col];
    }

    // get color with buffer memory order
    Fragment GetFragmentAtIndex(int row, int col) const {
        int idx = row * width + col;
        return Fragment{color[idx], depth[idx]};
    }

    const glm::vec4* GetColorBuffer() const {
        return color;
    }

    // clear the frame buffer
    void Clear() {
        for (int i = 0; i < width * height; i++) {
            color[i] = glm::vec4(0.0f);
            depth[i] = INFINITY;
        }
    }

    // get color with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    glm::vec4& ColorAtCoord(int x, int y) {
        return color[index_map_func(width, height, x, y)];
    }

    // get color with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const glm::vec4& ColorAtCoord(int x, int y) const {
        return color[index_map_func(width, height, x, y)];
    }

    // get depth with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    float& DepthAtCoord(int x, int y) {
        return depth[index_map_func(width, height, x, y)];
    }

    // get depth with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    const float& DepthAtCoord(int x, int y) const {
        return depth[index_map_func(width, height, x, y)];
    }

    // get fragment with coordinate in clipping space ( (0, 0) is at the left-bottom corner )
    Fragment FragmentAtCoord(int x, int y) const {
        int idx = index_map_func(width, height, x, y);
        return Fragment{color[idx], depth[idx]};
    }

private:
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

private:
    int width;
    int height;
    const IndexMapFunc index_map_func;

    glm::vec4* color;
    float* depth;
};

