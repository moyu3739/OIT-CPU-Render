#include <iostream>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include "RenderPipeline.h"
#include "Displayer.h"
#include "Timer.h"


int main(){
    Displayer displayer;
    RenderPipeline pipeline(1920, 1080);

    pipeline.LoadObj("asset/obj/bunny.obj");
    // pipeline.ResetNormal();
    pipeline.InitShader();

    const float T = 3.0f;
    const float Y = 1.0f;

    Timer tm;
    tm.StartTimer();
    float last_frame = 0.0f;
    while(1){
        pipeline.ClearFrameBuffer();

        float t = tm.ReadTimer();

        float r = 2 * 3.14159 * t / T;
        float y = Y * sin(r);
        float s = 1.0f + 0.3f * sin(r);
        pipeline.SetModelTransform(glm::vec3(0.0f, y, 0.0f), r, s);

        pipeline.fragment_shader.obj_color.r = 0.75f + 0.25f * sin(r);
        pipeline.fragment_shader.obj_color.g = 0.75f + 0.25f * sin(r + 2.0f/3.0f * 3.14159f);
        pipeline.fragment_shader.obj_color.b = 0.75f + 0.25f * sin(r - 2.0f/3.0f * 3.14159f);

        pipeline.fragment_shader.light_pos.x = 100.0f * sin(4 * r);
        pipeline.fragment_shader.light_pos.y = 100.f;
        pipeline.fragment_shader.light_pos.z = 100.0f * cos(4 * r);

        float start_render = tm.ReadTimer();
        pipeline.Render();
        float duration_render = tm.ReadTimer() - start_render;

        float start_load = tm.ReadTimer();
        displayer.LoadFromFrameBuffer(pipeline.framebuffer);
        float duration_load = tm.ReadTimer() - start_load;

        displayer.Show();

        // 计算帧率
        float now = tm.ReadTimer();
        float duration_frame = now - last_frame;
        last_frame = now;

        //system("cls"); // 清空缓冲区
        printf("Render:\t %.4f ms\n", duration_render * 1000);
        printf("Load buffer:\t%.4f ms\n", duration_load * 1000);
        printf("FPS: %.1f fps\n", 1.0f / duration_frame);
    }

    return 0;
}