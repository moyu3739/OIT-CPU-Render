#pragma once

#include <vector>
#include "utility.h"
#include "Primitive.h"
#include "Pipeline.h"
#include "FrameBuffer.h"
#include "FrameBufferManager.h"


class PipelineManager {
public:
    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    PipelineManager(int render_thread_num, int blend_thread_num)
    : render_thread_num(render_thread_num), blend_thread_num(blend_thread_num) {}

    ~PipelineManager() {
        for (PipelineBase* pipeline: opa_pipelines) delete pipeline;
        for (PipelineBase* pipeline: tra_pipelines) delete pipeline;
    }

    // create a pipeline
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  shared pointer to the created pipeline
    template <class VS, class FS>
    Pipeline<VS, FS>* CreatePipeline(bool is_transparent) {
        Pipeline<VS, FS>* pipeline = new Pipeline<VS, FS>;
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with shaders
    // @param[in] is_transparent  whether the pipeline is transparent
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @return  shared pointer to the created pipeline
    template <class VS, class FS>
    Pipeline<VS, FS>* CreatePipeline(VS* vertex_shader, FS* fragment_shader, bool is_transparent) {
        Pipeline<VS, FS>* pipeline = new Pipeline<VS, FS>(vertex_shader, fragment_shader);
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
    template <class VS, class FS>
    Pipeline<VS, FS>* CreatePipeline(const std::vector<typename VS::Input>& vertex_buffer, 
                                     VS* vertex_shader, FS* fragment_shader, bool use_oit) {
        Pipeline<VS, FS>* pipeline = new Pipeline<VS, FS>(vertex_shader, fragment_shader);
        pipeline->BoundVertexBuffer(vertex_buffer); // bind vertex buffer
        if (use_oit) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // render all pipelines, and blend the frame buffer
    // @param[in] frame_buffer  frame buffer
    void Render(FrameBuffer* frame_buffer) {
        for (PipelineBase* pipeline: opa_pipelines) pipeline->Render(frame_buffer, render_thread_num, false);
        for (PipelineBase* pipeline: tra_pipelines) pipeline->Render(frame_buffer, render_thread_num, true);
        frame_buffer->Blend(blend_thread_num);
    }

private:
    const int render_thread_num;
    const int blend_thread_num;
    std::vector<PipelineBase*> opa_pipelines; // opaque pipelines
    std::vector<PipelineBase*> tra_pipelines; // transparent pipelines
};

