#pragma once

#include <glm/glm.hpp>


class VertexShader{
public:
    struct Input{
    };

    struct Output{
        glm::vec4 __position__; // vertex position in clip space
    };

public:
    VertexShader() {}

    virtual ~VertexShader() {}

    virtual Output Call(const Input& input) {
        return Output{};
    }

};


class FragmentShader{
public:
    using Input = VertexShader::Output;

    struct Output{
        glm::vec4 __color__; // fragment color
    };

public:
    FragmentShader() {}

    virtual ~FragmentShader() {}

    // interpolate a vertex attribute
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    template <typename T>
    static T InterpolateAttr(const T& v1, const T& v2, const T& v3, const glm::vec3& barycentric){
        return (barycentric.x * v1 + barycentric.y * v2 + barycentric.z * v3)
                / (barycentric.x + barycentric.y + barycentric.z);
    }

    // interpolate vertex attributes
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    virtual Input Interpolate(
            const VertexShader::Output& v1,
            const VertexShader::Output& v2,
            const VertexShader::Output& v3,
            const glm::vec3& barycentric) {
        return Input{};
    }

    virtual Output Call(const Input& input) {
        return Output{};
    }

};
