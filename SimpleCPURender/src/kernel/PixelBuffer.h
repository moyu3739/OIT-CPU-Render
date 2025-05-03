#pragma once

#include <mutex>
#include <glm/glm.hpp>
#include "Primitive.h"


class PixelBuffer {
public:
    PixelBuffer(int width, int height, const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY)
    : width(width), height(height), bg_pixel{bg_color, bg_depth} {
        pixel_buffer = new Pixel[width * height];
        mtx_buffer = new std::mutex[width * height];
    }

    ~PixelBuffer() {
        delete[] pixel_buffer;
        delete[] mtx_buffer;
    }

    void Clear() {
        for (int i = 0; i < width * height; i++) memcpy(&pixel_buffer[i], &bg_pixel, sizeof(Pixel));
    }

    void Clear(const glm::vec3& bg_color, float bg_depth = INFINITY) {
        for (int i = 0; i < width * height; i++) SetAt(bg_color, bg_depth, i);
    }

    glm::vec3& ColorAt(int x, int y) {
        return ColorAt(y * width + x);
    }

    const glm::vec3& ColorAt(int x, int y) const {
        return ColorAt(y * width + x);
    }

    float& DepthAt(int x, int y) {
        return DepthAt(y * width + x);
    }

    const float& DepthAt(int x, int y) const {
        return DepthAt(y * width + x);
    }

    void SetAt(const glm::vec3& color, float depth, int x, int y) {
        SetAt(color, depth, y * width + x);
    }

    void CoverAt_T(const glm::vec3& color, float depth, int x, int y) {
        CoverAt_T(color, depth, y * width + x);
    }

    glm::vec3& ColorAt(int idx) {
        return pixel_buffer[idx].color;
    }

    const glm::vec3& ColorAt(int idx) const {
        return pixel_buffer[idx].color;
    }

    float& DepthAt(int idx) {
        return pixel_buffer[idx].depth;
    }

    const float& DepthAt(int idx) const {
        return pixel_buffer[idx].depth;
    }

    void SetAt(const glm::vec3& color, float depth, int idx) {
        pixel_buffer[idx].color = color;
        pixel_buffer[idx].depth = depth;
    }

    void CoverAt_T(const glm::vec3& color, float depth, int idx) {
        std::lock_guard<std::mutex> lock(mtx_buffer[idx]);
        if (depth < pixel_buffer[idx].depth) SetAt(color, depth, idx);
    }

    // get the color buffer directly
    // @note  the format of the color buffer is as below:
    // @note    - 'RGBA' 4 channels, 32 bits floating number per channel, 128 bits per pixel
    // @note    - 'A' channel can be any value, which should be IGNORED
    // @note    - buffer order is firstly from left to right, and then from BOTTOM to top
    void* GetColorBuffer() const {
        return pixel_buffer;
    }

private:
    const int width;
    const int height;
    const Pixel bg_pixel;
    Pixel* pixel_buffer;
    std::mutex* mtx_buffer;
};

