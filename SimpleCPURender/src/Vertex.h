#pragma once

#include <glm/glm.hpp>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

struct FragmentPixel {
    glm::vec4 color;
    float depth;
};

