#pragma once

#include <vector>
#include "utility.h"
#include "Primitive.h"
#include "Pipeline.h"
#include "FrameBuffer.h"


class PipelineManager {
public:
    PipelineManager(int width, int height, BufferVerticalOrder bvo)
        : frame_buffer(width, height, bvo) {}

    ~PipelineManager() {
        for (PipelineBase* pipeline: opa_pipelines) CheckDel(pipeline);
        for (PipelineBase* pipeline: tra_pipelines) CheckDel(pipeline);
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
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  shared pointer to the created pipeline
    template <class VS, class FS>
    Pipeline<VS, FS>* CreatePipeline(const std::vector<typename VS::Input>& vertex_buffer, 
                                     VS* vertex_shader, FS* fragment_shader, bool is_transparent) {
        Pipeline<VS, FS>* pipeline = new Pipeline<VS, FS>(vertex_shader, fragment_shader);
        pipeline->BoundVertexBuffer(vertex_buffer); // bind vertex buffer
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // render all pipelines
    void Render() {
        for (PipelineBase* pipeline: opa_pipelines) pipeline->Render(frame_buffer);
        for (PipelineBase* pipeline: tra_pipelines) pipeline->Render(frame_buffer);
    }

    const FrameBuffer& GetFrameBuffer() const {
        return frame_buffer;
    }

    // clear the frame buffer
    void ClearFrameBuffer(){
        frame_buffer.Clear();
    }

private:
    int width;
    int height;
    std::vector<PipelineBase*> opa_pipelines; // opaque pipelines
    std::vector<PipelineBase*> tra_pipelines; // transparent pipelines
    FrameBuffer frame_buffer; // a `Fragment` for each pixel
};

