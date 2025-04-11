#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"


class IntensityVertexShader: public VertexShader{
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
    };

public:
    IntensityVertexShader() {}

    ~IntensityVertexShader() {}

    virtual void* MakeInput() const {
        return new Input;
    }

    virtual void* MakeOutput() const {
        return new Output;
    }

    virtual void DestroyInput(void* input) const {
        delete reinterpret_cast<Input*>(input);
    }

    virtual void DestroyOutput(void* output) const {
        delete reinterpret_cast<Output*>(output);
    }

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) {
        const Input& input = *reinterpret_cast<const Input*>(input_wrapper.__data__);
        Output& output = *reinterpret_cast<Output*>(output_wrapper.__data__);

        output.world_pos = glm::vec3(model * glm::vec4(input.model_pos, 1.0f));
        output.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model))) * input.model_normal);
        output.texcoord = input.texcoord;
        output_wrapper.__position__ = projection * view * model * glm::vec4(input.model_pos, 1.0f);
    }

public:
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class IntensityFragmentShader: public FragmentShader{
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
    IntensityFragmentShader() {}

    ~IntensityFragmentShader() {}

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    // @param[out] fs_input  fragment shader input
    virtual void Interpolate(
            const VertexShader::OutputWrapper& v0_wrapper,
            const VertexShader::OutputWrapper& v1_wrapper,
            const VertexShader::OutputWrapper& v2_wrapper,
            const glm::vec3& barycentric,
            InputWrapper& fs_input_wrapper) override {
        const auto& v1 = *reinterpret_cast<const IntensityVertexShader::Output*>(v0_wrapper.__data__);
        const auto& v2 = *reinterpret_cast<const IntensityVertexShader::Output*>(v1_wrapper.__data__);
        const auto& v3 = *reinterpret_cast<const IntensityVertexShader::Output*>(v2_wrapper.__data__);
        Input& fs_input = *reinterpret_cast<Input*>(fs_input_wrapper.__data__);
        fs_input.world_pos = InterpolateAttr(v1.world_pos, v2.world_pos, v3.world_pos, barycentric);
        fs_input.world_normal = InterpolateAttr(v1.world_normal, v2.world_normal, v3.world_normal, barycentric);
        fs_input.texcoord = InterpolateAttr(v1.texcoord, v2.texcoord, v3.texcoord, barycentric);
    }

    virtual void* MakeInput() const override {
        return new Input;
    }

    virtual void* MakeOutput() const override {
        return new Output;
    }

    virtual void DestroyInput(void* input) const override {
        delete reinterpret_cast<Input*>(input);
    }

    virtual void DestroyOutput(void* output) const override {
        delete reinterpret_cast<Output*>(output);
    }

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) {
        const Input& input = *reinterpret_cast<const Input*>(input_wrapper.__data__);

        // obj_color = texture->Sample(input.texcoord);
        glm::vec3 frag_color;
        if (texture != nullptr) frag_color = glm::vec3(texture->Sample(input.texcoord));
        else frag_color = obj_color;
        glm::vec3 ambient = ka * frag_color;
        glm::vec3 light_dir = glm::normalize(light_pos - input.world_pos);
        // glm::vec3 diffuse = (kd * glm::max(glm::dot(input.world_normal, light_dir), 0.0f)) * frag_color * light_color;
        glm::vec3 diffuse = (kd * glm::abs(glm::dot(input.world_normal, light_dir))) * frag_color * light_color;
        glm::vec3 color = ambient + diffuse;

        // output_wrapper.__color__ = glm::vec4(color, Clamp(input.world_pos.y / 10.0f + 0.7f, 0.0f, 1.0f));
        output_wrapper.__color__ = glm::vec4(color, 0.3f);
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ka = 0.2f;
    float kd = 0.8f;
    glm::vec3 obj_color = glm::vec3(1.0f, 1.0f, 1.0f);
    Texture* texture = nullptr;
};

