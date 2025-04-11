#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"
#include "Shape.h"


class ShapeVertexShader: public VertexShader{
public:
    using Input = ShapeVertex; // vertex input type

    struct Output{
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec3 color; // texture coordinate
        glm::vec4 __position__; // vertex position in clip space
    };

public:
    ShapeVertexShader() {}

    ~ShapeVertexShader() {}

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

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) override {
        const Input& input = *reinterpret_cast<const Input*>(input_wrapper.__data__);
        Output& output = *reinterpret_cast<Output*>(output_wrapper.__data__);

        glm::mat4 overall_model = *global_model * model;
        output.world_pos = glm::vec3(overall_model * glm::vec4(input.position, 1.0f));
        output.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(overall_model))) * input.normal);
        output.color = input.color;
        output_wrapper.__position__ = projection * view * overall_model * glm::vec4(input.position, 1.0f);
    }

public:
    std::shared_ptr<glm::mat4> global_model;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};


class ShapeFragmentShader: public FragmentShader{
public:
    struct Input{
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec3 color; // texture coordinate
    };

    struct Output{
        glm::vec4 __color__; // fragment color
    };

public:
    ShapeFragmentShader() {}

    ~ShapeFragmentShader() {}

    float Linear(float x1, float y1, float x2, float y2, float x) {
        if (x1 == x2) return y1;
        return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
    }

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
        const auto& v1 = *reinterpret_cast<const ShapeVertexShader::Output*>(v0_wrapper.__data__);
        const auto& v2 = *reinterpret_cast<const ShapeVertexShader::Output*>(v1_wrapper.__data__);
        const auto& v3 = *reinterpret_cast<const ShapeVertexShader::Output*>(v2_wrapper.__data__);
        Input& fs_input = *reinterpret_cast<Input*>(fs_input_wrapper.__data__);
        fs_input.world_pos = InterpolateAttr(v1.world_pos, v2.world_pos, v3.world_pos, barycentric);
        fs_input.world_normal = InterpolateAttr(v1.world_normal, v2.world_normal, v3.world_normal, barycentric);
        fs_input.color = InterpolateAttr(v1.color, v2.color, v3.color, barycentric);
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

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) override {
        const Input& input = *reinterpret_cast<const Input*>(input_wrapper.__data__);

        glm::vec3 ambient = ka * input.color;
        glm::vec3 light_dir = glm::normalize(light_pos - input.world_pos);
        // glm::vec3 diffuse = (kd * glm::max(glm::dot(input.world_normal, light_dir), 0.0f)) * input.color * light_color;
        glm::vec3 diffuse = (kd * glm::abs(glm::dot(input.world_normal, light_dir))) * input.color * light_color;
        glm::vec3 color = ambient + diffuse;

        output_wrapper.__color__ = glm::vec4(color, Clamp(Linear(-1.5f, 0.5f, 1.5f, 0.05, input.world_pos.y), 0.0f, 1.0f));
        // output_wrapper.__color__ = glm::vec4(color, 0.3f);
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ka = 0.2f;
    float kd = 0.8f;
};

