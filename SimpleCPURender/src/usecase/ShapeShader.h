#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"

#undef max
#undef min


class ShapeVertexShader: public VertexShader{
public:
    struct Input: public VertexShaderInput {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };

    struct Output: public VertexShaderOutput {
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec3 color; // texture coordinate
    };

public:
    ShapeVertexShader() {}

    ~ShapeVertexShader() {}

    virtual VertexShaderInput* MakeInput() const override {
        return new Input;
    }

    virtual VertexShaderOutput* MakeOutput() const override {
        return new Output;
    }

    virtual void DestroyInput(VertexShaderInput* input) const override {
        delete reinterpret_cast<Input*>(input);
    }

    virtual void DestroyOutput(VertexShaderOutput* output) const override {
        delete reinterpret_cast<Output*>(output);
    }

    virtual void Call(const VertexShaderInput& input, VertexShaderOutput& output) override {
        const Input& rinput = reinterpret_cast<const Input&>(input);
        Output& routput = reinterpret_cast<Output&>(output);

        glm::mat4 overall_model = *global_model * model;
        routput.world_pos = glm::vec3(overall_model * glm::vec4(rinput.position, 1.0f));
        routput.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(overall_model))) * rinput.normal);
        routput.color = rinput.color;
        routput.__position__ = projection * view * overall_model * glm::vec4(rinput.position, 1.0f);
    }

public:
    std::shared_ptr<glm::mat4> global_model;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class ShapeFragmentShader: public FragmentShader{
public:
    struct Input: public FragmentShaderInput {
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec3 color; // texture coordinate
    };

    struct Output: public FragmentShaderOutput {};

public:
    ShapeFragmentShader() {}

    ~ShapeFragmentShader() {}

    float Linear(float x1, float y1, float x2, float y2, float x) {
        if (x1 == x2) return y1;
        return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
    }

    virtual FragmentShaderInput* MakeInput() const override {
        return new Input;
    }

    virtual FragmentShaderOutput* MakeOutput() const override {
        return new Output;
    }

    virtual void DestroyInput(FragmentShaderInput* input) const override {
        delete reinterpret_cast<Input*>(input);
    }

    virtual void DestroyOutput(FragmentShaderOutput* output) const override {
        delete reinterpret_cast<Output*>(output);
    }

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    // @param[out] fs_input  fragment shader input
    virtual void Interpolate(
        const VertexShaderOutput& v0,
        const VertexShaderOutput& v1,
        const VertexShaderOutput& v2,
        const glm::vec3& barycentric,
        FragmentShaderInput& fs_input
    ) override {
        const auto& rv0 = reinterpret_cast<const ShapeVertexShader::Output&>(v0);
        const auto& rv1 = reinterpret_cast<const ShapeVertexShader::Output&>(v1);
        const auto& rv2 = reinterpret_cast<const ShapeVertexShader::Output&>(v2);
        Input& rfs_input = reinterpret_cast<Input&>(fs_input);

        rfs_input.world_pos = InterpolateAttr(rv0.world_pos, rv1.world_pos, rv2.world_pos, barycentric);
        rfs_input.world_normal = InterpolateAttr(rv0.world_normal, rv1.world_normal, rv2.world_normal, barycentric);
        rfs_input.color = InterpolateAttr(rv0.color, rv1.color, rv2.color, barycentric);
    }

    virtual void Call(const FragmentShaderInput& input, FragmentShaderOutput& output) override {
        const Input& rinput = reinterpret_cast<const Input&>(input);

        glm::vec3 ambient = ka * rinput.color;
        glm::vec3 light_dir = glm::normalize(light_pos - rinput.world_pos);
        // glm::vec3 diffuse = (kd * ut::Max(glm::dot(rinput.world_normal, light_dir), 0.0f)) * rinput.color * light_color;
        glm::vec3 diffuse = (kd * glm::abs(glm::dot(rinput.world_normal, light_dir))) * rinput.color * light_color;
        glm::vec3 color = ambient + diffuse;

        output.__color__ = glm::vec4(color, ut::Clamp(Linear(-1.5f, 0.5f, 1.5f, 0.05, rinput.world_pos.y), 0.0f, 1.0f));
        // output.__color__ = glm::vec4(color, 0.3f);
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ka = 0.2f;
    float kd = 0.8f;
};

