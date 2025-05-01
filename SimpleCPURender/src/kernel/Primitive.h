#pragma once

#include <glm/glm.hpp>


struct Pixel {
    glm::vec3 color;
    float depth;
};

struct Fragment {
    glm::vec4 color;
    float depth;
};

