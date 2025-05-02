#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"

#undef max
#undef min


class AnimeStyleVertexShader: public VertexShader{
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
    AnimeStyleVertexShader() {}

    ~AnimeStyleVertexShader() {}

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


class AnimeStyleFragmentShader: public FragmentShader{
public:
    struct Input: public FragmentShaderInput {
        glm::vec3 world_pos; // vertex position in world space
        glm::vec3 world_normal; // vertex normal in world space
        glm::vec2 texcoord; // texture coordinate
    };

    struct Output: public FragmentShaderOutput {};

public:
    AnimeStyleFragmentShader() {}

    ~AnimeStyleFragmentShader() {}

    glm::vec3 RGB2HSV(const glm::vec3& rgb) {
        float cmax = std::max(rgb.r, std::max(rgb.g, rgb.b));
        float cmin = std::min(rgb.r, std::min(rgb.g, rgb.b));
        float delta = cmax - cmin;
        glm::vec3 hsv;

        // hue
        if (delta == 0.0f) hsv.x = 0.0f;
        else if (cmax == rgb.r) hsv.x = 60.0f * fmod((rgb.g - rgb.b) / delta, 6.0f);
        else if (cmax == rgb.g) hsv.x = 60.0f * ((rgb.b - rgb.r) / delta + 2.0f);
        else hsv.x = 60.0f * ((rgb.r - rgb.g) / delta + 4.0f);
        // saturation
        if (cmax == 0.0f) hsv.y = 0.0f;
        else hsv.y = delta / cmax;
        // value
        hsv.z = cmax;

        return hsv;
    }

    glm::vec3 HSv1RGB(const glm::vec3& hsv) {
        float c = hsv.z * hsv.y;
        float x = c * (1.0f - fabs(fmod(hsv.x / 60.0f, 2.0f) - 1.0f));
        float m = hsv.z - c;
        glm::vec3 rgb;

        if (hsv.x < 60.0f) rgb = glm::vec3(c, x, 0.0f);
        else if (hsv.x < 120.0f) rgb = glm::vec3(x, c, 0.0f);
        else if (hsv.x < 180.0f) rgb = glm::vec3(0.0f, c, x);
        else if (hsv.x < 240.0f) rgb = glm::vec3(0.0f, x, c);
        else if (hsv.x < 300.0f) rgb = glm::vec3(x, 0.0f, c);
        else rgb = glm::vec3(c, 0.0f, x);

        return rgb + glm::vec3(m);
    }

    float CalcValueCoef(float x) {
        if (x <= 0.0f) return ka;
        else if (x >= kv) return 1.0f;
        return (1.0f - ka) * x + ka; // kv in [0, 1]
    }

    glm::vec3 GetDiffuse(const glm::vec3& color, float dot) {
        glm::vec3 hsv = RGB2HSV(color);
        float diff = CalcValueCoef(dot);
        hsv.z *= diff;
        hsv.y += ks * (1 - hsv.y) * (1.0f - diff);
        return HSv1RGB(hsv);
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
    // @param[in] v0, v1, v2  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    // @param[out] fs_input  fragment shader input
    virtual void Interpolate(
        const VertexShaderOutput& v0,
        const VertexShaderOutput& v1,
        const VertexShaderOutput& v2,
        const glm::vec3& barycentric,
        FragmentShaderInput& fs_input
    ) override {
        const auto& rv0 = reinterpret_cast<const AnimeStyleVertexShader::Output&>(v0);
        const auto& rv1 = reinterpret_cast<const AnimeStyleVertexShader::Output&>(v1);
        const auto& rv2 = reinterpret_cast<const AnimeStyleVertexShader::Output&>(v2);
        Input& rfs_input = reinterpret_cast<Input&>(fs_input);
        
        rfs_input.world_pos = InterpolateAttr(rv0.world_pos, rv1.world_pos, rv2.world_pos, barycentric);
        rfs_input.world_normal = InterpolateAttr(rv0.world_normal, rv1.world_normal, rv2.world_normal, barycentric);
        rfs_input.texcoord = InterpolateAttr(rv0.texcoord, rv1.texcoord, rv2.texcoord, barycentric);
    }

    virtual void Call(const FragmentShaderInput& input, FragmentShaderOutput& output) override {
        const Input& rinput = reinterpret_cast<const Input&>(input);

        glm::vec4 obj_color = texture->Sample(rinput.texcoord);
        glm::vec3 light_dir = glm::normalize(light_pos - rinput.world_pos);
        float dot = glm::dot(rinput.world_normal, light_dir);
        glm::vec3 color = GetDiffuse(kd * glm::vec3(obj_color) * light_color, dot);

        // output.__color__ = glm::vec4(color, 0.3f);
        output.__color__ = glm::vec4(color, ut::Clamp(rinput.world_pos.y / 10.0f + 0.5f, 0.0f, 1.0f));
    }

public:
    glm::vec3 light_pos;
    glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    float ka = 0.6f; // ambient coefficient
    float kv = 0.35f; // value decreasing coefficient (control the range of shadow)
    float ks = 0.4f; // saturation increasing coefficient (control the saturation of shadow)
    float kd = 1.0f; // diffuse coefficient
    Texture* texture = nullptr;
};

