#pragma once

#include <vector>
#include "Primitive.h"


class FrameBuffer{
public:
    FrameBuffer(int width, int height) {
        data.resize(width, std::vector<Fragment>(height, {glm::vec4(0.0f), INFINITY}));
    }

    int GetWidth() const {
        return data.size();
    }

    int GetHeight() const {
        return data[0].size();
    }

    Fragment& At(int x, int y) {
        return data[x][y];
    }

    const Fragment& At(int x, int y) const {
        return data[x][y];
    }

    // clear the frame buffer
    void Clear() {
        for(auto& row : data){
            for(auto& pixel : row){
                pixel.color = glm::vec4(0.0f);
                pixel.depth = INFINITY;
            }
        }
    }

private:
    std::vector<std::vector<Fragment>> data;
};

