#pragma once

#include <glm/glm.hpp>
#include "Shader.h"

#undef max
#undef min


class PhongVertexShader : public VertexShader {
public:
    struct Input : public VertexShaderInput {
        Input() {}
        Input(const glm::vec3& model_pos, const glm::vec3& model_normal):
            model_pos(model_pos), model_normal(model_normal) {}
        glm::vec3 model_pos;    // vertex position in model space
        glm::vec3 model_normal; // vertex normal in model space
    };

    struct Output : public VertexShaderOutput {
        glm::vec3 world_pos;    // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec3 view_pos;     // vertex position in view space
    };

public:
    PhongVertexShader() {}

    ~PhongVertexShader() {}

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

        // Transform vertex to world space
        routput.world_pos = glm::vec3(model * glm::vec4(rinput.model_pos, 1.0f));
        // Transform normal to world space
        routput.world_normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(model))) * rinput.model_normal);
        // Transform vertex to view space
        routput.view_pos = glm::vec3(view * glm::vec4(routput.world_pos, 1.0f));
        // Transform vertex to clip space
        routput.__position__ = projection * view * model * glm::vec4(rinput.model_pos, 1.0f);
    }

public:
    glm::mat4 model;      // model transformation matrix
    glm::mat4 view;       // view transformation matrix
    glm::mat4 projection; // projection transformation matrix
};


class PhongFragmentShader : public FragmentShader {
public:
    struct Input : public FragmentShaderInput {
        glm::vec3 world_pos;    // position in world space
        glm::vec3 world_normal; // normal in world space
        glm::vec3 view_pos;     // position in view space
    };

    struct Output : public FragmentShaderOutput {};

public:
    PhongFragmentShader() {}

    ~PhongFragmentShader() {}

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

    // Interpolate vertex attributes
    virtual void Interpolate(
        const VertexShaderOutput& v0,
        const VertexShaderOutput& v1,
        const VertexShaderOutput& v2,
        const glm::vec3& barycentric,
        FragmentShaderInput& fs_input
    ) override {
        const auto& rv0 = reinterpret_cast<const PhongVertexShader::Output&>(v0);
        const auto& rv1 = reinterpret_cast<const PhongVertexShader::Output&>(v1);
        const auto& rv2 = reinterpret_cast<const PhongVertexShader::Output&>(v2);
        Input& rfs_input = reinterpret_cast<Input&>(fs_input);

        rfs_input.world_pos = InterpolateAttr(rv0.world_pos, rv1.world_pos, rv2.world_pos, barycentric);
        rfs_input.world_normal = glm::normalize(InterpolateAttr(rv0.world_normal, rv1.world_normal, rv2.world_normal, barycentric));
        rfs_input.view_pos = InterpolateAttr(rv0.view_pos, rv1.view_pos, rv2.view_pos, barycentric);
    }

    virtual void Call(const FragmentShaderInput& input, FragmentShaderOutput& output) {
        const Input& rinput = reinterpret_cast<const Input&>(input);

        // Use uniform color
        glm::vec3 frag_color = obj_color;
        // Calculate normal and light direction (normalized)
        glm::vec3 normal = glm::normalize(rinput.world_normal);
        glm::vec3 light_dir = glm::normalize(light_pos - rinput.world_pos);
        // Calculate view direction
        glm::vec3 view_dir = glm::normalize(view_pos - rinput.world_pos);
        // Calculate reflection direction
        glm::vec3 reflect_dir = glm::reflect(-light_dir, normal);

        // 1. Ambient light
        glm::vec3 ambient = ka * frag_color;
        // 2. Diffuse light
        float diff = glm::max(glm::dot(normal, light_dir), 0.0f);
        glm::vec3 diffuse = kd * diff * frag_color * light_color;
        // 3. Specular light
        float spec = glm::pow(glm::max(glm::dot(view_dir, reflect_dir), 0.0f), shininess);
        glm::vec3 specular = ks * spec * light_color;

        // Combine all lighting components
        glm::vec3 color = ambient + diffuse + specular;
        // Set final color and alpha
        output.__color__ = glm::vec4(color, alpha);
    }

public:
    glm::vec3 light_pos;                     // light position
    glm::vec3 view_pos;                      // camera position
    glm::vec3 light_color = glm::vec3(1.0f); // light color
    glm::vec3 obj_color = glm::vec3(1.0f);   // object color
    float ka = 0.2f;                         // ambient coefficient
    float kd = 0.4f;                         // diffuse coefficient
    float ks = 0.4f;                         // specular coefficient
    float shininess = 32.0f;                 // specular exponent
    float alpha = 1.0f;                      // transparency
};

