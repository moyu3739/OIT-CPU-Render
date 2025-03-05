#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Vertex.h"
#include "Shader.h"


class RenderPipeline{
public:
    RenderPipeline(int width, int height){
        this->width = width;
        this->height = height;
        framebuffer.resize(width, std::vector<FragmentPixel>(height, {glm::vec4(0.0f), INFINITY}));
    }

    void LoadObj(const std::string& model_path);

    // set model transformation
    // @param[in] translation  translation vector
    // @param[in] rotation  rotation angle in radian (around y-axis)
    // @param[in] scale  scale factor
    void SetModelTransform(
        const glm::vec3& translation = glm::vec3(0.0f, 0.0f, 0.0f),
        float rotation = 0.0f,
        float scale = 1.0f);

    // set view transformation
    // @param[in] eye  camera position
    // @param[in] target  the point the camera is looking at
    // @param[in] up  the up vector of the camera
    void SetViewTransform(
        const glm::vec3& eye,
        const glm::vec3& target = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

    // set perspective projection transformation
    // @param[in] fovy  field of view in y-direction (in radian)
    // @param[in] aspect  aspect ratio of the window (width / height)
    // @param[in] znear  near plane for clipping
    // @param[in] zfar  far plane for clipping
    void SetPerspectiveProjectionTransform(
        float fovy,
        float aspect,
        float znear,
        float zfar);

    // set orthographic projection transformation
    // @param[in] orth_width  width of the orthographic projection
    // @param[in] orth_height  height of the orthographic projection
    // @param[in] znear  near plane for clipping
    // @param[in] zfar  far plane for clipping
    void SetOrthographicProjectionTransform(
        float orth_width,
        float orth_height,
        float znear,
        float zfar);

    void InitShader();

    void Render();

    // set normal of vertices as the normal of the triangle
    // @param[in] left_handed  the relation between the order of vertices within a triangle and the direction of its normal
    void ResetNormal(bool left_handed = false);

    // clear the framebuffer
    void ClearFrameBuffer(){
        for(auto& row : framebuffer){
            for(auto& pixel : row){
                pixel.color = glm::vec4(0.0f);
                pixel.depth = INFINITY;
            }
        }
    }

    // map `coord`, from float(-1, 1) to int[0, `range`)
    static int Coord2Pixel(float coord, int range){
        return static_cast<int>((coord + 1.0f) * 0.5f * range);
    }

    // map `pixel`, from int[0, `range`) to float(-1, 1)
    static float Pixel2Coord(int pixel, int range){
        return (2.0f * pixel + 1.0f) / range - 1.0f;
    }

public:
    int width;
    int height;

    std::vector<Vertex> vertices;

    VertexShader vertex_shader;
    FragmentShader fragment_shader;

    std::vector<std::vector<FragmentPixel>> framebuffer; // a `FragmentPixel` for each pixel
};

