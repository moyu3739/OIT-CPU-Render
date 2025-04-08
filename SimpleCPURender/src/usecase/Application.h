#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utility.h"
#include "Primitive.h"
#include "IntensityShader.h"
#include "Texture.h"
#include "ImageTexture.h"
#include "Pipeline.h"
#include "PipelineManager.h"
#include "Displayer.h"
#include "Engine.h"


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

struct Object {
    std::unique_ptr<std::vector<Vertex>> vertices;
    std::unique_ptr<Texture> texture;
};


class Application{
public:
    Application(int width, int height)
        : width(width), height(height) {}

    ~Application() {}

    virtual void Run() = 0;

    void LoadModel(const std::string& model_name, const std::string& obj_path, const std::string& texture_path = "");

    virtual void LoadVertexBuffer() = 0;

    virtual std::unique_ptr<Engine> InitEngine(int render_thread_num, int blend_thread_num,
                       const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY) = 0;

    virtual void UpdateTransform(const glm::mat4& transform) = 0;

    void RenderFrame();

    void RenderAnimation(float time_limit);

    // get model transformation
    // @param[in] translation  translation vector
    // @param[in] rotation  rotation angle in radian (around y-axis)
    // @param[in] scale  scale factor
    glm::mat4 GetModelTransform(
        const glm::vec3& translation = glm::vec3(0.0f, 0.0f, 0.0f),
        float rotation = 0.0f,
        float scale = 1.0f);

    // get view transformation
    // @param[in] eye  camera position
    // @param[in] target  the point the camera is looking at
    // @param[in] up  the up vector of the camera
    glm::mat4 GetViewTransform(
        const glm::vec3& eye,
        const glm::vec3& target = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    // get perspective projection transformation
    // @param[in] fovy  field of view in y-direction (in radian)
    // @param[in] aspect  aspect ratio of the window (width / height)
    // @param[in] znear  near plane for clipping
    // @param[in] zfar  far plane for clipping
    glm::mat4 GetPerspectiveProjectionTransform(
        float fovy,
        float aspect,
        float znear,
        float zfar);

    // get orthographic projection transformation
    // @param[in] orth_width  width of the orthographic projection
    // @param[in] orth_height  height of the orthographic projection
    // @param[in] znear  near plane for clipping
    // @param[in] zfar  far plane for clipping
    glm::mat4 GetOrthographicProjectionTransform(
        float orth_width,
        float orth_height,
        float znear,
        float zfar);

    // set normal of vertices of a model as the normal of the triangle
    // @param[in] model_name  the name of the model
    // @param[in] left_handed  the relation between the order of vertices within a triangle and the direction of its normal
    void ResetNormal(const std::string& model_name, bool left_handed = false);

    // set normal of vertices of a model as the normal of the triangle
    // @param[in] model_name  the name of the model
    // @param[in] left_handed  the relation between the order of vertices within a triangle and the direction of its normal
    void ResetNormalAll(bool left_handed = false) {
        for (const auto& model : models) ResetNormal(model.first, left_handed);
    }

public:
    int width;
    int height;
    std::unordered_map<std::string, Object> models; // <model_name, model>
};

