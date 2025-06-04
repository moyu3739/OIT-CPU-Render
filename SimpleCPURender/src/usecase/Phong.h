#pragma once

#include <string>
#include <glm/glm.hpp>
#include "PhongShader.h"


glm::mat4 GetModelTransform(const glm::vec3& translation, float rotation, float scale);

void LoadModel(const std::string& obj_path, oit::VertexBuffer& vertex_buffer);

void QuickStart();
