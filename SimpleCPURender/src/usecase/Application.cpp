#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include <iostream>
#include <cassert>
#include <tiny_obj_loader.h>
#include "Application.h"
#include "Timer.h"


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

glm::mat4 Application::GetModelTransform(const glm::vec3& translation, float rotation, float scale){
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

void Application::RenderFrame() {
    auto engine = InitEngine(16, 16, glm::vec3(1.0f));
    engine->RenderAndShow(0);
}

void Application::RenderAnimation(float time_limit) {
    auto engine = InitEngine(16, 16, glm::vec3(1.0f));

    const float T = 3.0f;
    const float Y = 0.5f;

    Timer tm;
    float total_render_time = 0.0f;
    float total_show_time = 0.0f;

    float last_frame = 0.0f;
    int frame_count = 0;

    tm.StartTimer();
    while(1){
        float t = tm.ReadTimer();
        if (t > time_limit) break;

        float r = 2 * 3.14159 * t / T;
        float y = Y * sin(r);
        UpdateTransform(GetModelTransform(glm::vec3(0.0f, 0.0f, 0.0f), r, 1.0f));

        // render and show
        float start_render = tm.ReadTimer();
        engine->PipelineRenderAndShow(1);
        float duration_render = tm.ReadTimer() - start_render;
        total_render_time += duration_render;

        // 计算帧率
        frame_count++;
        float now = tm.ReadTimer();
        float duration_frame = now - last_frame;
        last_frame = now;

        printf("\033[2J\033[H"); // 清空缓冲区
        printf("[%.1f]\n", t);
        printf("Render time:\t %.1f ms\n", duration_render * 1000);
        printf("Frame time:\t%.1f ms\n", duration_frame * 1000);
        printf("FPS:\t\t%.1f fps\n\n", 1.0f / duration_frame);
        printf("Average render time:\t%.1f ms\n", total_render_time * 1000 / frame_count);
        printf("Average frame time:\t%.1f ms\n", now * 1000 / frame_count);
        printf("Average FPS:\t\t%.1f fps\n", frame_count / now);
    }
}


