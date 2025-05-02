#include <iostream>
#include <thread>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Application.h"
#include "Anime.h"
#include "Intensity.h"
#include "CornellBox.h"

#include "Pipeline.h"
#include "Frontend.h"
#include "FrameBuffer.h"
#include "FrameBufferManager.h"
#include "Timer.h"
#include "Engine.h"


// void PipelinedRenderCustom0(Anime& app, float time_limit) {
//     const int render_thread_num = 16;
//     const int blend_thread_num  = 16;

//     auto pipeline = app.InitPipeline();
//     FrameBuffer frame_buffer(app.width, app.height, render_thread_num, glm::vec3(1.0f), INFINITE, true);
//     Displayer displayer;

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_clear_time = 0.0f;
//     float total_render_time = 0.0f;
//     float totla_blend_time = 0.0f;
//     float total_load_time = 0.0f;
//     float total_show_time = 0.0f;
//     float total_process_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

//         float start_process = tm.ReadTimer();

//         // render
//         float start_render = tm.ReadTimer();
//         pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
//         app.fshaders[0]->texture = app.models["Babala hair"].texture.get();
//         pipeline->Render(&frame_buffer, render_thread_num, true);

//         pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
//         app.fshaders[0]->texture = app.models["Babala body"].texture.get();
//         pipeline->Render(&frame_buffer, render_thread_num, true);

//         pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
//         app.fshaders[0]->texture = app.models["Babala face"].texture.get();
//         pipeline->Render(&frame_buffer, render_thread_num, true);

//         float duration_render = tm.ReadTimer() - start_render;
//         total_render_time += duration_render;

//         // blend
//         float start_blend = tm.ReadTimer();
//         frame_buffer.Blend(blend_thread_num);
//         float duration_blend = tm.ReadTimer() - start_blend;
//         totla_blend_time += duration_blend;

//         // load buffer
//         float start_load = tm.ReadTimer();
//         displayer.Load(&frame_buffer);
//         float duration_load = tm.ReadTimer() - start_load;
//         total_load_time += duration_load;
//         // show
//         float start_show = tm.ReadTimer();
//         displayer.Show();
//         float duration_show = tm.ReadTimer() - start_show;
//         total_show_time += duration_show;
//         // to clear
//         float start_clear = tm.ReadTimer();
//         frame_buffer.Clear();
//         float duration_clear = tm.ReadTimer() - start_clear;
//         total_clear_time += duration_clear;
        
//         // calculate process time
//         float duration_process = tm.ReadTimer() - start_process;
//         total_process_time += duration_process;
//         // calculate FPS
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t%.1f ms\n", duration_render * 1000);
//         printf("Blend time:\t%.1f ms\n", duration_blend * 1000);
//         printf("Load time:\t%.1f ms\n", duration_load * 1000);
//         printf("Show time:\t%.1f ms\n", duration_show * 1000);
//         printf("Clear time:\t%.1f ms\n", duration_clear * 1000);
//         printf("Process time:\t%.1f ms\n", duration_process * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average blend time:\t%.1f ms\n", totla_blend_time * 1000 / frame_count);
//         printf("Average load time:\t%.1f ms\n", total_load_time * 1000 / frame_count);
//         printf("Average show time:\t%.1f ms\n", total_show_time * 1000 / frame_count);
//         printf("Average clear time:\t%.1f ms\n", total_clear_time * 1000 / frame_count);
//         printf("Average process time:\t%.1f ms\n", total_process_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }

// void PipelinedRenderCustom1(Anime& app, float time_limit) {
//     const int render_thread_num = 16;
//     const int blend_thread_num  = 16;

//     auto pipeline = app.InitPipeline();
//     DoubleFrameBufferManager frame_buffer_manager(
//         app.width, app.height, render_thread_num, glm::vec3(1.0f), INFINITE, true);
//     Displayer displayer;

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_clear_time = 0.0f;
//     float total_render_time = 0.0f;
//     float totla_blend_time = 0.0f;
//     float total_load_time = 0.0f;
//     float total_show_time = 0.0f;
//     float total_process_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

//         float start_process = tm.ReadTimer();


//         // start to render and blend
//         float duration_render;
//         float duration_blend;
//         std::thread thread_render([&](){
//             FrameBuffer* back_buffer = frame_buffer_manager.GetBackBuffer();

//             // render
//             float start_render = tm.ReadTimer();
//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
//             app.fshaders[0]->texture = app.models["Babala hair"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
//             app.fshaders[0]->texture = app.models["Babala body"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
//             app.fshaders[0]->texture = app.models["Babala face"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             duration_render = tm.ReadTimer() - start_render;
//             total_render_time += duration_render;

//             // blend
//             float start_blend = tm.ReadTimer();
//             back_buffer->Blend(blend_thread_num);
//             duration_blend = tm.ReadTimer() - start_blend;
//             totla_blend_time += duration_blend;
//         });

//         // load buffer
//         FrameBuffer* front_buffer = frame_buffer_manager.GetFrontBuffer();
//         float start_load = tm.ReadTimer();
//         displayer.Load(front_buffer);
//         float duration_load = tm.ReadTimer() - start_load;
//         total_load_time += duration_load;
//         // show
//         float start_show = tm.ReadTimer();
//         displayer.Show();
//         float duration_show = tm.ReadTimer() - start_show;
//         total_show_time += duration_show;
//         // to clear
//         float start_clear = tm.ReadTimer();
//         front_buffer->Clear();
//         float duration_clear = tm.ReadTimer() - start_clear;
//         total_clear_time += duration_clear;

//         // wait
//         thread_render.join();

//         // swap buffer
//         frame_buffer_manager.SwapBuffer();
        
//         // calculate process time
//         float duration_process = tm.ReadTimer() - start_process;
//         total_process_time += duration_process;
//         // calculate FPS
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t%.1f ms\n", duration_render * 1000);
//         printf("Blend time:\t%.1f ms\n", duration_blend * 1000);
//         printf("Load time:\t%.1f ms\n", duration_load * 1000);
//         printf("Show time:\t%.1f ms\n", duration_show * 1000);
//         printf("Clear time:\t%.1f ms\n", duration_clear * 1000);
//         printf("Process time:\t%.1f ms\n", duration_process * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average blend time:\t%.1f ms\n", totla_blend_time * 1000 / frame_count);
//         printf("Average load time:\t%.1f ms\n", total_load_time * 1000 / frame_count);
//         printf("Average show time:\t%.1f ms\n", total_show_time * 1000 / frame_count);
//         printf("Average clear time:\t%.1f ms\n", total_clear_time * 1000 / frame_count);
//         printf("Average process time:\t%.1f ms\n", total_process_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }

// void PipelinedRenderCustom2(Anime& app, float time_limit) {
//     const int render_thread_num = 16;
//     const int blend_thread_num  = 16;

//     auto pipeline = app.InitPipeline();
//     DoubleFrameBufferManager frame_buffer_manager(
//         app.width, app.height, render_thread_num, glm::vec3(1.0f), INFINITE, true);
//     Displayer displayer;

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_clear_time = 0.0f;
//     float total_render_time = 0.0f;
//     float totla_blend_time = 0.0f;
//     float total_load_time = 0.0f;
//     float total_show_time = 0.0f;
//     float total_process_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

//         float start_process = tm.ReadTimer();


//         // start to render and blend
//         float duration_render;
//         float duration_blend;
//         std::thread thread_render([&](){
//             FrameBuffer* back_buffer = frame_buffer_manager.GetBackBuffer();

//             // render
//             float start_render = tm.ReadTimer();
//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
//             app.fshaders[0]->texture = app.models["Babala hair"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
//             app.fshaders[0]->texture = app.models["Babala body"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
//             app.fshaders[0]->texture = app.models["Babala face"].texture.get();
//             pipeline->Render(back_buffer, render_thread_num, true);

//             duration_render = tm.ReadTimer() - start_render;
//             total_render_time += duration_render;

//             // blend
//             float start_blend = tm.ReadTimer();
//             back_buffer->Blend(blend_thread_num);
//             duration_blend = tm.ReadTimer() - start_blend;
//             totla_blend_time += duration_blend;
//         });

//         // load buffer
//         FrameBuffer* front_buffer = frame_buffer_manager.GetFrontBuffer();
//         float start_load = tm.ReadTimer();
//         displayer.Load(front_buffer);
//         float duration_load = tm.ReadTimer() - start_load;
//         total_load_time += duration_load;

//         // start to clear
//         float duration_clear;
//         std::thread thread_clear([&](){
//             FrameBuffer* front_buffer = frame_buffer_manager.GetFrontBuffer();

//             float start_clear = tm.ReadTimer();
//             front_buffer->Clear();
//             duration_clear = tm.ReadTimer() - start_clear;
//             total_clear_time += duration_clear;
//         });

//         // show
//         float start_show = tm.ReadTimer();
//         displayer.Show();
//         float duration_show = tm.ReadTimer() - start_show;
//         total_show_time += duration_show;

//         // wait
//         thread_render.join();
//         thread_clear.join();

//         // swap buffer
//         frame_buffer_manager.SwapBuffer();
        
//         // calculate process time
//         float duration_process = tm.ReadTimer() - start_process;
//         total_process_time += duration_process;
//         // calculate FPS
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t%.1f ms\n", duration_render * 1000);
//         printf("Blend time:\t%.1f ms\n", duration_blend * 1000);
//         printf("Load time:\t%.1f ms\n", duration_load * 1000);
//         printf("Show time:\t%.1f ms\n", duration_show * 1000);
//         printf("Clear time:\t%.1f ms\n", duration_clear * 1000);
//         printf("Process time:\t%.1f ms\n", duration_process * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average blend time:\t%.1f ms\n", totla_blend_time * 1000 / frame_count);
//         printf("Average load time:\t%.1f ms\n", total_load_time * 1000 / frame_count);
//         printf("Average show time:\t%.1f ms\n", total_show_time * 1000 / frame_count);
//         printf("Average clear time:\t%.1f ms\n", total_clear_time * 1000 / frame_count);
//         printf("Average process time:\t%.1f ms\n", total_process_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }

// void PipelinedRenderCustom3(Anime& app, float time_limit) {
//     const int render_thread_num = 16;
//     const int blend_thread_num  = 16;

//     auto pipeline = app.InitPipeline();
//     TripleFrameBufferManager frame_buffer_manager(
//         app.width, app.height, render_thread_num, glm::vec3(1.0f), INFINITE, true);
//     Displayer displayer;

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_clear_time = 0.0f;
//     float total_render_time = 0.0f;
//     float totla_blend_time = 0.0f;
//     float total_load_time = 0.0f;
//     float total_show_time = 0.0f;
//     float total_process_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

//         float start_process = tm.ReadTimer();


//         // start to render and blend
//         float duration_render;
//         float duration_blend;
//         std::thread thread_render([&](){
//             FrameBuffer* render_buffer = frame_buffer_manager.GetBackBuffer();

//             // render
//             float start_render = tm.ReadTimer();
//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
//             app.fshaders[0]->texture = app.models["Babala hair"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
//             app.fshaders[0]->texture = app.models["Babala body"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
//             app.fshaders[0]->texture = app.models["Babala face"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             duration_render = tm.ReadTimer() - start_render;
//             total_render_time += duration_render;

//             // blend
//             float start_blend = tm.ReadTimer();
//             render_buffer->Blend(blend_thread_num);
//             duration_blend = tm.ReadTimer() - start_blend;
//             totla_blend_time += duration_blend;
//         });

//         // start to clear
//         float duration_clear;
//         std::thread thread_clear([&](){
//             FrameBuffer* load_buffer = frame_buffer_manager.GetFrontBuffer();

//             float start_clear = tm.ReadTimer();
//             load_buffer->Clear();
//             duration_clear = tm.ReadTimer() - start_clear;
//             total_clear_time += duration_clear;
//         });

//         // load buffer
//         FrameBuffer* load_buffer = frame_buffer_manager.GetMidBuffer();
//         float start_load = tm.ReadTimer();
//         displayer.Load(load_buffer);
//         float duration_load = tm.ReadTimer() - start_load;
//         total_load_time += duration_load;

//         // show
//         float start_show = tm.ReadTimer();
//         displayer.Show();
//         float duration_show = tm.ReadTimer() - start_show;
//         total_show_time += duration_show;

//         thread_render.join();
//         thread_clear.join();

//         // rotate buffer
//         frame_buffer_manager.RotateBuffer();
        
//         // calculate process time
//         float duration_process = tm.ReadTimer() - start_process;
//         total_process_time += duration_process;
//         // calculate FPS
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t%.1f ms\n", duration_render * 1000);
//         printf("Blend time:\t%.1f ms\n", duration_blend * 1000);
//         printf("Load time:\t%.1f ms\n", duration_load * 1000);
//         printf("Show time:\t%.1f ms\n", duration_show * 1000);
//         printf("Clear time:\t%.1f ms\n", duration_clear * 1000);
//         printf("Process time:\t%.1f ms\n", duration_process * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average blend time:\t%.1f ms\n", totla_blend_time * 1000 / frame_count);
//         printf("Average load time:\t%.1f ms\n", total_load_time * 1000 / frame_count);
//         printf("Average show time:\t%.1f ms\n", total_show_time * 1000 / frame_count);
//         printf("Average clear time:\t%.1f ms\n", total_clear_time * 1000 / frame_count);
//         printf("Average process time:\t%.1f ms\n", total_process_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }

// void PipelinedRenderCustom4(Anime& app, float time_limit) {
//     const int render_thread_num = 16;
//     const int blend_thread_num  = 16;

//     auto pipeline = app.InitPipeline();
//     TripleFrameBufferManager frame_buffer_manager(
//         app.width, app.height, render_thread_num, glm::vec3(1.0f), INFINITE, true);
//     DoubleBufferDisplayer displayer;

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_clear_time = 0.0f;
//     float total_render_time = 0.0f;
//     float totla_blend_time = 0.0f;
//     float total_load_time = 0.0f;
//     float total_show_time = 0.0f;
//     float total_process_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

//         float start_process = tm.ReadTimer();


//         // start to render and blend
//         float duration_render;
//         float duration_blend;
//         std::thread thread_render([&](){
//             FrameBuffer* render_buffer = frame_buffer_manager.GetBackBuffer();

//             // render
//             float start_render = tm.ReadTimer();
//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
//             app.fshaders[0]->texture = app.models["Babala hair"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
//             app.fshaders[0]->texture = app.models["Babala body"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
//             app.fshaders[0]->texture = app.models["Babala face"].texture.get();
//             pipeline->Render(render_buffer, render_thread_num, true);

//             duration_render = tm.ReadTimer() - start_render;
//             total_render_time += duration_render;

//             // blend
//             float start_blend = tm.ReadTimer();
//             render_buffer->Blend(blend_thread_num);
//             duration_blend = tm.ReadTimer() - start_blend;
//             totla_blend_time += duration_blend;
//         });

//         // start to load buffer
//         float duration_load;
//         std::thread thread_load([&](){
//             FrameBuffer* load_buffer = frame_buffer_manager.GetMidBuffer();

//             float start_load = tm.ReadTimer();
//             displayer.Load(load_buffer);
//             duration_load = tm.ReadTimer() - start_load;
//             total_load_time += duration_load;
//         });

//         // start to clear
//         float duration_clear;
//         std::thread thread_clear([&](){
//             FrameBuffer* load_buffer = frame_buffer_manager.GetFrontBuffer();

//             float start_clear = tm.ReadTimer();
//             load_buffer->Clear();
//             duration_clear = tm.ReadTimer() - start_clear;
//             total_clear_time += duration_clear;
//         });

//         // show
//         float start_show = tm.ReadTimer();
//         displayer.Show();
//         float duration_show = tm.ReadTimer() - start_show;
//         total_show_time += duration_show;

//         thread_render.join();
//         thread_load.join();
//         thread_clear.join();

//         // rotate buffer
//         frame_buffer_manager.RotateBuffer();
//         displayer.SwapBuffer();
        
//         // calculate process time
//         float duration_process = tm.ReadTimer() - start_process;
//         total_process_time += duration_process;
//         // calculate FPS
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t%.1f ms\n", duration_render * 1000);
//         printf("Blend time:\t%.1f ms\n", duration_blend * 1000);
//         printf("Load time:\t%.1f ms\n", duration_load * 1000);
//         printf("Show time:\t%.1f ms\n", duration_show * 1000);
//         printf("Clear time:\t%.1f ms\n", duration_clear * 1000);
//         printf("Process time:\t%.1f ms\n", duration_process * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average blend time:\t%.1f ms\n", totla_blend_time * 1000 / frame_count);
//         printf("Average load time:\t%.1f ms\n", total_load_time * 1000 / frame_count);
//         printf("Average show time:\t%.1f ms\n", total_show_time * 1000 / frame_count);
//         printf("Average clear time:\t%.1f ms\n", total_clear_time * 1000 / frame_count);
//         printf("Average process time:\t%.1f ms\n", total_process_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }

// void PipelinedRender(Application& app, float time_limit) {
//     auto engine = app.InitEngine(16, 16, glm::vec3(1.0f), INFINITE);

//     const float T = 3.0f;
//     const float Y = 1.0f;

//     Timer tm;
//     float total_render_time = 0.0f;
//     float total_show_time = 0.0f;

//     float last_frame = 0.0f;
//     int frame_count = 0;

//     tm.StartTimer();
//     while(1){
//         float t = tm.ReadTimer();
//         if (t > time_limit) break;

//         float r = 2 * 3.14159 * t / T;
//         float y = Y * sin(r);
//         app.UpdateTransform(app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f));

//         // render and show
//         float start_render = tm.ReadTimer();
//         engine->PipelineRenderAndShow(1);
//         float duration_render = tm.ReadTimer() - start_render;
//         total_render_time += duration_render;

//         // 计算帧率
//         frame_count++;
//         float now = tm.ReadTimer();
//         float duration_frame = now - last_frame;
//         last_frame = now;

//         printf("\033[2J\033[H"); // 清空缓冲区
//         printf("[%.1f]\n", t);
//         printf("Render time:\t %.1f ms\n", duration_render * 1000);
//         printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
//         printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
//         printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
//         printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
//         printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
//     }
// }



int main() {
    Anime app = Anime(800, 800);
    app.Run();

    // CornellBox app = CornellBox(800, 800, 200, true, 1919810);
    // app.Run();

    // Intensity app = Intensity(800, 800);
    // app.Run();

    return 0;
}