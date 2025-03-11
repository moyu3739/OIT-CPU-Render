#pragma once

#include <vector>
#include <glm/glm.hpp>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

struct Fragment {
    glm::vec4 color;
    float depth;
};

struct Object {
    std::vector<Vertex> vertices;

};

