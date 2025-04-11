#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "utility.h"


class VertexShader{
public:
    struct InputWrapper {
        void* __data__;
    };

    struct OutputWrapper {
        glm::vec4 __position__; // vertex position in clip space
        void* __data__;
    };

public:
    VertexShader() {}

    virtual ~VertexShader() {}

    virtual void* MakeInput() const = 0;

    virtual void* MakeOutput() const = 0;

    virtual void DestroyInput(void* input) const = 0;

    virtual void DestroyOutput(void* output) const = 0;

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) = 0;

};


class FragmentShader{
public:
    struct InputWrapper {
        void* __data__;
    };

    struct OutputWrapper {
        glm::vec4 __color__; // fragment color
        void* __data__;
    };

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

    // interpolate vertex attributes
    // @param[in] v0, v1, v2  vertex attributes of the triangle
    // @param[in] barycentric  barycentric coordinates of the pixel
    // @param[out] fs_input  fragment shader input
    virtual void Interpolate(
            const VertexShader::OutputWrapper& v0,
            const VertexShader::OutputWrapper& v1,
            const VertexShader::OutputWrapper& v2,
            const glm::vec3& barycentric,
            InputWrapper& fs_input) = 0;

    virtual void* MakeInput() const = 0;

    virtual void* MakeOutput() const = 0;

    virtual void DestroyInput(void* input) const = 0;

    virtual void DestroyOutput(void* output) const = 0;

    virtual void Call(const InputWrapper& input_wrapper, OutputWrapper& output_wrapper) = 0;

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
            vs_input[i].__data__ = nullptr;
            vs_output[i].__data__ = nullptr;
        }
        fs_input.__data__ = nullptr;
        fs_output.__data__ = nullptr;
    }

    void Make() {
        for (int i = 0; i < VS_IO_NUM; i++) {
            vs_input[i].__data__ = vshader->MakeInput();
            vs_output[i].__data__ = vshader->MakeOutput();
        }
        fs_input.__data__ = fshader->MakeInput();
        fs_output.__data__ = fshader->MakeOutput();
    }

    void Destroy() {
        for (int i = 0; i < VS_IO_NUM; i++) {
            if (vs_input[i].__data__ != nullptr) vshader->DestroyInput(vs_input[i].__data__);
            if (vs_output[i].__data__ != nullptr) vshader->DestroyOutput(vs_output[i].__data__);
        }
        if (fs_input.__data__ != nullptr) fshader->DestroyInput(fs_input.__data__);
        if (fs_output.__data__ != nullptr) fshader->DestroyOutput(fs_output.__data__);
        SetNULL();
    }

private:
    constexpr static int VS_IO_NUM = 3;
    const VertexShader* vshader;
    const FragmentShader* fshader;

public:
    VertexShader::InputWrapper vs_input[VS_IO_NUM];
    VertexShader::OutputWrapper vs_output[VS_IO_NUM];
    FragmentShader::InputWrapper fs_input;
    FragmentShader::OutputWrapper fs_output;
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

    ShaderIOGroup(int n, VertexShader* vertex_shader, FragmentShader* fragment_shader) {
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
