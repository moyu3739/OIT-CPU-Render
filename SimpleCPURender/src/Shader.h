#pragma once

#include <glm/glm.hpp>


class VertexShader{
public:
    struct VertexShaderInput{
        glm::vec3 model_pos; // vertex position in model space
        glm::vec3 model_normal; // vertex normal in model space
    };

    struct VertexShaderOutput{
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec4 __position__; // vertex position in clip space
    };

public:
    VertexShader() {}

    VertexShaderOutput Call(const VertexShaderInput& input){
        VertexShaderOutput output;
        output.world_pos = glm::vec3(model * glm::vec4(input.model_pos, 1.0f));
        output.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model))) * input.model_normal);
        output.__position__ = projection * view * model * glm::vec4(input.model_pos, 1.0f);
        return output;
    }

public:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class FragmentShader{
public:
    using FragmentShaderInput = VertexShader::VertexShaderOutput;

    struct FragmentShaderOutput{
        glm::vec4 __color__; // fragment color
    };

public:
    FragmentShader() {}

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    template <typename T>
    T Interpolate(const T& v1, const T& v2, const T& v3, const glm::vec3& barycentric){
        return (barycentric.x * v1 + barycentric.y * v2 + barycentric.z * v3)
                / (barycentric.x + barycentric.y + barycentric.z);
    }

    FragmentShaderOutput Call(const FragmentShaderInput& input){
        glm::vec3 ambient = ka * obj_color;
        glm::vec3 light_dir = glm::normalize(light_pos - input.world_pos);
        glm::vec3 diffuse = (kd * glm::max(glm::dot(input.world_normal, light_dir), 0.0f)) * obj_color * light_color;
        // glm::vec3 diffuse = (kd * glm::abs(glm::dot(input.world_normal, light_dir))) * obj_color * light_color;
        glm::vec3 color = ambient + diffuse;
        return FragmentShaderOutput{glm::vec4(color, 1.0f)};
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color;
    float ka;
    float kd;
    glm::vec3 obj_color;
};
