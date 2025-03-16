#pragma once

#include <mutex>
#include <atomic>
#include <glm/glm.hpp>


class AtomicPixel {
public:
    AtomicPixel() {}

    glm::vec3& Color() {
        return color;
    }

    const glm::vec3& Color() const {
        return color;
    }

    float& Depth() {
        return depth;
    }

    const float& Depth() const {
        return depth;
    }

    void Set(const glm::vec3& color, float depth) {
        this->color = color;
        this->depth = depth;
    }

    void Cover(const glm::vec3& color, float depth) {
        std::lock_guard<std::mutex> lock(mtx);
        if (depth < this->depth) {
            this->color = color;
            this->depth = depth;
        }
    }

private:
    glm::vec3 color;
    float depth;
    std::mutex mtx;
};







class __AtomicPixel {
private:
    struct Pixel {
        glm::vec3 color;
        float depth;
    };

public:
    __AtomicPixel() {}

    glm::vec3 GetColor() const {
        return pixel.load(std::memory_order_relaxed).color;
    }

    float GetDepth() {
        return pixel.load(std::memory_order_relaxed).depth;
    }

    // void SetColor(const glm::vec3& color) {
    //     Pixel p = pixel.load(std::memory_order_relaxed);
    //     p.color = color;
    //     pixel.store(p, std::memory_order_relaxed);
    // }

    // void SetDepth(float depth) {
    //     Pixel p = pixel.load(std::memory_order_relaxed);
    //     p.depth = depth;
    //     pixel.store(p, std::memory_order_relaxed);
    // }

    void Set(const glm::vec3& color, float depth) {
        Pixel p{color, depth};
        pixel.store(p, std::memory_order_relaxed);
    }

    void Cover(const glm::vec3& color, float depth) {
        Pixel new_pixel{color, depth};
        Pixel old_pixel = pixel.load(std::memory_order_acquire);
        while (new_pixel.depth < old_pixel.depth) {
            if (pixel.compare_exchange_strong(
                old_pixel, new_pixel,
                std::memory_order_release, std::memory_order_acquire
            )) {
                break;
            }
        }
    }

private:
    std::atomic<Pixel> pixel;
};
