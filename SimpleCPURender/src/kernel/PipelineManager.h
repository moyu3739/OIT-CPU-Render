#pragma once

#include <vector>
#include "utility.h"
#include "Primitive.h"
#include "Pipeline.h"
#include "FrameBuffer.h"


class PipelineManager {
public:
    PipelineManager(int width, int height, BufferVerticalOrder bvo) {
        frame_buffer = new FrameBuffer(width, height, bvo, true);
    }

    ~PipelineManager() {
        for (PipelineBase* pipeline: opa_pipelines) delete pipeline;
        for (PipelineBase* pipeline: tra_pipelines) delete pipeline;
        delete frame_buffer;
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

    // render all pipelines
    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    void Render(int render_thread_num = 1, int blend_thread_num = 1) {
        for (PipelineBase* pipeline: opa_pipelines) pipeline->Render(frame_buffer, false, render_thread_num);
        for (PipelineBase* pipeline: tra_pipelines) pipeline->Render(frame_buffer, true, render_thread_num);
        frame_buffer->Blend(blend_thread_num);
    }

    const FrameBuffer* GetFrameBuffer() const {
        return frame_buffer;
    }

    // clear the frame buffer
    // @param[in] bg_color  background color
    void ClearFrameBuffer(const glm::vec3 bg_color = glm::vec3(0.0f)) {
        frame_buffer->Clear(bg_color);
    }

private:
    std::vector<PipelineBase*> opa_pipelines; // opaque pipelines
    std::vector<PipelineBase*> tra_pipelines; // transparent pipelines
    FrameBuffer* frame_buffer; // a `Fragment` for each pixel
};

