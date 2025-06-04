#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "utility.h"


namespace oit {

struct VertexShaderInput {
protected:
    VertexShaderInput() {}
};

struct VertexShaderOutput {
protected:
    VertexShaderOutput() {}
public:
    glm::vec4 __position__; // vertex position in clip space
};

struct FragmentShaderInput {
protected:
    FragmentShaderInput() {}
};

struct FragmentShaderOutput {
protected:
    FragmentShaderOutput() {}
public:
    glm::vec4 __color__; // fragment color
};

using VertexBuffer = std::vector<VertexShaderInput*>;


class VertexShader {
public:
    VertexShader() {}

    virtual ~VertexShader() {}

    virtual VertexShaderInput* MakeInput() const = 0;

    virtual VertexShaderOutput* MakeOutput() const = 0;

    virtual void DestroyInput(VertexShaderInput* input) const = 0;

    virtual void DestroyOutput(VertexShaderOutput* output) const = 0;

    virtual void Call(const VertexShaderInput& input, VertexShaderOutput& output) = 0;
};


class FragmentShader {
public:
    FragmentShader() {}

    virtual ~FragmentShader() {}

    // interpolate a vertex attribute
    // @param[in] v1, v2, v3  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    template <typename T>
    static T InterpolateAttr(const T& v0, const T& v1, const T& v2, const glm::vec3& barycentric){
        return (barycentric.x * v0 + barycentric.y * v1 + barycentric.z * v2)
                / (barycentric.x + barycentric.y + barycentric.z);
    }

    virtual FragmentShaderInput* MakeInput() const = 0;

    virtual FragmentShaderOutput* MakeOutput() const = 0;

    virtual void DestroyInput(FragmentShaderInput* input) const = 0;

    virtual void DestroyOutput(FragmentShaderOutput* output) const = 0;

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
    ) = 0;

    virtual void Call(const FragmentShaderInput& input, FragmentShaderOutput& output) = 0;
};


// `ShaderIO` is a wrapper for vertex shader and fragment shader IO.
// It contains the input and output data of the vertex shader and fragment shader.
class ShaderIO {
public:
    ShaderIO() {
        SetNULL();
    }

    ShaderIO(const VertexShader* vshader, const FragmentShader* fshader)
    : vshader(vshader), fshader(fshader) {
        Make();
    }

    ShaderIO(const ShaderIO&) = delete;
    ShaderIO& operator=(const ShaderIO&) = delete;

    ~ShaderIO() {
        Destroy();
    }

    void BoundShader(const VertexShader* vshader, const FragmentShader* fshader) {
        Destroy();
        this->vshader = vshader;
        this->fshader = fshader;
        Make();
    }

    void SetNULL() {
        for (int i = 0; i < VS_IO_NUM; i++) {
            vs_input[i] = nullptr;
            vs_output[i] = nullptr;
        }
        fs_input = nullptr;
        fs_output = nullptr;
    }

    void Make() {
        for (int i = 0; i < VS_IO_NUM; i++) {
            vs_input[i] = vshader->MakeInput();
            vs_output[i] = vshader->MakeOutput();
        }
        fs_input = fshader->MakeInput();
        fs_output = fshader->MakeOutput();
    }

    void Destroy() {
        for (int i = 0; i < VS_IO_NUM; i++) {
            if (vs_input[i] != nullptr) vshader->DestroyInput(vs_input[i]);
            if (vs_output[i] != nullptr) vshader->DestroyOutput(vs_output[i]);
        }
        if (fs_input != nullptr) fshader->DestroyInput(fs_input);
        if (fs_output != nullptr) fshader->DestroyOutput(fs_output);
        SetNULL();
    }

private:
    constexpr static int VS_IO_NUM = 3;
    const VertexShader* vshader;
    const FragmentShader* fshader;

public:
    VertexShaderInput* vs_input[VS_IO_NUM];
    VertexShaderOutput* vs_output[VS_IO_NUM];
    FragmentShaderInput* fs_input;
    FragmentShaderOutput* fs_output;
};


// `ShaderIOGroup` manages a group of `ShaderIO` objects.
// It is used to store the input and output data of the vertex shader and fragment shader for each thread.
class ShaderIOGroup {
public:
    ShaderIOGroup(int n) {
        shader_io_list.resize(n);
        for (int i = 0; i < n; i++)
            shader_io_list[i] = new ShaderIO();
    }

    ShaderIOGroup(int n, const VertexShader* vertex_shader, const FragmentShader* fragment_shader) {
        shader_io_list.resize(n);
        for (int i = 0; i < n; i++)
            shader_io_list[i] = new ShaderIO(vertex_shader, fragment_shader);
    }

    ~ShaderIOGroup() {
        for (ShaderIO* shader_io: shader_io_list) delete shader_io;
    }

    ShaderIOGroup(const ShaderIOGroup&) = delete;
    ShaderIOGroup& operator=(const ShaderIOGroup&) = delete;

    ShaderIO* GetAt(int idx) const {
        return shader_io_list[idx];
    }

    void BoundShader(const VertexShader* vertex_shader, const FragmentShader* fragment_shader) {
        for (ShaderIO* shader_io: shader_io_list)
            shader_io->BoundShader(vertex_shader, fragment_shader);
    }

private:
    std::vector<ShaderIO*> shader_io_list;
};

} // namespace oit
