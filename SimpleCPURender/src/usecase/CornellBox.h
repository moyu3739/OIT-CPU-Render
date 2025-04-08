#pragma once

#include <iostream>
#include <cmath>
#include <memory>
#include <random>
#include <cassert>
#include <glm/glm.hpp>
#include "Application.h"
#include "Engine.h"
#include "FrameBuffer.h"
#include "Displayer.h"
#include "Pipeline.h"
#include "Shader.h"
#include "ShapeShader.h"
#include "AnimeStyleShader.h"


class CornellBox: public Application {
    using MyVertexShader   = ShapeVertexShader;
    using MyFragmentShader = ShapeFragmentShader;
    using MyPipeline = Pipeline<MyVertexShader, MyFragmentShader>;

public:
    CornellBox(int width, int height, int N_shapes, bool combined)
    : Application(width, height), N_shapes(N_shapes), combined(combined) {}

    virtual void Run() override {
        LoadVertexBuffer();
        // RenderAnimation(100.0f);
        RenderFrame();
    }

    void LoadVertexBuffer() override {
        if (combined)
            LoadVertexBufferShapesCombined();
        else
            LoadVertexBufferShapesDivided();
    }

    void LoadVertexBufferShapesDivided();

    void LoadVertexBufferShapesCombined();

    virtual std::unique_ptr<Engine> InitEngine(int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY) override;

    virtual void UpdateTransform(const glm::mat4& transform) override {
        *global_model = transform;
    }

    static int RandomInt(int v_min, int v_max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(v_min, v_max);
        return dis(gen);
    }

    static float RandomFloat(float v_min, float v_max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(v_min, v_max);
        return dis(gen);
    }

    static glm::vec3 RandomVec3(const glm::vec3& t_min, const glm::vec3& t_max) {
        return glm::vec3(RandomFloat(t_min.x, t_max.x),
                         RandomFloat(t_min.y, t_max.y),
                         RandomFloat(t_min.z, t_max.z));
    }

public:
    int N_shapes;
    bool combined = false;
    std::shared_ptr<glm::mat4> global_model;
    std::vector<std::unique_ptr<Shape>> shapes;

    std::unordered_map<std::string, std::vector<typename MyVertexShader::Input>> vertex_buffers; // <model_name, vertex_buffer>
    std::vector<std::unique_ptr<MyVertexShader>> vshaders;
    std::vector<std::unique_ptr<MyFragmentShader>> fshaders;
};