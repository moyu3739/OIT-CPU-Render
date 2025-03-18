#pragma once

#include <mutex>
#include <thread>
#include <glm/glm.hpp>
#include "PipelineManager.h"
#include "FrameBufferManager.h"
#include "Displayer.h"


// maintain pipeline manager, frame buffer manager and displayer
class Engine {
public:
    Engine(int width, int height, int render_thread_num, int blend_thread_num, bool enable_oit = false) {
        pipeline_manager = new PipelineManager(render_thread_num, blend_thread_num);
        frame_buffer_manager = new FrameBufferManager(width, height, render_thread_num, enable_oit);
        displayer = new Displayer();
        self_created = true;
    }

    Engine(PipelineManager* pipeline_manager, FrameBufferManager* frame_buffer_manager, Displayer* displayer) {
        this->pipeline_manager = pipeline_manager;
        this->frame_buffer_manager = frame_buffer_manager;
        this->displayer = displayer;
        self_created = false;
    }

    ~Engine() {
        if (self_created) {
            delete pipeline_manager;
            delete frame_buffer_manager;
            delete displayer;
        }
    }

    // render all pipelines to back frame buffer, and blend
    void Render() {
        FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer(); // get back buffer
        pipeline_manager->Render(back_frame_buffer); // render and blend
    }

    // load front frame buffer to displayer, then show, and then clear front buffer
    // @param[in] delay  delay time for showing
    // @param[in] bg_color  background color
    void Show(int delay = 1, const glm::vec3& bg_color = glm::vec3(0.0f)) {
        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer(); // get front buffer
        displayer->LoadFromFrameBuffer(front_frame_buffer); // load buffer
        displayer->Show(delay); // show
        front_frame_buffer->Clear(bg_color); // clear front buffer
    }

    // pipelined do render and show
    // @param[in] delay  delay time for showing
    // @param[in] bg_color  background color
    void PipelineRenderAndShow(int delay = 1, const glm::vec3& bg_color = glm::vec3(0.0f)) {
        // render
        std::thread render_thread(&Engine::Render, this);
        // show
        Show(delay, bg_color);
        render_thread.join();
        // swap front and back buffer
        frame_buffer_manager->SwapBuffer();
    }

    // serialized do render and show
    // @param[in] delay  delay time for showing
    // @param[in] bg_color  background color
    void RenderAndShow(int delay = 1, const glm::vec3& bg_color = glm::vec3(0.0f)) {
        FrameBuffer* back_frame_buffer = frame_buffer_manager->GetBackBuffer(); // get back buffer
        back_frame_buffer->Clear(bg_color); // clear front buffer
        pipeline_manager->Render(back_frame_buffer); // render and blend
        frame_buffer_manager->SwapBuffer();

        FrameBuffer* front_frame_buffer = frame_buffer_manager->GetFrontBuffer(); // get front buffer
        displayer->LoadFromFrameBuffer(front_frame_buffer); // load buffer
        displayer->Show(delay); // show
    }

public:
    bool self_created;
    PipelineManager* pipeline_manager;
    FrameBufferManager* frame_buffer_manager;
    Displayer* displayer;
};

