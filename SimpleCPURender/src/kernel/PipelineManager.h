#pragma once

#include <vector>
#include "utility.h"
#include "Primitive.h"
#include "Shader.h"
#include "TriangleTraversal.h"
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

    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    // create an empty pipeline without any shaders or vertex buffer
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  The pipeline manager would manage the created pipeline instance,
    //        so user does not need to delete it manually.
    Pipeline* CreatePipeline(bool is_transparent) {
        Pipeline* pipeline = new Pipeline(render_thread_num);
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with shaders
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @param[in] tt_type  build-in triangle traversal type to apply
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  - Instances of the shaders and the triangle traversal filter would not be copied here,
    //          which means user should pay attention to the lifetime of them.
    // @note  - The pipeline manager would manage the created pipeline instance,
    //          so user does not need to delete it manually.
    Pipeline* CreatePipeline(VertexShader* vertex_shader, FragmentShader* fragment_shader,
                             TriangleTraversalType tt_type, bool is_transparent) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, tt_type, render_thread_num);
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with shaders
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @param[in] triangle_traversal  triangle traversal filter
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  - Instances of the shaders and the triangle traversal filter would not be copied here,
    //          which means user should pay attention to the lifetime of them.
    // @note  - The pipeline manager would manage the created pipeline instance,
    //          so user does not need to delete it manually.
    Pipeline* CreatePipeline(VertexShader* vertex_shader, FragmentShader* fragment_shader,
                             TriangleTraversal* triangle_traversal, bool is_transparent) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, triangle_traversal, render_thread_num);
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with vertex buffer and shaders
    // @param[in] vertex_buffer  vertex buffer
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @param[in] tt_type  build-in triangle traversal type to apply
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  - Instances of the vertex buffer, the shaders and the triangle traversal filter would not be copied here,
    //          which means user should pay attention to the lifetime of them.
    // @note  - The pipeline manager would manage the created pipeline instance,
    //          so user does not need to delete it manually.
    Pipeline* CreatePipeline(const VertexBuffer& vertex_buffer, 
                             VertexShader* vertex_shader, FragmentShader* fragment_shader,
                             TriangleTraversalType tt_type, bool is_transparent) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, tt_type, render_thread_num);
        pipeline->BoundVertexBuffer(vertex_buffer); // bind vertex buffer
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
        else opa_pipelines.emplace_back(pipeline);
        return pipeline;
    }

    // create a pipeline with vertex buffer and shaders
    // @param[in] vertex_buffer  vertex buffer
    // @param[in] vertex_shader  vertex shader
    // @param[in] fragment_shader  fragment shader
    // @param[in] triangle_traversal  triangle traversal filter
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  - Instances of the vertex buffer, the shaders and the triangle traversal filter would not be copied here,
    //          which means user should pay attention to the lifetime of them.
    // @note  - The pipeline manager would manage the created pipeline instance,
    //          so user does not need to delete it manually.
    Pipeline* CreatePipeline(const VertexBuffer& vertex_buffer, 
                             VertexShader* vertex_shader, FragmentShader* fragment_shader,
                             TriangleTraversal* triangle_traversal, bool is_transparent) {
        Pipeline* pipeline = new Pipeline(vertex_shader, fragment_shader, triangle_traversal, render_thread_num);
        pipeline->BoundVertexBuffer(vertex_buffer); // bind vertex buffer
        if (is_transparent) tra_pipelines.emplace_back(pipeline);
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

