#pragma once

#include <vector>
#include "utility.h"
#include "Primitive.h"
#include "Pipeline.h"
#include "FrameBuffer.h"


class PipelineManager {
public:
    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    PipelineManager(int render_thread_num, int blend_thread_num)
    : render_thread_num(render_thread_num), blend_thread_num(blend_thread_num) {}

    ~PipelineManager() {
        for (Pipeline* pipeline: opa_pipelines) delete pipeline;
        for (Pipeline* pipeline: tra_pipelines) delete pipeline;
    }

    // create an empty pipeline without any shaders or vertex buffer
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  shared pointer to the created pipeline
    Pipeline* CreatePipeline(bool is_transparent) {
        Pipeline* pipeline = new Pipeline(render_thread_num);
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with shaders
    // @param[in] is_transparent  whether the pipeline is transparent
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @return  shared pointer to the created pipeline
    Pipeline* CreatePipeline(VertexShader* vertex_shader, FragmentShader* fragment_shader, bool is_transparent) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, render_thread_num);
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with vertex buffer and shaders
    // @param[in] vertex_buffer  vertex buffer
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored in this pipeline
    // @return  shared pointer to the created pipeline
    Pipeline* CreatePipeline(const std::vector<VertexShader::InputWrapper>& vertex_buffer, 
            VertexShader* vertex_shader, FragmentShader* fragment_shader, bool use_oit) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, render_thread_num);
        pipeline->BoundVertexBuffer(vertex_buffer); // bind vertex buffer
        if (use_oit) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // render all pipelines, and blend the frame buffer
    // @param[in] frame_buffer  frame buffer
    void Render(FrameBuffer* frame_buffer) {
        for (Pipeline* pipeline: opa_pipelines) pipeline->Render(frame_buffer, false);
        for (Pipeline* pipeline: tra_pipelines) pipeline->Render(frame_buffer, true);
        frame_buffer->Blend(blend_thread_num);
    }

private:
    const int render_thread_num;
    const int blend_thread_num;
    std::vector<Pipeline*> opa_pipelines; // opaque pipelines
    std::vector<Pipeline*> tra_pipelines; // transparent pipelines
};

