#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utility.h"
#include "Primitive.h"
#include "ItensityShader.h"
#include "Texture.h"
#include "ImageTexture.h"
#include "Pipeline.h"


class Application{
public:
    Application(int width, int height): pipeline(width, height) {}

    ~Application() {}

    void LoadObj(const std::string& model_path);

    void LoadVertexBuffer();

    void InitShader();

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

    // set normal of vertices as the normal of the triangle
    // @param[in] left_handed  the relation between the order of vertices within a triangle and the direction of its normal
    void ResetNormal(bool left_handed = false);

public:
    std::vector<Vertex> vertices;
    Pipeline<ItensityVertexShader, ItensityFragmentShader> pipeline;
};

