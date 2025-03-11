#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include <iostream>
#include <cassert>
#include <tiny_obj_loader.h>
#include "Application.h"



void Application::LoadObj(const std::string& model_path){
    // use tiny_obj_loader to load mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    std::string::size_type index = model_path.find_last_of("/");
    std::string mtl_base_dir = model_path.substr(0, index + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_path.c_str(), mtl_base_dir.c_str())) {
        throw std::runtime_error("load " + model_path + " failure: " + err);
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;

            vertex.position.x = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position.y = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position.z = attrib.vertices[3 * index.vertex_index + 2];

            if (index.normal_index >= 0) {
                vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
                vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
                vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
            }

            if (index.texcoord_index >= 0) {
                vertex.texcoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.texcoord.y = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            vertices.emplace_back(vertex);
        }
    }

    // for (auto& vertex : vertices){
    //     auto& p = vertex.position;
    //     auto& n = vertex.normal;
    //     auto& t = vertex.texcoord;
    //     printf("v(%f, %f, %f) - vn(%f, %f, %f) - t(%f, %f)\n", p.x, p.y, p.z, n.x, n.y, n.z, t.x, t.y);
    // }
    printf("total vertices:  %d\n", vertices.size());
    printf("total triangles: %d\n", vertices.size() / 3);

    assert(vertices.size() % 3 == 0);
}

void Application::LoadVertexBuffer(){
    std::vector<ItensityVertexShader::Input> vertex_buffer;
    for (const auto& vertex : vertices){
        vertex_buffer.emplace_back(ItensityVertexShader::Input{vertex.position, vertex.normal, vertex.texcoord});
    }
    pipeline.LoadVertexBuffer(vertex_buffer);
}

void Application::InitShader(){
    //////// initialize shaders
    auto vshader = new ItensityVertexShader;
    auto fshader = new ItensityFragmentShader;

    //////// set vertex-shader parameters

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.0f;
    vshader->model = GetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 10.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = GetViewTransform(eye, target, up);

    // initialize projection transform
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * pipeline.width / pipeline.height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = GetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // const float orth_width = 10.0f;
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // vshader->projection = GetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    //////// set fragment-shader parameters
    fshader->light_pos = glm::vec3(0.0f, 100.0f, 100.0f);
    fshader->light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    fshader->ka = 0.1f;
    fshader->kd = 0.8f;
    // fshader->obj_color = glm::vec3(1.0f, 1.0f, 1.0f);
    // fshader->texture = new CheckerTexture(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 1.0f, 1.0f), 16.0f);
    fshader->texture = new ImageTexture("asset/texture/Ankila.png", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), CLAMP_TO_BORDER, NEAREST);

    //////// load shaders
    pipeline.LoadShader(vshader, fshader);
}

glm::mat4 Application::GetModelTransform(const glm::vec3& translation, float rotation, float scale){
    // model matrix transform the homogenous coodinates from
    // model space (raw vertex data of the model) to world space, depending on following
    // translation
    glm::mat4 translation_mat(
                    1,             0,             0,          0,
                    0,             1,             0,          0,
                    0,             0,             1,          0,
        translation.x, translation.y, translation.x,          1
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

glm::mat4 Application::GetViewTransform(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up){
    return glm::lookAt(eye, target, up);
}

glm::mat4 Application::GetPerspectiveProjectionTransform(float fovy, float aspect, float znear, float zfar){
    return glm::perspective(fovy, aspect, znear, zfar);
}

glm::mat4 Application::GetOrthographicProjectionTransform(float orth_width, float orth_height, float znear, float zfar){
    return glm::ortho(-orth_width / 2, orth_width / 2, -orth_height / 2, orth_height / 2, znear, zfar);
}

void Application::ResetNormal(bool left_handed){
    assert(vertices.size() % 3 == 0);

    for (int i = 0; i < vertices.size(); i += 3){
        Vertex& v1 = vertices[i];
        Vertex& v2 = vertices[i + 1];
        Vertex& v3 = vertices[i + 2];

        glm::vec3 e12 = v2.position - v1.position;
        glm::vec3 e13 = v3.position - v1.position;
        glm::vec3 normal = left_handed ? glm::normalize(glm::cross(e13, e12)) : glm::normalize(glm::cross(e12, e13));

        v1.normal = normal;
        v2.normal = normal;
        v3.normal = normal;
    }
}


