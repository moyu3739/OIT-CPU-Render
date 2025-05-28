// #ifndef TINYOBJLOADER_IMPLEMENTATION
// #define TINYOBJLOADER_IMPLEMENTATION
// #endif

#include <iostream>
#include <string>
#include <ctime>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include "PhongShader.h"
#include "Engine.h"
#include "Phong.h"


glm::mat4 GetModelTransform(const glm::vec3& translation, float rotation, float scale) {
    // model matrix transform the homogenous coodinates from
    // model space (raw vertex data of the model) to world space, depending on following
    // translation
    glm::mat4 translation_mat(
                    1,             0,             0,          0,
                    0,             1,             0,          0,
                    0,             0,             1,          0,
        translation.x, translation.y, translation.z,          1
    );
    // rotation
    glm::mat4 rotation_mat(
        cos(rotation),     0, -sin(rotation),     0,
                    0,     1,              0,     0,
        sin(rotation),     0,  cos(rotation),     0,
                    0,     0,              0,     1
    );
    // scale
    glm::mat4 scale_mat(
        scale,     0,     0,    0,
            0, scale,     0,    0,
            0,     0, scale,    0,
            0,     0,     0,    1
    );
    return translation_mat * rotation_mat * scale_mat;
}


void LoadModel(const std::string& obj_path, VertexBuffer& vertex_buffer) {
    // use tiny_obj_loader to load mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    std::string::size_type index = obj_path.find_last_of("/");
    std::string mtl_base_dir = obj_path.substr(0, index + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, obj_path.c_str(), mtl_base_dir.c_str())) {
        throw std::runtime_error("load " + obj_path + " failure: " + err);
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            glm::vec3 model_pos;
            model_pos.x = attrib.vertices[3 * index.vertex_index + 0];
            model_pos.y = attrib.vertices[3 * index.vertex_index + 1];
            model_pos.z = attrib.vertices[3 * index.vertex_index + 2];

            glm::vec3 model_normal;
            if (index.normal_index >= 0) {
                model_normal.x = attrib.normals[3 * index.normal_index + 0];
                model_normal.y = attrib.normals[3 * index.normal_index + 1];
                model_normal.z = attrib.normals[3 * index.normal_index + 2];
            }

            vertex_buffer.emplace_back(new PhongVertexShader::Input{model_pos, model_normal});
        }
    }

    printf("vertices:  %d\n", vertex_buffer.size());
    printf("triangles: %d\n", vertex_buffer.size() / 3);
}


void QuickStart() {
    // set up window size
    const int width = 800;
    const int height = 800;
    // new shaders
    auto* vshader = new PhongVertexShader;
    auto* fshader = new PhongFragmentShader;

    //// initialize vertex shader
    // initialize model transform
    const glm::vec3 translation(0.0f, 0.0f, 0.0f);
    const float rotation = glm::radians(0.0f);
    const float scale = 1.0f;
    vshader->model = GetModelTransform(translation, rotation, scale);
    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 8.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = glm::lookAt(eye, target, up);
    // initialize perspective projection transform
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = glm::perspective(fovy, aspect, znear, zfar);

    //// initialize fragment shader
    fshader->light_pos = glm::vec3(0.0f, 10.0f, 10.0f);
    fshader->view_pos = eye;
    fshader->light_color = glm::vec3(1.0f);
    fshader->obj_color = glm::vec3(0.6f, 1.0f, 0.8f);
    fshader->ka = 0.5f;
    fshader->kd = 0.1f;
    fshader->ks = 0.4f;
    fshader->shininess = 32.0f;
    fshader->alpha = 0.5f;

    // load a cube to vertex buffer
    VertexBuffer vertex_buffer;
    LoadModel("asset/obj/bunny.obj", vertex_buffer);

    // create engine and create pipeline in it
    auto* engine = new Engine(width, height, 16, 16, glm::vec3(1.0f), INFINITY, 3, true);
    engine->CreatePipeline(vertex_buffer, vshader, fshader, ON_FACE, true);

    // engine.SerialRender();
    
    // render loop
    float t0 = std::clock() / 1000.0f;
    while(true) {
        float t = std::clock() / 1000.0f - t0;
        vshader->model = GetModelTransform(translation, t, scale);

        engine->PipelinedRender(1, 1);
        if (t > 10.0f) break; // quit after 10 seconds
    }

    delete engine;
    delete vshader;
    delete fshader;
    for(auto* input : vertex_buffer) delete input;
}

