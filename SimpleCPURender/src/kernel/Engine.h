#pragma once

#include <mutex>
#include <thread>
#include <cassert>
#include <glm/glm.hpp>
#include "PipelineManager.h"
#include "FrameBufferManager.h"
#include "Displayer.h"


// maintain pipeline manager, frame buffer manager and displayer
class Engine {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] parallel_level  level of parallelism (0 - 4)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    Engine(
        int width, int height, int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
        int parallel_level = 1, bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        pipeline_manager_exclusive(true),
        framebuffer_manager_exclusive(true),
        displayer_exclusive(true),
        parallel_level(parallel_level)
    {
        pipeline_manager = new PipelineManager(render_thread_num, blend_thread_num);

        switch(parallel_level) {
            case 0:
                frame_buffer_manager = new SingleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                displayer = new SingleBufferDisplayer;
                break;
            case 1:
            case 2:
                frame_buffer_manager = new DoubleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                displayer = new SingleBufferDisplayer;
                break;
            case 3:
                frame_buffer_manager = new TripleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                displayer = new SingleBufferDisplayer;
                break;
            case 4:
                frame_buffer_manager = new TripleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                displayer = new DoubleBufferDisplayer;
                break;
            default:
                assert(false);
                break;
        }
    }

    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] displayer  displayer to show the frame buffer
    // @param[in] parallel_level  level of parallelism (0 - 4)
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    Engine(
        int width, int height, int render_thread_num, int blend_thread_num, Displayer* displayer,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
        int parallel_level = 1, bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        pipeline_manager_exclusive(true),
        framebuffer_manager_exclusive(true),
        displayer_exclusive(false),
        parallel_level(parallel_level)
    {
        this->displayer = displayer;
        pipeline_manager = new PipelineManager(render_thread_num, blend_thread_num);

        switch(parallel_level) {
            case 0:
                frame_buffer_manager = new SingleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            case 1:
            case 2:
                frame_buffer_manager = new DoubleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            case 3:
                frame_buffer_manager = new TripleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            case 4:
                frame_buffer_manager = new TripleFrameBufferManager(width, height, render_thread_num, bg_color, bg_depth,
                    enable_oit, use_backward_pplist, backward_blend_alpha_threshold);
                break;
            default:
                assert(false);
                break;
        }
    }

    Engine(
        PipelineManager* pipeline_manager,
        FrameBufferManager* frame_buffer_manager,
        Displayer* displayer,
        int parallel_level = 1
    ):
        pipeline_manager_exclusive(false),
        framebuffer_manager_exclusive(false),
        displayer_exclusive(false),
        parallel_level(parallel_level),
        pipeline_manager(pipeline_manager),
        frame_buffer_manager(frame_buffer_manager),
        displayer(displayer)
    {}

    ~Engine() {
        if (pipeline_manager_exclusive) delete pipeline_manager;
        if (framebuffer_manager_exclusive) delete frame_buffer_manager;
        if (displayer_exclusive) delete displayer;
    }

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    PipelineManager* GetPipelineManager() const {
        return pipeline_manager;
    }

    FrameBufferManager* GetFrameBufferManager() const {
        return frame_buffer_manager;
    }

    Displayer* GetDisplayer() const {
        return displayer;
    }

    // serialized do render and show
    // @param[in] delay  delay time for showing
    void RenderSerial(int delay = 1) {
        // clear buffer
        FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer(); // get back buffer
        back_frame_buffer->Clear();
        // render
        pipeline_manager->Render(back_frame_buffer);
        frame_buffer_manager->RotateBuffer();
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer(); // get front buffer
        displayer->LoadFromFrameBuffer(front_frame_buffer);
        // show
        displayer->Show(delay);
    }

    // parallelly do render and show
    // @param[in] delay  delay time for showing
    void RenderParallel(int delay = 1) {
        switch(parallel_level) {
            case 0: RenderParallelLevel0(delay); break;
            case 1: RenderParallelLevel1(delay); break;
            case 2: RenderParallelLevel2(delay); break;
            case 3: RenderParallelLevel3(delay); break;
            case 4: RenderParallelLevel4(delay); break;
            default: assert(false); break;
        }
    }

    // actually serialized do render
    void RenderParallelLevel0(int delay = 1) {
        RenderSerial(delay);
    }

    void RenderParallelLevel1(int delay = 1) {
        assert(frame_buffer_manager->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
        displayer->LoadFromFrameBuffer(front_frame_buffer);
        // show
        displayer->Show(delay);
        // clear buffer
        front_frame_buffer->Clear();
        // wait render thread
        thread_render.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
    }

    void RenderParallelLevel2(int delay = 1) {
        assert(frame_buffer_manager->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // load buffer
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
        displayer->LoadFromFrameBuffer(front_frame_buffer);
        // start to clear
        float duration_clear;
        std::thread thread_clear([&](){
            FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
            front_frame_buffer->Clear();
        });
        // show
        displayer->Show(delay);
        // wait render thread and clear thread
        thread_render.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
    }

    void RenderParallelLevel3(int delay = 1) {
        assert(frame_buffer_manager->GetBufferNumber() >= 3);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // start to clear
        float duration_clear;
        std::thread thread_clear([&](){
            FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
            front_frame_buffer->Clear();
        });
        // load buffer
        FrameBuffer* load_frame_buffer = frame_buffer_manager->GetBufferAt(1);
        displayer->LoadFromFrameBuffer(load_frame_buffer);
        // show
        displayer->Show(delay);
        // wait render thread and clear thread
        thread_render.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
    }

    void RenderParallelLevel4(int delay = 1) {
        assert(frame_buffer_manager->GetBufferNumber() >= 3);
        assert(displayer->GetBufferNumber() >= 2);

        // start to render
        std::thread thread_render([&](){
            FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer();
            pipeline_manager->Render(back_frame_buffer);
        });
        // start to load buffer
        float duration_load;
        std::thread thread_load([&](){
            FrameBuffer* load_frame_buffer = frame_buffer_manager->GetBufferAt(1);
            displayer->LoadFromFrameBuffer(load_frame_buffer);
        });
        // start to clear
        float duration_clear;
        std::thread thread_clear([&](){
            FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer();
            front_frame_buffer->Clear();
        });
        // show
        displayer->Show();
        // wait render thread, load thread and clear thread
        thread_render.join();
        thread_load.join();
        thread_clear.join();
        // rotate buffer
        frame_buffer_manager->RotateBuffer();
        displayer->RotateBuffer();
    }

private:
    const bool pipeline_manager_exclusive;
    const bool framebuffer_manager_exclusive;
    const bool displayer_exclusive;
    const int parallel_level;
    PipelineManager* pipeline_manager;
    FrameBufferManager* frame_buffer_manager;
    Displayer* displayer;
};

