#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Application.h"
#include "utility.h"
#include "Primitive.h"
#include "IntensityShader.h"
#include "Texture.h"
#include "ImageTexture.h"
#include "Pipeline.h"
#include "PipelineManager.h"
#include "Engine.h"


class Intensity: public Application {
    using MyVertexShader   = IntensityVertexShader;
    using MyFragmentShader = IntensityFragmentShader;

public:
    Intensity(int width, int height): Application(width, height) {}

    virtual void Run() override {
        LoadModel("dragon", "asset/obj/dragon.obj");
        // LoadModel("earth", "asset/obj/sphere.obj", "asset/texture/miscellaneous/earthmap.jpg");
        LoadVertexBuffer();

        RenderAnimation(1000.0f);
        // RenderFrame();
    }

    virtual void LoadVertexBuffer() override;

    virtual std::unique_ptr<Engine> InitEngine(
        int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color, float bg_depth = INFINITY, 
        int pipeline_level = 1, bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ) override;

    std::unique_ptr<Pipeline> InitPipeline(int render_thread_num);

    virtual void UpdateTransform(const glm::mat4& transform) override {
        for (auto& vshader: vshaders) vshader->model = transform;
    }

public:
    std::unordered_map<std::string, std::vector<MyVertexShader::Input>> vertex_datas;
    std::unordered_map<std::string, VertexBuffer> vertex_buffers; // <model_name, vertex_buffer>
    std::vector<std::unique_ptr<MyVertexShader>> vshaders;
    std::vector<std::unique_ptr<MyFragmentShader>> fshaders;
};