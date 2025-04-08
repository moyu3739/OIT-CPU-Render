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
#include "AnimeStyleShader.h"
#include "Texture.h"
#include "ImageTexture.h"
#include "Pipeline.h"
#include "PipelineManager.h"
#include "Engine.h"


class Anime: public Application {
    using MyVertexShader   = AnimeStyleVertexShader;
    using MyFragmentShader = AnimeStyleFragmentShader;
    using MyPipeline = Pipeline<MyVertexShader, MyFragmentShader>;

public:
    Anime(int width, int height): Application(width, height) {}

    virtual void Run() override {
        LoadModel("Babala hair", "asset/obj/Babala/hair.obj", "asset/texture/Babala/hair.png");
        LoadModel("Babala face", "asset/obj/Babala/face.obj", "asset/texture/Babala/face.png");
        LoadModel("Babala body", "asset/obj/Babala/body.obj", "asset/texture/Babala/body.png");
        LoadVertexBuffer();

        // RenderAnimation(10.0f);
        RenderFrame();
    }

    virtual void LoadVertexBuffer() override;

    virtual std::unique_ptr<Engine> InitEngine(int render_thread_num, int blend_thread_num,
                       const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY) override;

    std::unique_ptr<MyPipeline> InitPipeline();

    virtual void UpdateTransform(const glm::mat4& transform) override {
        for (auto& vshader: vshaders) vshader->model = transform;
    }

public:
    std::unordered_map<std::string, std::vector<typename MyVertexShader::Input>> vertex_buffers; // <model_name, vertex_buffer>
    std::vector<std::unique_ptr<MyVertexShader>> vshaders;
    std::vector<std::unique_ptr<MyFragmentShader>> fshaders;
};