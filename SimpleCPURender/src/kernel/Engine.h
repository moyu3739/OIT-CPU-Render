#pragma once

#include <thread>
#include <stdexcept>
#include <cassert>
#include <glm/glm.hpp>
#include "utility.h"
#include "Shader.h"
#include "TriangleTraversal.h"
#include "PipelineManager.h"
#include "FrameBufferManager.h"
#include "Frontend.h"
#include "Timer.h"


namespace oit {

// maintain pipeline manager, frame buffer manager and frontend
class Engine {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    // @param[in] pipeline_level  level of pipelining (0 - 4)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when backward blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    Engine(
        int width, int height, int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY, int pipeline_level = 1,
        bool enable_oit = false, bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        pipeline_level(pipeline_level),
        render_thread_num(render_thread_num),
        blend_thread_num(blend_thread_num)
    {
        ResetPipelines();
        ResetFrameBuffers(width, height, bg_color, bg_depth,
            enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
        ResetFrontend();
    }

    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    // @param[in] frontend  frontend to use
    // @param[in] pipeline_level  level of pipelining (0 - 4)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when backward blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    Engine(
        int width, int height, int render_thread_num, int blend_thread_num, Frontend* frontend,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY, int pipeline_level = 1,
        bool enable_oit = false, bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        pipeline_level(pipeline_level),
        render_thread_num(render_thread_num),
        blend_thread_num(blend_thread_num)
    {
        ResetPipelines();
        ResetFrameBuffers(width, height, bg_color, bg_depth,
            enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
        ResetFrontend(frontend);
    }

    // @param[in] render_thread_num  number of threads to render the pipelines
    // @param[in] blend_thread_num  number of threads to blend the frame buffer
    // @param[in] pipeline_level  level of pipelining (0 - 4)
    Engine(
        PipelineManager* pipeline_manager,
        FrameBufferManager* frame_buffer_manager,
        Frontend* frontend,
        int render_thread_num, int blend_thread_num, int pipeline_level = 1
    ):
        render_thread_num(render_thread_num),
        blend_thread_num(blend_thread_num),
        pipeline_level(pipeline_level),
        pipeline_manager(pipeline_manager),
        frame_buffer_manager(frame_buffer_manager),
        frontend(frontend)
    {}

    ~Engine() {
        if (pipeline_manager_exclusive) delete pipeline_manager;
        if (framebuffer_manager_exclusive) delete frame_buffer_manager;
        if (frontend_exclusive) delete frontend;
    }

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    static int FrameBufferNumberNeeded(int pipeline_level) {
        switch(pipeline_level) {
            case 0:
                return 1;
            case 1:
            case 2:
                return 2;
            case 3:
            case 4:
                return 3;
            default:
                throw std::invalid_argument("Invalid parallel level");
        }
    }

    static int FrontendBufferNumberNeeded(int pipeline_level) {
        switch(pipeline_level) {
            case 0:
            case 1:
            case 2:
            case 3:
                return 0;
            case 4:
                return 2;
            default:
                throw std::invalid_argument("Invalid parallel level");
        }
    }

    // reset pipelines with a new one
    // @return  pointer to the new pipeline manager
    PipelineManager* ResetPipelines() {
        if (pipeline_manager_exclusive) delete pipeline_manager;
        pipeline_manager = new PipelineManager(render_thread_num, blend_thread_num);
        pipeline_manager_exclusive = true;
        return pipeline_manager;
    }

    // reset frame buffers with new ones
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when backward blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    // @return  pointer to the new frame buffer manager
    FrameBufferManager* ResetFrameBuffers(
        int width, int height,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
        bool enable_oit = false, bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ) {
        if (framebuffer_manager_exclusive) delete frame_buffer_manager;

        switch(pipeline_level) {
            case 0:
                frame_buffer_manager = new SingleFrameBufferManager(width, height, render_thread_num,
                    bg_color, bg_depth, enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            case 1:
            case 2:
                frame_buffer_manager = new DoubleFrameBufferManager(width, height, render_thread_num,
                    bg_color, bg_depth, enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            case 3:
            case 4:
                frame_buffer_manager = new TripleFrameBufferManager(width, height, render_thread_num,
                    bg_color, bg_depth, enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            default:
                assert(false);
        }

        framebuffer_manager_exclusive = true;
        return frame_buffer_manager;
    }

    // reset frontend with the given one
    // @return  `frontent`
    Frontend* ResetFrontend(Frontend* frontend) {
        if (frontend->GetBufferNumber() < FrontendBufferNumberNeeded(pipeline_level)) {
            throw std::invalid_argument("Frontend does not have enough buffers");
        }

        if (frontend_exclusive) delete this->frontend;
        this->frontend = frontend;
        frontend_exclusive = false;
        return this->frontend;
    }

    // reset frontend with a new one (build-in)
    // @return  pointer to the new frontend
    Frontend* ResetFrontend() {
        if (frontend_exclusive) delete frontend;

        switch(pipeline_level) {
            case 0:
            case 1:
            case 2:
            case 3:
                frontend = new OpencvSingleBufferDisplayer; break;
            case 4:
                frontend = new OpencvDoubleBufferDisplayer; break;
            default:
                assert(false);
        }

        frontend_exclusive = true;
        return frontend;
    }

    PipelineManager* GetPipelineManager() const {
        return pipeline_manager;
    }

    FrameBufferManager* GetFrameBufferManager() const {
        return frame_buffer_manager;
    }

    Frontend* GetFrontend() const {
        return frontend;
    }

    // create an empty pipeline without any shaders or vertex buffer
    // @param[in] is_transparent  whether the pipeline is transparent
    // @return  pointer to the created pipeline
    // @note  The pipeline manager would manage the created pipeline instance,
    //        so user does not need to delete it manually.
    Pipeline* CreatePipeline(bool is_transparent) {
        return pipeline_manager->CreatePipeline(is_transparent);
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
        return pipeline_manager->CreatePipeline(vertex_shader, fragment_shader, tt_type, is_transparent);
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
        return pipeline_manager->CreatePipeline(vertex_shader, fragment_shader, triangle_traversal, is_transparent);
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
        return pipeline_manager->CreatePipeline(vertex_buffer, vertex_shader, fragment_shader, tt_type, is_transparent);
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
        return pipeline_manager->CreatePipeline(vertex_buffer, vertex_shader, fragment_shader, triangle_traversal, is_transparent);
    }

    // serialized do render and output
    // @param[in] info  info for render
    void SerialRender(unsigned long long load_info = 0, unsigned long long output_info = 0) {
        Timer tm;
        tm.StartTimer();

        // clear buffer
        FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer(); // get back buffer
        back_frame_buffer->Clear();
        // render
        pipeline_manager->Render(back_frame_buffer);
        frame_buffer_manager->RotateBuffer();
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer(); // get front buffer
        frontend->Load(front_frame_buffer, load_info);

        float t = tm.ReadTimer();
        printf("Render time: %.1f ms\n", t * 1000);

        // output
        frontend->Output(output_info);
    }

    // parallelly do render and output
    // @param[in] info  info for render
    void PipelinedRender(unsigned long long load_info = 0, unsigned long long output_info = 0) {
        switch(pipeline_level) {
            case 0: PipelinedRenderLevel0(load_info, output_info); break;
            case 1: PipelinedRenderLevel1(load_info, output_info); break;
            case 2: PipelinedRenderLevel2(load_info, output_info); break;
            case 3: PipelinedRenderLevel3(load_info, output_info); break;
            case 4: PipelinedRenderLevel4(load_info, output_info); break;
            default: assert(false); break;
        }
    }

    // actually serialized do render
    void PipelinedRenderLevel0(unsigned long long load_info, unsigned long long output_info) {
        SerialRender(load_info, output_info);
    }

    void PipelinedRenderLevel1(unsigned long long load_info, unsigned long long output_info) {
        assert(frame_buffer_manager->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
        frontend->Load(front_frame_buffer, output_info);
        // output
        frontend->Output(output_info);
        // clear buffer
        front_frame_buffer->Clear();
        // wait render thread
        thread_render.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
        frontend->RotateBuffer();
    }

    void PipelinedRenderLevel2(unsigned long long load_info, unsigned long long output_info) {
        assert(frame_buffer_manager->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
        frontend->Load(front_frame_buffer, output_info);
        // start to clear
        std::thread thread_clear([&](){
            front_frame_buffer->Clear();
        });
        // output
        frontend->Output(output_info);
        // wait render thread and clear thread
        thread_render.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
        frontend->RotateBuffer();
    }

    void PipelinedRenderLevel3(unsigned long long load_info, unsigned long long output_info) {
        assert(frame_buffer_manager->GetBufferNumber() >= 3);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // start to clear
        std::thread thread_clear([&](){
            FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
            front_frame_buffer->Clear();
        });
        // load buffer
        FrameBuffer* load_frame_buffer = frame_buffer_manager->GetBufferAt(1);
        frontend->Load(load_frame_buffer, output_info);
        // output
        frontend->Output(output_info);
        // wait render thread and clear thread
        thread_render.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
        frontend->RotateBuffer();
    }

    void PipelinedRenderLevel4(unsigned long long load_info, unsigned long long output_info) {
        assert(frame_buffer_manager->GetBufferNumber() >= 3);
        assert(frontend->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // start to load buffer
        std::thread thread_load([&](){
            FrameBuffer* load_frame_buffer = frame_buffer_manager->GetBufferAt(1);
            frontend->Load(load_frame_buffer, output_info);
        });
        // start to clear
        std::thread thread_clear([&](){
            FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
            front_frame_buffer->Clear();
        });
        // output
        frontend->Output(output_info);
        // wait render thread, load thread and clear thread
        thread_render.join();
        thread_load.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
        frontend->RotateBuffer();
    }

private:
    const int pipeline_level;
    const int render_thread_num;
    const int blend_thread_num;

    bool pipeline_manager_exclusive = false;
    bool framebuffer_manager_exclusive = false;
    bool frontend_exclusive = false;

    PipelineManager* pipeline_manager;
    FrameBufferManager* frame_buffer_manager;
    Frontend* frontend;
};

} // namespace oit

