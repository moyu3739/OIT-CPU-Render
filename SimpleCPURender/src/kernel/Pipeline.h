#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <thread>
#include <atomic>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "utility.h"
#include "Primitive.h"
#include "FrameBuffer.h"


class PipelineBase{
public:
    PipelineBase() {}

    virtual ~PipelineBase() {}

    // render the pipeline
    // @param[in] frame_buffer  frame buffer
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    virtual void Render(FrameBuffer* frame_buffer, int thread_num = 1, bool use_oit = false) = 0;
};


template <class VS, class FS>
class Pipeline: public PipelineBase{
public:
    Pipeline() {}

    Pipeline(VS* vertex_shader, FS* fragment_shader)
        : vertex_shader(vertex_shader), fragment_shader(fragment_shader) {}

    ~Pipeline() {}

    void BoundVertexShader(VS* vertex_shader){
        this->vertex_shader = vertex_shader;
    }

    void BoundFragmentShader(FS* fragment_shader){
        this->fragment_shader = fragment_shader;
    }

    void BoundShader(VS* vertex_shader, FS* fragment_shader){
        this->vertex_shader = vertex_shader;
        this->fragment_shader = fragment_shader;
    }

    void BoundVertexBuffer(const std::vector<typename VS::Input>& vertex_buffer){
        this->vertex_buffer = &vertex_buffer;
    }

    virtual void Render(FrameBuffer* frame_buffer, int thread_num = 1, bool use_oit = false) override {
        // RenderSplit(frame_buffer, thread_num, use_oit);
        RenderFetch(frame_buffer, thread_num, use_oit);
    }

    // render the pipeline,
    // using paralleling method of trivial splitting vertex buffer equally
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_num  number of threads to render the pipeline
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    void RenderSplit(FrameBuffer* frame_buffer, int thread_num = 1, bool use_oit = false) {
        assert(vertex_buffer->size() % 3 == 0);

        // if single thread, do it directly
        if (thread_num == 1) {
            if (use_oit) RenderProcessRange<true>(0, vertex_buffer->size(), frame_buffer, 0);
            else RenderProcessRange<false>(0, vertex_buffer->size(), frame_buffer, 0);
            return;
        }

        int f_num = vertex_buffer->size() / 3;
        std::vector<int> split_points = RangeSplit(0, f_num, thread_num);
        std::vector<std::thread> threads;
        if (use_oit) { // use OIT
            for (int i = 0; i < thread_num; i++){
                int v_begin = split_points[i] * 3;
                int v_end = split_points[i + 1] * 3;
                threads.emplace_back(&Pipeline::RenderProcessRange<true>, this, v_begin, v_end, frame_buffer, i);
            }
        }
        else { // not use OIT
            for (int i = 0; i < thread_num; i++){
                int v_begin = split_points[i] * 3;
                int v_end = split_points[i + 1] * 3;
                threads.emplace_back(&Pipeline::RenderProcessRange<false>, this, v_begin, v_end, frame_buffer, i);
            }
        }

        for (std::thread& thread: threads) thread.join();
    }

    // render the pipeline,
    // using paralleling method of atomic fetching vertex buffer by threads
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_num  number of threads to render the pipeline
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    void RenderFetch(FrameBuffer* frame_buffer, int thread_num = 1, bool use_oit = false) {
        assert(vertex_buffer->size() % 3 == 0);

        // if single thread, do it directly
        if (thread_num == 1) {
            if (use_oit) RenderProcessRange<true>(0, vertex_buffer->size(), frame_buffer, 0);
            else RenderProcessRange<false>(0, vertex_buffer->size(), frame_buffer, 0);
            return;
        }

        std::atomic<int> counter(0);
        std::vector<std::thread> threads;
        if (use_oit) { // use OIT
            for (int i = 0; i < thread_num; i++){
                threads.emplace_back(&Pipeline::RenderProcessFetch<true>, this, &counter, frame_buffer, i);
            }
        }
        else { // not use OIT
            for (int i = 0; i < thread_num; i++){
                threads.emplace_back(&Pipeline::RenderProcessFetch<false>, this, &counter, frame_buffer, i);
            }
        }

        for (std::thread& thread: threads) thread.join();
    }

private:
    // render the pipeline, given the range of vertices
    // @param[in] v_begin  the index of the first vertex
    // @param[in] v_end  the index AFTER the last vertex
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_id  thread id (0, 1, 2, ..., thread_num - 1)
    template <bool use_oit = false>
    void RenderProcessRange(int v_begin, int v_end, FrameBuffer* frame_buffer, int thread_id) {
        for (int i = v_begin; i < v_end; i += 3){
            RenderTriangle<use_oit>(i, frame_buffer, thread_id);
        }
    }

    // render the pipeline, vertices fetched atomically and orderly by threads themselves
    // @param[in] counter  atomic counter for fetching vertices
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_id  thread id (0, 1, 2, ..., thread_num - 1)
    template <bool use_oit = false>
    void RenderProcessFetch(std::atomic<int>* counter, FrameBuffer* frame_buffer, int thread_id) {
        while (true){
            int i = counter->fetch_add(3, std::memory_order_relaxed);
            if (i >= vertex_buffer->size()) break;
            RenderTriangle<use_oit>(i, frame_buffer, thread_id);
        }
    }

    // render a triangle, given the index of the first vertex
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_id  thread id (0, 1, 2, ..., thread_num - 1)
    template <bool use_oit = false>
    void RenderTriangle(int idx, FrameBuffer* frame_buffer, int thread_id) {
        int width = frame_buffer->GetWidth();
        int height = frame_buffer->GetHeight();

        const typename VS::Input& vs_input_v1 = vertex_buffer->at(idx);
        const typename VS::Input& vs_input_v2 = vertex_buffer->at(idx + 1);
        const typename VS::Input& vs_input_v3 = vertex_buffer->at(idx + 2);
        
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

        // rasterization
        for (int x = pixel_min_x; x <= pixel_max_x; x++){
            for (int y = pixel_min_y; y <= pixel_max_y; y++){
                /*************************** CAN BE ENCAPSULATED *******************************/
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
                /*********************************************************************************/

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
                if (frame_buffer->DepthTestAt(screen_depth, x, y)){ // get max depth, note that z-axis points out of the screen
                    // call fragment-shader
                    typename FS::Output fs_output = fragment_shader->Call(fs_input);

                    if constexpr (use_oit)
                        frame_buffer->HandleNewFragment_T(fs_output.__color__, screen_depth, x, y, thread_id);
                    else
                        frame_buffer->CoverFragment_T(fs_output.__color__, screen_depth, x, y);
                }
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

private:
    const std::vector<typename VS::Input>* vertex_buffer;

public:
    VS* vertex_shader = nullptr;
    FS* fragment_shader = nullptr;
};

