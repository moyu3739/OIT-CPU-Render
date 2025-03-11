#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utility.h"
#include "Primitive.h"


template <class VS, class FS>
class Pipeline{
public:
    Pipeline(int width, int height){
        this->width = width;
        this->height = height;
        frame_buffer.resize(width, std::vector<Fragment>(height, {glm::vec4(0.0f), INFINITY}));
    }

    ~Pipeline(){
        DestroyShader();
    }

    void LoadShader(VS* vertex_shader, FS* fragment_shader){
        this->vertex_shader = vertex_shader;
        this->fragment_shader = fragment_shader;
    }

    void DestroyShader(){
        CheckDel(vertex_shader);
        CheckDel(fragment_shader);
    }

    void LoadVertexBuffer(const std::vector<typename VS::Input>& vertex_buffer){
        this->vertex_buffer = vertex_buffer; // NEED TO BE OPTIMIZED
    }

    void Render(){
        assert(vertex_buffer.size() % 3 == 0);
    
        for (int i = 0; i < vertex_buffer.size(); i += 3){
            typename VS::Input& vs_input_v1 = vertex_buffer[i];
            typename VS::Input& vs_input_v2 = vertex_buffer[i + 1];
            typename VS::Input& vs_input_v3 = vertex_buffer[i + 2];
            
            // call vertex-shader
            typename VS::Output vs_output_v1 = vertex_shader->Call(vs_input_v1);
            typename VS::Output vs_output_v2 = vertex_shader->Call(vs_input_v2);
            typename VS::Output vs_output_v3 = vertex_shader->Call(vs_input_v3);
    
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
    
            // clip bounding box
            pixel_min_x = std::max(0, pixel_min_x);
            pixel_max_x = std::min(width - 1, pixel_max_x);
            pixel_min_y = std::max(0, pixel_min_y);
            pixel_max_y = std::min(height - 1, pixel_max_y);

            // printf("rendering triangle %d: (%d, %d) - (%d, %d)\n", i / 3, pixel_min_x, pixel_min_y, pixel_max_x, pixel_max_y);
    
            // rasterization
            for (int x = pixel_min_x; x <= pixel_max_x; x++){
                for (int y = pixel_min_y; y <= pixel_max_y; y++){
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
                    typename FS::Input fs_input = fragment_shader->Interpolate(
                        vs_output_v1, vs_output_v2, vs_output_v3, barycentric);
                    float screen_depth = fragment_shader->InterpolateAttr(
                        screen_pos_v1.z, screen_pos_v2.z, screen_pos_v3.z, barycentric);

                    // depth test
                    if (screen_depth < -1.0f || screen_depth > 1.0f){
                        continue;
                    }

                    // write color to frame buffer
                    if (screen_depth < frame_buffer[x][y].depth){ // get max depth, note that z-axis points out of the screen
                        // call fragment-shader
                        typename FS::Output fs_output = fragment_shader->Call(fs_input);
    
                        frame_buffer[x][y].color = fs_output.__color__;
                        frame_buffer[x][y].depth = screen_depth;
                    }
                }
            }
        }
    }
    
    // clear the frame buffer
    void ClearFrameBuffer(){
        for(auto& row : frame_buffer){
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
    std::vector<typename VS::Input> vertex_buffer;
    std::vector<std::vector<Fragment>> frame_buffer; // a `Fragment` for each pixel

public:
    int width;
    int height;
    VS* vertex_shader = nullptr;
    FS* fragment_shader = nullptr;
};

