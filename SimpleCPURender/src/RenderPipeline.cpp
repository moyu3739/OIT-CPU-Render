#define TINYOBJLOADER_IMPLEMENTATION

#include <cassert>
#include <tiny_obj_loader.h>
#include "RenderPipeline.h"


void RenderPipeline::LoadObj(const std::string& model_path){
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

void RenderPipeline::SetModelTransform(const glm::vec3& translation, float rotation, float scale){
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
    vertex_shader.model = translation_mat * rotation_mat * scale_mat;
}

void RenderPipeline::SetViewTransform(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up){
    vertex_shader.view = glm::lookAt(eye, target, up);
}

void RenderPipeline::SetPerspectiveProjectionTransform(float fovy, float aspect, float znear, float zfar){
    vertex_shader.projection = glm::perspective(fovy, aspect, znear, zfar);
}

void RenderPipeline::SetOrthographicProjectionTransform(float orth_width, float orth_height, float znear, float zfar){
    vertex_shader.projection = glm::ortho(
        -orth_width / 2, orth_width / 2, -orth_height / 2, orth_height / 2, znear, zfar);
}

void RenderPipeline::InitShader(){
    //////// set vertex-shader parameters

    // initialize model transform
    glm::vec3 translation(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.0f;
    SetModelTransform(translation, rotation, scale);

    // initialize view transform
    const glm::vec3 eye(0.0f, 0.0f, 10.0f); // camera position
    const glm::vec3 target(0.0f, 0.0f, 0.0f); // `eye` and `target` defines the direction the camera looking at
    const glm::vec3 up(0.0f, 1.0f, 0.0f); // `up` vector of the camera
    SetViewTransform(eye, target, up);

    // initialize projection transform
    constexpr float fovy = glm::radians(60.0f); // field of view
    const float aspect = 1.0f * width / height; // aspect of the window (width / height)
    const float znear = 0.01f; // near plane for clipping
    const float zfar = 100.0f; // far plane for clipping
    SetPerspectiveProjectionTransform(fovy, aspect, znear, zfar);

    // const float orth_width = 10.0f;
    // const float orth_height = orth_width / width * height;
    // const float znear = 0.1f; // near plane for clipping
    // const float zfar = 100.0f; // far plane for clipping
    // SetOrthographicProjectionTransform(orth_width, orth_height, znear, zfar);

    //////// set fragment-shader parameters
    fragment_shader.light_pos = glm::vec3(0.0f, 0.0f, 100.0f);
    fragment_shader.light_color = glm::vec3(1.0f, 1.0f, 1.0f);
    fragment_shader.ka = 0.1f;
    fragment_shader.kd = 0.8f;
    fragment_shader.obj_color = glm::vec3(1.0f, 1.0f, 1.0f);
}

void RenderPipeline::Render(){
    assert(vertices.size() % 3 == 0);

    for (int i = 0; i < vertices.size(); i += 3){
        Vertex& v1 = vertices[i];
        Vertex& v2 = vertices[i + 1];
        Vertex& v3 = vertices[i + 2];
        
        // call vertex-shader
        VertexShader::VertexShaderOutput vs_output_v1 = vertex_shader.Call({v1.position, v1.normal});
        VertexShader::VertexShaderOutput vs_output_v2 = vertex_shader.Call({v2.position, v2.normal});
        VertexShader::VertexShaderOutput vs_output_v3 = vertex_shader.Call({v3.position, v3.normal});

        // save w for perspective division
        float w1 = vs_output_v1.__position__.w;
        float w2 = vs_output_v2.__position__.w;
        float w3 = vs_output_v3.__position__.w;

        // map to clipping space, where (-1, 1) is visible region
        glm::vec4 screen_pos_v1 = vs_output_v1.__position__ / w1;
        glm::vec4 screen_pos_v2 = vs_output_v2.__position__ / w2;
        glm::vec4 screen_pos_v3 = vs_output_v3.__position__ / w3;

        // calculate bounding box
        int pixel_min_x = Coord2Pixel(std::min(screen_pos_v1.x, std::min(screen_pos_v2.x, screen_pos_v3.x)), width);
        int pixel_max_x = Coord2Pixel(std::max(screen_pos_v1.x, std::max(screen_pos_v2.x, screen_pos_v3.x)), width);
        int pixel_min_y = Coord2Pixel(std::min(screen_pos_v1.y, std::min(screen_pos_v2.y, screen_pos_v3.y)), height);
        int pixel_max_y = Coord2Pixel(std::max(screen_pos_v1.y, std::max(screen_pos_v2.y, screen_pos_v3.y)), height);

        // glm::mat3x3 m; m * m;

        // rasterization
        for (int x = pixel_min_x; x <= pixel_max_x; x++){
            for (int y = pixel_min_y; y <= pixel_max_y; y++){
                // check if pixel is in screen
                if (x < 0 || x >= width || y < 0 || y >= height){
                    continue;
                }

                // map pixel to clipping space, where (-1, 1) is visible region
                float screen_x = Pixel2Coord(x, width);
                float screen_y = Pixel2Coord(y, height);

                // barycentric coordinates
                glm::vec3 barycentric = glm::inverse(
                    glm::mat3x3(screen_pos_v1.x, screen_pos_v1.y, 1.0f,
                                screen_pos_v2.x, screen_pos_v2.y, 1.0f,
                                screen_pos_v3.x, screen_pos_v3.y, 1.0f)
                ) * glm::vec3(screen_x, screen_y, 1.0f);
                if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0){
                    continue;
                }

                // perspective division
                barycentric /= glm::vec3(w1, w2, w3);

                // perspective-correct interpolation
                glm::vec3 world_pos = fragment_shader.Interpolate(
                    vs_output_v1.world_pos, vs_output_v2.world_pos, vs_output_v3.world_pos, barycentric);
                glm::vec3 world_normal = fragment_shader.Interpolate(
                    vs_output_v1.world_normal, vs_output_v2.world_normal, vs_output_v3.world_normal, barycentric);
                float screen_depth = fragment_shader.Interpolate(
                    screen_pos_v1.z, screen_pos_v2.z, screen_pos_v3.z, barycentric);

                // depth test
                if (screen_depth < -1.0f || screen_depth > 1.0f){
                    continue;
                }

                // write color to framebuffer
                if (screen_depth < framebuffer[x][y].depth){ // get max depth, note that z-axis points out of the screen
                    // call fragment-shader
                    FragmentShader::FragmentShaderOutput fs_output = fragment_shader.Call({world_pos, world_normal});

                    framebuffer[x][y].color = fs_output.__color__;
                    framebuffer[x][y].depth = screen_depth;
                }
            }
        }
    }
}

void RenderPipeline::ResetNormal(bool left_handed){
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


