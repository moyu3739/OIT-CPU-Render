#pragma once

#include <vector>
#include <stdexcept>
#include "utility.h"
#include "Primitive.h"
#include "Shader.h"
#include "TriangleTraversal.h"
#include "Pipeline.h"
#include "FrameBuffer.h"


namespace oit {

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
        // for (Pipeline* pipeline: opa_pipelines) pipeline->Render(frame_buffer, false);
        // for (Pipeline* pipeline: tra_pipelines) pipeline->Render(frame_buffer, true);
        RenderPipelinesCounter<false>(opa_pipelines, frame_buffer);
        RenderPipelinesCounter<true>(tra_pipelines, frame_buffer);
        frame_buffer->Blend(blend_thread_num);
    }

    // render pipelines to the given frame buffer.
    // It is optimized for situation of multiple pipelines in the manager, using inter-pipeline parallelism.
    // @note This function employs a dynamic work-stealing parallel pattern using an atomic counter.
    //       The 64-bit counter combines pipeline index (high 32 bits) and triangle index (low 32 bits),
    //       allowing threads to dynamically fetch and process triangles, ensuring good load balancing
    //       even with varying triangle processing times. When multiple threads are available, triangles
    //       across all pipelines are distributed among threads; otherwise, pipelines are processed sequentially.
    template <bool use_oit = false>
    void RenderPipelinesCounter(const std::vector<Pipeline*>& pipelines, FrameBuffer* frame_buffer) {
        if (pipelines.empty()) return;

        // check length of vertex buffer
        for (auto* pipeline: pipelines) {
            if (pipeline->vertex_buffer->size() % 3 != 0)
                throw std::runtime_error("Vertex buffer size is not a multiple of 3");
        }
        
        // if single thread, do it directly
        if (render_thread_num == 1) {
            for (auto* pipeline: pipelines)
                pipeline->RenderSliceProcess<use_oit>(0, pipeline->vertex_buffer->size(), frame_buffer, 0);
            return;
        }

        // counter is a 64-bit integer,
        // upper 32 bits are the pipeline index, lower 32 bits are the triangle index
        std::atomic<unsigned long long> counter(0);
        std::vector<std::thread> threads;
        for (int i = 0; i < render_thread_num; i++){
            threads.emplace_back(&PipelineManager::RenderPipelinesCounterProcess<use_oit>, pipelines, &counter, frame_buffer, i);
        }

        for (std::thread& thread: threads) thread.join();
    }

private:
    template <bool use_oit = false>
    static void RenderPipelinesCounterProcess(
        const std::vector<Pipeline*>& pipelines, std::atomic<unsigned long long>* counter, 
        FrameBuffer* frame_buffer, int thread_id)
    {
        if (pipelines.empty()) return;

        while (true){
            // counter is a 64-bit integer,
            // upper 32 bits are the pipeline index, lower 32 bits are the triangle index
            unsigned long long pos = counter->fetch_add(3, std::memory_order_relaxed);
            int idx_p = pos >> 32;
            int idx_t = pos & 0xffffffffLL;

            // if the pipeline still has a triangle, render the current triangle
            if (idx_t < pipelines[idx_p]->vertex_buffer->size()) {
                pipelines[idx_p]->RenderTriangle<use_oit>(idx_t, frame_buffer, thread_id);
                continue;
            }

            // if all pipelines finished, break
            if (idx_p == pipelines.size() - 1) {
                break;
            }

            // If the current thread happens to finish all triangles in the pipeline and the next pipieline exists,
            // it takes responsibility to set the counter to the next pipeline.
            if (idx_t == pipelines[idx_p]->vertex_buffer->size()) {
                unsigned long long new_pos = static_cast<unsigned long long>(idx_p + 1) << 32;
                counter->store(new_pos, std::memory_order_relaxed);
            }
        }
    }

private:
    const int render_thread_num;
    const int blend_thread_num;
    std::vector<Pipeline*> opa_pipelines; // opaque pipelines
    std::vector<Pipeline*> tra_pipelines; // transparent pipelines
};

} // namespace oit
