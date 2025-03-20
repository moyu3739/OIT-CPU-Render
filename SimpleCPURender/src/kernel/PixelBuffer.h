#pragma once

#include <mutex>
#include <memory>
#include <omp.h>
#include <glm/glm.hpp>


class PixelBuffer {
private:
    struct Pixel {
        glm::vec3 color;
        float depth;
    };

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

private:
    const int width;
    const int height;
    const Pixel bg_pixel;
    Pixel* pixel_buffer;
    std::mutex* mtx_buffer;
};

