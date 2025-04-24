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

public:
    CornellBox(int width, int height, int N_shapes, bool combined, int random_seed = 0)
    : Application(width, height), N_shapes(N_shapes), combined(combined), random_gen(random_seed) {}

    virtual void Run() override {
        LoadVertexBuffer();

        RenderAnimation(10.0f);
        // RenderFrame();
    }

    void LoadVertexBuffer() override {
        if (combined)
            LoadVertexBufferShapesCombined();
        else
            LoadVertexBufferShapesDivided();
    }

    void LoadVertexBufferShapesDivided();

    void LoadVertexBufferShapesCombined();

    virtual std::unique_ptr<Engine> InitEngine(
        int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color, float bg_depth = INFINITY, 
        int parallel_level = 1, bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f) override;

    virtual void UpdateTransform(const glm::mat4& transform) override {
        *global_model = transform;
    }

    class RandomGenerator {
    public:
        RandomGenerator(int seed = 0) {
            if (seed == 0) {
                std::random_device rd;
                seed = rd();
                gen = std::mt19937(seed);
                printf("Random seed: %d\n", seed);
            }
            else {
                gen = std::mt19937(seed);
            }
        }

        int RandomInt(int v_min, int v_max) {
            std::uniform_int_distribution<int> dis(v_min, v_max);
            return dis(gen);
        }

        float RandomFloat(float v_min, float v_max) {
            std::uniform_real_distribution<float> dis(v_min, v_max);
            return dis(gen);
        }

        glm::vec3 RandomVec3(const glm::vec3& t_min, const glm::vec3& t_max) {
            return glm::vec3(RandomFloat(t_min.x, t_max.x),
                             RandomFloat(t_min.y, t_max.y),
                             RandomFloat(t_min.z, t_max.z));
        }
    
    private:
        std::mt19937 gen;
    };

    

public:
    int N_shapes;
    bool combined = false;
    std::shared_ptr<glm::mat4> global_model;
    std::vector<std::unique_ptr<Shape>> shapes;
    RandomGenerator random_gen;

    std::unordered_map<std::string, std::vector<MyVertexShader::Input>> vertex_datas;
    std::unordered_map<std::string, std::vector<VertexShader::InputWrapper>> vertex_buffers; // <model_name, vertex_buffer>
    std::vector<std::unique_ptr<MyVertexShader>> vshaders;
    std::vector<std::unique_ptr<MyFragmentShader>> fshaders;
};