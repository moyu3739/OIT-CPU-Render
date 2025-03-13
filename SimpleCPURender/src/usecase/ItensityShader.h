#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"


class ItensityVertexShader: public VertexShader{
public:
    struct Input{
        glm::vec3 model_pos; // vertex position in model space
        glm::vec3 model_normal; // vertex normal in model space
        glm::vec2 texcoord; // texture coordinate
    };

    struct Output{
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec2 texcoord; // texture coordinate
        glm::vec4 __position__; // vertex position in clip space
    };

public:
    ItensityVertexShader() {}

    ~ItensityVertexShader() {}

    Output Call(const Input& input){
        Output output;
        output.world_pos = glm::vec3(model * glm::vec4(input.model_pos, 1.0f));
        output.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model))) * input.model_normal);
        output.texcoord = input.texcoord;
        output.__position__ = projection * view * model * glm::vec4(input.model_pos, 1.0f);
        return output;
    }

public:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class ItensityFragmentShader: public FragmentShader{
public:
    struct Input{
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec2 texcoord; // texture coordinate
    };

    struct Output{
        glm::vec4 __color__; // fragment color
    };

public:
    ItensityFragmentShader() {}

    ~ItensityFragmentShader() {}

    Output Call(const Input& input){
        obj_color = texture->Sample(input.texcoord);
        glm::vec3 ambient = ka * obj_color;
        glm::vec3 light_dir = glm::normalize(light_pos - input.world_pos);
        glm::vec3 diffuse = (kd * glm::max(glm::dot(input.world_normal, light_dir), 0.0f)) * obj_color * light_color;
        // glm::vec3 diffuse = (kd * glm::abs(glm::dot(input.world_normal, light_dir))) * obj_color * light_color;
        glm::vec3 color = ambient + diffuse;
        return Output{glm::vec4(color, Clamp(input.world_pos.y / 10.0f + 0.5f, 0.0f, 1.0f))};
    }

    // interpolate a vertex attribute
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    template <typename T>
    T InterpolateAttr(const T& v1, const T& v2, const T& v3, const glm::vec3& barycentric){
        return (barycentric.x * v1 + barycentric.y * v2 + barycentric.z * v3)
                / (barycentric.x + barycentric.y + barycentric.z);
    }

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    Input Interpolate(
            const ItensityVertexShader::Output& v1,
            const ItensityVertexShader::Output& v2,
            const ItensityVertexShader::Output& v3,
            const glm::vec3& barycentric){
        Input fs_input;
        fs_input.world_pos = InterpolateAttr(v1.world_pos, v2.world_pos, v3.world_pos, barycentric);
        fs_input.world_normal = InterpolateAttr(v1.world_normal, v2.world_normal, v3.world_normal, barycentric);
        fs_input.texcoord = InterpolateAttr(v1.texcoord, v2.texcoord, v3.texcoord, barycentric);
        return fs_input;
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color;
    float ka;
    float kd;
    glm::vec3 obj_color;
    Texture* texture = nullptr;
};

