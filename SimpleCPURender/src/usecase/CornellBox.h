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
#include "Frontend.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Shape.h"
#include "ShapeShader.h"


enum ShapeType {
    SHAPE_SPHERE,
    SHAPE_CYLINDER,
    SHAPE_BOX,
    SHAPE_CONE,
    SHAPE_RANDOM = -1
};


class CornellBox: public Application {
    using MyVertexShader   = ShapeVertexShader;
    using MyFragmentShader = ShapeFragmentShader;

public:
    CornellBox(
        int width, int height, int N_shapes, bool combined,
        const glm::vec3& t_min, const glm::vec3& t_max, int random_seed = 0, ShapeType shape_type = SHAPE_RANDOM,
        bool enable_oit = false, bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ):
        Application(width, height, enable_oit, use_backward_pplist, backward_blend_alpha_threshold),
        N_shapes(N_shapes), combined(combined),
        t_min(t_min), t_max(t_max) ,random_gen(random_seed), shape_type(shape_type)
    {}

    virtual void Run() override {
        LoadVertexBuffer();

        RenderAnimation(1000.0f);
        // RenderFrame();
    }

    void LoadVertexBuffer() override {
        if (combined)
            LoadVertexBufferShapesCombined();
        else
            LoadVertexBufferShapesDivided();

        int total_vertex = 0;
        for (auto& vb: vertex_buffers) total_vertex += vb.second.size();
        printf("- vertices:  %d\n", total_vertex);
        printf("- triangles: %d\n", total_vertex / 3);
    }

    void LoadVertexBufferShapesDivided();

    void LoadVertexBufferShapesCombined();

    virtual std::unique_ptr<Engine> InitEngine(
        int render_thread_num, int blend_thread_num,
        const glm::vec3& bg_color, float bg_depth = INFINITY, 
        int pipeline_level = 1, bool enable_oit = false,
        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f
    ) override;

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
    glm::vec3 t_min;
    glm::vec3 t_max;
    ShapeType shape_type;

    std::shared_ptr<glm::mat4> global_model;
    std::vector<std::unique_ptr<Shape>> shapes;
    RandomGenerator random_gen;

    std::unordered_map<std::string, std::vector<MyVertexShader::Input>> vertex_datas;
    std::unordered_map<std::string, VertexBuffer> vertex_buffers; // <model_name, vertex_buffer>
    std::vector<std::unique_ptr<MyVertexShader>> vshaders;
    std::vector<std::unique_ptr<MyFragmentShader>> fshaders;
};