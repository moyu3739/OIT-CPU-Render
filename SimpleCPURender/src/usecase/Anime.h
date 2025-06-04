#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "AnimeStyleShader.h"
#include "Engine.h"
#include "Application.h"

using namespace oit;


class Anime: public Application {
    using MyVertexShader   = AnimeStyleVertexShader;
    using MyFragmentShader = AnimeStyleFragmentShader;

public:
    Anime(
        int width, int height,
        bool enable_oit = false, bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ): Application(width, height, enable_oit, use_backward_pplist, backward_blend_alpha_threshold) {}

    virtual void Run() override {
        LoadModel("Babala hair", "asset/obj/Babala/hair.obj", "asset/texture/Babala/hair.png");
        LoadModel("Babala face", "asset/obj/Babala/face.obj", "asset/texture/Babala/face.png");
        LoadModel("Babala body", "asset/obj/Babala/body.obj", "asset/texture/Babala/body.png");
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