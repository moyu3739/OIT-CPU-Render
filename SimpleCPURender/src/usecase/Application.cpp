#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include <iostream>
#include <cassert>
#include <tiny_obj_loader.h>
#include "Application.h"



void Application::LoadModel(const std::string& model_name,
                            const std::string& obj_path, const std::string& texture_path){
    std::unique_ptr<std::vector<Vertex>> vertices;
    std::unique_ptr<Texture> texture;

    vertices = std::make_unique<std::vector<Vertex>>();
    if (!texture_path.empty()) texture = std::make_unique<ImageTexture>(texture_path.c_str());

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

            vertices->emplace_back(vertex);
        }
    }

    // for (auto& vertex : vertices){
    //     auto& p = vertex.position;
    //     auto& n = vertex.normal;
    //     auto& t = vertex.texcoord;
    //     printf("v(%f, %f, %f) - vn(%f, %f, %f) - t(%f, %f)\n", p.x, p.y, p.z, n.x, n.y, n.z, t.x, t.y);
    // }
    printf("have loaded model: \"%s\"\n", model_name.c_str());
    printf("    - obj file:     %s\n", obj_path.c_str());
    printf("    - texture file: %s\n", texture_path.c_str());
    printf("    - vertices:     %d\n", vertices->size());
    printf("    - triangles:    %d\n", vertices->size() / 3);

    assert(vertices->size() % 3 == 0);

    models[model_name] = Object{std::move(vertices), std::move(texture)};
}

void Application::LoadVertexBuffer(){
    // clear vertex buffers
    vertex_buffers.clear();
    // load vertex buffers
    for (const auto& model: models){ // for each model
        const std::string& model_name = model.first;
        const Object& obj = model.second;

        vertex_buffers[model_name] = std::vector<MyVertexShader::Input>();
        for (const Vertex& vertex: *obj.vertices){ // for each vertex in the model
            MyVertexShader::Input vs_input;
            vs_input.model_pos = vertex.position;
            vs_input.model_normal = vertex.normal;
            vs_input.texcoord = vertex.texcoord;
            vertex_buffers[model_name].emplace_back(vs_input);
        }
    }
}

std::unique_ptr<Engine> Application::InitEngine(int render_thread_num, int blend_thread_num,
                                const glm::vec3& bg_color, float bg_depth){
    auto engine = std::make_unique<Engine>(width, height, render_thread_num, blend_thread_num, bg_color, bg_depth, true);

    //////// set vertex-shader parameters
    auto vshader = std::make_unique<MyVertexShader>();

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.5f;
    vshader->model = GetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 10.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = GetViewTransform(eye, target, up);

    // initialize perspective projection transform
    glm::vec3 light_pos(0.0f, 0.0f, 10.0f);
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = GetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // initialize orthographic projection transform
    // glm::vec3 light_pos(0.0f, 0.0f, 10000.0f);
    // const float orth_width = 7.5f;
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // vshader->projection = GetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    for (const auto& nvb: vertex_buffers) {
        const std::string& model_name = nvb.first;
        const Object& obj = models[model_name];
        const std::vector<MyVertexShader::Input>& vertex_buffer = nvb.second;

        //////// set fragment-shader parameters
        auto fshader = std::make_unique<MyFragmentShader>();
        fshader->light_pos = light_pos;
        fshader->texture = obj.texture.get();
        
        //////// load shaders
        engine->pipeline_manager->CreatePipeline(vertex_buffer, vshader.get(), fshader.get(), true);
        fshaders.emplace_back(std::move(fshader));
    }
    vshaders.emplace_back(std::move(vshader));

    return engine;
}

std::unique_ptr<MyPipeline> Application::InitPipeline(){
    //////// set vertex-shader parameters
    auto vshader = std::make_unique<MyVertexShader>();

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.5f;
    vshader->model = GetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 10.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    vshader->view = GetViewTransform(eye, target, up);

    // initialize projection transform
    glm::vec3 light_pos(0.0f, 0.0f, 10.0f);
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    vshader->projection = GetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // const float orth_width = 15.0f;
    // glm::vec3 light_pos(0.0f, 0.0f, 10000.0f);
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // vshader->projection = GetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    //////// set fragment-shader parameters
    auto fshader = std::make_unique<MyFragmentShader>();
    fshader->light_pos = light_pos;
    fshader->light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    
    //////// load shaders
    std::unique_ptr<MyPipeline> pipeline = std::make_unique<MyPipeline>(vshader.get(), fshader.get());
    vshaders.emplace_back(std::move(vshader));
    fshaders.emplace_back(std::move(fshader));
    return pipeline;
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

void Application::ResetNormal(const std::string& model_name, bool left_handed){
    std::vector<Vertex>& vertices = *models[model_name].vertices;
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


