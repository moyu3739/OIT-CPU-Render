#pragma once

#include <glm/glm.hpp>


namespace oit {

struct Pixel {
    glm::vec3 color;
    float depth;
};

struct Fragment {
    glm::vec4 color;
    float depth;
};

} // namespace oit
