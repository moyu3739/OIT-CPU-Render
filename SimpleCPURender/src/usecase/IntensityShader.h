#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"

#undef max
#undef min

using namespace oit;


class IntensityVertexShader: public VertexShader{
public:
    struct Input: public VertexShaderInput {
        glm::vec3 model_pos; // vertex position in model space
        glm::vec3 model_normal; // vertex normal in model space
        glm::vec2 texcoord; // texture coordinate
    };

    struct Output: public VertexShaderOutput {
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec2 texcoord; // texture coordinate
    };

public:
    IntensityVertexShader() {}

    ~IntensityVertexShader() {}

    virtual VertexShaderInput* MakeInput() const {
        return new Input;
    }

    virtual VertexShaderOutput* MakeOutput() const {
        return new Output;
    }

    virtual void DestroyInput(VertexShaderInput* input) const {
        delete reinterpret_cast<Input*>(input);
    }

    virtual void DestroyOutput(VertexShaderOutput* output) const {
        delete reinterpret_cast<Output*>(output);
    }

    virtual void Call(const VertexShaderInput& input, VertexShaderOutput& output) {
        const Input& rinput = reinterpret_cast<const Input&>(input);
        Output& routput = reinterpret_cast<Output&>(output);

        routput.world_pos = glm::vec3(model * glm::vec4(rinput.model_pos, 1.0f));
        routput.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model))) * rinput.model_normal);
        routput.texcoord = rinput.texcoord;
        routput.__position__ = projection * view * model * glm::vec4(rinput.model_pos, 1.0f);
    }

public:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class IntensityFragmentShader: public FragmentShader{
public:
    struct Input: public FragmentShaderInput {
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec2 texcoord; // texture coordinate
    };

    struct Output: public FragmentShaderOutput {};

public:
    IntensityFragmentShader() {}

    ~IntensityFragmentShader() {}

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
        const auto& rv0 = reinterpret_cast<const IntensityVertexShader::Output&>(v0);
        const auto& rv1 = reinterpret_cast<const IntensityVertexShader::Output&>(v1);
        const auto& rv2 = reinterpret_cast<const IntensityVertexShader::Output&>(v2);
        Input& rfs_input = reinterpret_cast<Input&>(fs_input);

        rfs_input.world_pos = InterpolateAttr(rv0.world_pos, rv1.world_pos, rv2.world_pos, barycentric);
        rfs_input.world_normal = InterpolateAttr(rv0.world_normal, rv1.world_normal, rv2.world_normal, barycentric);
        rfs_input.texcoord = InterpolateAttr(rv0.texcoord, rv1.texcoord, rv2.texcoord, barycentric);
    }

    virtual void Call(const FragmentShaderInput& input, FragmentShaderOutput& output) {
        const Input& rinput = reinterpret_cast<const Input&>(input);

        // obj_color = texture->Sample(rinput.texcoord);
        glm::vec3 frag_color;
        if (texture != nullptr) frag_color = glm::vec3(texture->Sample(rinput.texcoord));
        else frag_color = obj_color;
        glm::vec3 ambient = ka * frag_color;
        glm::vec3 light_dir = glm::normalize(light_pos - rinput.world_pos);
        // glm::vec3 diffuse = (kd * ut::Max(glm::dot(rinput.world_normal, light_dir), 0.0f)) * frag_color * light_color;
        glm::vec3 diffuse = (kd * glm::abs(glm::dot(rinput.world_normal, light_dir))) * frag_color * light_color;
        glm::vec3 color = ambient + diffuse;

        // output_wrapper.__color__ = glm::vec4(color, ut::Clamp(rinput.world_pos.y / 10.0f + 0.7f, 0.0f, 1.0f));
        output.__color__ = glm::vec4(color, 0.3f);
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color = glm::vec3(1.0f);
    float ka = 0.2f;
    float kd = 0.8f;
    glm::vec3 obj_color = glm::vec3(1.0f);
    Texture* texture = nullptr;
};

