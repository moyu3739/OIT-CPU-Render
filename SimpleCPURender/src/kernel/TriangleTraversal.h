#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include "utility.h"


enum TriangleTraversalType {
    ON_FACE,
    ON_EDGE,
    ON_VERTEX
};


class TriangleTraversal {
public:
    TriangleTraversal() {}

    virtual ~TriangleTraversal() {}

    virtual bool Call(int width, int height, int pixel_x, int pixel_y,
        float screen_x, float screen_y, const glm::vec3& barycentric,
        const glm::vec4& screen_pos_v0, const glm::vec4& screen_pos_v1, const glm::vec4& screen_pos_v2) = 0;
};


class TriangleTraversalFace: public TriangleTraversal {
public:
    TriangleTraversalFace() {}

    virtual ~TriangleTraversalFace() {}

    virtual bool Call(
        int width, int height, int pixel_x, int pixel_y,
        float screen_x, float screen_y, const glm::vec3& barycentric,
        const glm::vec4& screen_pos_v0, const glm::vec4& screen_pos_v1, const glm::vec4& screen_pos_v2
    ) override {
        return barycentric.x >= 0 && barycentric.y >= 0 && barycentric.z >= 0;
    }
};


class TriangleTraversalEdge: public TriangleTraversal {
public:
    TriangleTraversalEdge(float thickness = 1.0f): half_thickness(thickness / 2.0f) {}

    virtual ~TriangleTraversalEdge() {}

    virtual bool Call(
        int width, int height, int pixel_x, int pixel_y,
        float screen_x, float screen_y, const glm::vec3& barycentric,
        const glm::vec4& screen_pos_v0, const glm::vec4& screen_pos_v1, const glm::vec4& screen_pos_v2
    ) override {
        return PixelAtEdge(width, height, pixel_x, pixel_y, screen_pos_v0, screen_pos_v1, half_thickness)
            || PixelAtEdge(width, height, pixel_x, pixel_y, screen_pos_v1, screen_pos_v2, half_thickness)
            || PixelAtEdge(width, height, pixel_x, pixel_y, screen_pos_v2, screen_pos_v0, half_thickness);
    }

    // check if the pixel is at the edge
    static bool PixelAtEdge(
        int width, int height, int pixel_x, int pixel_y,
        const glm::vec4& screen_pos_a, const glm::vec4& screen_pos_b, float half_thickness = 0.5f
    ) {
        float pixel_x_a = Screen2PixelFloat(screen_pos_a.x, width);
        float pixel_y_a = Screen2PixelFloat(screen_pos_a.y, height);
        float pixel_x_b = Screen2PixelFloat(screen_pos_b.x, width);
        float pixel_y_b = Screen2PixelFloat(screen_pos_b.y, height);

        // assume the formula of edge ab is: Ax + By + C = 0
        float A = pixel_y_b - pixel_y_a;
        float B = pixel_x_a - pixel_x_b;
        float C = pixel_x_b * pixel_y_a - pixel_x_a * pixel_y_b;

        float d = std::abs(A * pixel_x + B * pixel_y + C) / std::sqrt(A * A + B * B);
        return d < half_thickness;
    }

    float GetThickness() const {
        return 2.0f * half_thickness;
    }

    void SetThickness(float thickness) {
        half_thickness = thickness / 2.0f;
    }

private:
    float half_thickness; // half of edge thickness
};


class TriangleTraversalVertex: public TriangleTraversal {
public:
    TriangleTraversalVertex(float thickness = 1.0f): half_thickness(thickness / 2.0f) {}

    virtual ~TriangleTraversalVertex() {}

    virtual bool Call(
        int width, int height, int pixel_x, int pixel_y,
        float screen_x, float screen_y, const glm::vec3& barycentric,
        const glm::vec4& screen_pos_v0, const glm::vec4& screen_pos_v1, const glm::vec4& screen_pos_v2
    ) override {
        return PixelAtVertex(width, height, pixel_x, pixel_y, screen_pos_v0, half_thickness)
            || PixelAtVertex(width, height, pixel_x, pixel_y, screen_pos_v1, half_thickness)
            || PixelAtVertex(width, height, pixel_x, pixel_y, screen_pos_v2, half_thickness);
    }

    // check if the pixel is at the vertex
    static bool PixelAtVertex(
        int width, int height, int pixel_x, int pixel_y,
        const glm::vec4& screen_pos_v, float half_thickness = 0.5f
    ) {
        float pixel_x_v = Screen2PixelFloat(screen_pos_v.x, width);
        float pixel_y_v = Screen2PixelFloat(screen_pos_v.y, height);

        float dx = std::abs(pixel_x - pixel_x_v);
        float dy = std::abs(pixel_y - pixel_y_v);
        return dx < half_thickness && dy < half_thickness;
    }

    float GetThickness() const {
        return 2.0f * half_thickness;
    }

    void SetThickness(float thickness) {
        half_thickness = thickness / 2.0f;
    }

private:
    float half_thickness; // half of vertex thickness
};


class AllTriangleTraversal {
public:
    static TriangleTraversalFace face_triange_traversal;
    static TriangleTraversalEdge edge_triangle_traversal;
    static TriangleTraversalVertex vertex_triangle_traversal;
};
