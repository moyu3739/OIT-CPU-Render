#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "Texture.h"


class AnimeStyleVertexShader: public VertexShader{
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
    AnimeStyleVertexShader() {}

    ~AnimeStyleVertexShader() {}

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


class AnimeStyleFragmentShader: public FragmentShader{
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

    glm::vec3 HSV2RGB(const glm::vec3& hsv) {
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
        return HSV2RGB(hsv);
    }

    Output Call(const Input& input){
        glm::vec4 obj_color = texture->Sample(input.texcoord);
        glm::vec3 light_dir = glm::normalize(light_pos - input.world_pos);
        float dot = glm::dot(input.world_normal, light_dir);

        glm::vec3 color = GetDiffuse(kd * glm::vec3(obj_color) * light_color, dot);
        return Output{glm::vec4(color, 0.3f)};
        // return Output{glm::vec4(color, Clamp(input.world_pos.y / 10.0f + 0.5f, 0.0f, 1.0f))};
    }

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    Input Interpolate(
            const AnimeStyleVertexShader::Output& v1,
            const AnimeStyleVertexShader::Output& v2,
            const AnimeStyleVertexShader::Output& v3,
            const glm::vec3& barycentric){
        Input fs_input;
        fs_input.world_pos = InterpolateAttr(v1.world_pos, v2.world_pos, v3.world_pos, barycentric);
        fs_input.world_normal = InterpolateAttr(v1.world_normal, v2.world_normal, v3.world_normal, barycentric);
        fs_input.texcoord = InterpolateAttr(v1.texcoord, v2.texcoord, v3.texcoord, barycentric);
        return fs_input;
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

