#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Application.h"
#include "Pipeline.h"
#include "Displayer.h"
#include "FrameBuffer.h"
#include "Timer.h"


void CircularRender(Application& app) {
    app.InitPipeline();

    const float T = 3.0f;
    const float Y = 1.0f;

    Timer tm;
    tm.StartTimer();
    float last_frame = 0.0f;
    int frame_count = 0;
    while(1){
        app.pipeline_manager.ClearFrameBuffer();

        float t = tm.ReadTimer();

        float r = 2 * 3.14159 * t / T;
        float y = Y * sin(r);
        // float s = 1.0f + 0.3f * sin(r);
        app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);


        // app.Render();

        float start_render = tm.ReadTimer();
        app.pipeline_manager.Render();
        float duration_render = tm.ReadTimer() - start_render;

        float start_load = tm.ReadTimer();
        app.displayer.LoadFromFrameBuffer(app.pipeline_manager.GetFrameBuffer());
        float duration_load = tm.ReadTimer() - start_load;

        float start_show = tm.ReadTimer();
        app.displayer.Show();
        float duration_show = tm.ReadTimer() - start_show;


        // 计算帧率
        frame_count++;
        float now = tm.ReadTimer();
        float duration_frame = now - last_frame;
        last_frame = now;

        system("cls"); // 清空缓冲区
        printf("Render:\t %.4f ms\n", duration_render * 1000);
        printf("Load buffer:\t%.4f ms\n", duration_load * 1000);
        printf("Show:\t%.4f ms\n", duration_show * 1000);
        printf("FPS:\t%.1f fps\n", 1.0f / duration_frame);
        printf("average FPS:\t%.1f fps\n", frame_count / now);
    }
}

void CircularRenderCustom(Application& app) {
    auto pipeline = app.InitPipelineCustom();
    FrameBuffer frame_buffer(app.width, app.height, TOP_DOWN, true);

    const float T = 3.0f;
    const float Y = 1.0f;

    Timer tm;
    tm.StartTimer();
    float last_frame = 0.0f;
    int frame_count = 0;
    while(1){
        frame_buffer.Clear(glm::vec3(1.0f));

        float t = tm.ReadTimer();

        float r = 2 * 3.14159 * t / T;
        float y = Y * sin(r);
        // float s = 1.0f + 0.3f * sin(r);
        app.vshaders[0]->model = app.GetModelTransform(glm::vec3(0.0f, y, 0.0f), r, 1.5f);

        // app.pipeline.fragment_shader->obj_color.r = 0.75f + 0.25f * sin(r);
        // app.pipeline.fragment_shader->obj_color.g = 0.75f + 0.25f * sin(r + 2.0f/3.0f * 3.14159f);
        // app.pipeline.fragment_shader->obj_color.b = 0.75f + 0.25f * sin(r - 2.0f/3.0f * 3.14159f);

        // app.pipeline.fragment_shader->light_pos.x = 100.0f * sin(4 * r);
        // app.pipeline.fragment_shader->light_pos.y = 100.f;
        // app.pipeline.fragment_shader->light_pos.z = 100.0f * cos(4 * r);


        float start_render = tm.ReadTimer();

        pipeline->BoundVertexBuffer(app.vertex_buffers["Babala hair"]);
        app.fshaders[0]->texture = app.models["Babala hair"].texture;
        pipeline->Render(&frame_buffer, true);

        pipeline->BoundVertexBuffer(app.vertex_buffers["Babala body"]);
        app.fshaders[0]->texture = app.models["Babala body"].texture;
        pipeline->Render(&frame_buffer, true);

        pipeline->BoundVertexBuffer(app.vertex_buffers["Babala face"]);
        app.fshaders[0]->texture = app.models["Babala face"].texture;
        pipeline->Render(&frame_buffer, true);

        frame_buffer.Blend();

        float duration_render = tm.ReadTimer() - start_render;


        float start_load = tm.ReadTimer();
        app.displayer.LoadFromFrameBuffer(&frame_buffer);
        float duration_load = tm.ReadTimer() - start_load;

        float start_show = tm.ReadTimer();
        app.displayer.Show();
        float duration_show = tm.ReadTimer() - start_show;

        // 计算帧率
        frame_count++;
        float now = tm.ReadTimer();
        float duration_frame = now - last_frame;
        last_frame = now;

        system("cls"); // 清空缓冲区
        printf("Render:\t %.4f ms\n", duration_render * 1000);
        printf("Load buffer:\t%.4f ms\n", duration_load * 1000);
        printf("Show:\t%.4f ms\n", duration_show * 1000);
        printf("FPS:\t%.1f fps\n", 1.0f / duration_frame);
        printf("average FPS:\t%.1f fps\n", frame_count / now);
    }
}

void RenderFrame(Application& app) {
    app.InitPipeline();
    app.Render(0);
}


int main(){
    Application app(500, 700);

    // app.LoadModel("Ankila", "asset/obj/Ankila.obj", "asset/texture/Ankila.png");
    app.LoadModel("Babala hair", "asset/obj/Babala/hair.obj", "asset/texture/Babala/hair.png");
    app.LoadModel("Babala face", "asset/obj/Babala/face.obj", "asset/texture/Babala/face.png");
    app.LoadModel("Babala body", "asset/obj/Babala/body.obj", "asset/texture/Babala/body.png");
    // app.ResetNormalAll();
    app.LoadVertexBuffer();

    // RenderFrame(app);
    // CircularRender(app);
    CircularRenderCustom(app);
    
    
    return 0;
}