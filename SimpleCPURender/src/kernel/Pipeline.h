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
#include "Shader.h"
#include "TriangleTraversal.h"
#include "FrameBuffer.h"


class Pipeline {
public:
    Pipeline(int thread_num): thread_num(thread_num), shader_io_group(thread_num) {}

    Pipeline(VertexShader* vertex_shader, FragmentShader* fragment_shader,
             TriangleTraversal* triangle_traversal, int thread_num):
    vertex_shader(vertex_shader), fragment_shader(fragment_shader), triangle_traversal(triangle_traversal),
    thread_num(thread_num), shader_io_group(thread_num, vertex_shader, fragment_shader) {}

    Pipeline(VertexShader* vertex_shader, FragmentShader* fragment_shader,
             TriangleTraversalType tt_type, int thread_num):
    vertex_shader(vertex_shader), fragment_shader(fragment_shader), thread_num(thread_num),
    shader_io_group(thread_num, vertex_shader, fragment_shader) {
        static TriangleTraversalFace face_triange_traversal;
        static TriangleTraversalEdge edge_triangle_traversal;
        static TriangleTraversalVertex vertex_triangle_traversal;
        
        switch(tt_type) {
            case ON_FACE: triangle_traversal = &face_triange_traversal; break;
            case ON_EDGE: triangle_traversal = &edge_triangle_traversal; break;
            case ON_VERTEX: triangle_traversal = &vertex_triangle_traversal; break;
        }
    }

    void BoundShader(VertexShader* vertex_shader, FragmentShader* fragment_shader){
        this->vertex_shader = vertex_shader;
        this->fragment_shader = fragment_shader;
        shader_io_group.BoundShader(vertex_shader, fragment_shader);
    }

    void BoundTriangleTraversal(TriangleTraversal* triangle_traversal){
        this->triangle_traversal = triangle_traversal;
    }

    void BoundVertexBuffer(const VertexBuffer& vertex_buffer){
        this->vertex_buffer = &vertex_buffer;
    }

    int GetThreadNumber() const {
        return thread_num;
    }

    const VertexBuffer& GetVertexBuffer() const {
        return *vertex_buffer;
    }

    VertexShader* GetVertexShader() const {
        return vertex_shader;
    }

    FragmentShader* GetFragmentShader() const {
        return fragment_shader;
    }

    TriangleTraversal* GetTriangleTraversal() const {
        return triangle_traversal;
    }

    // render the pipeline
    // @param[in] frame_buffer  frame buffer
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    void Render(FrameBuffer* frame_buffer, bool use_oit = false) {
        // RenderSlice(frame_buffer, use_oit);
        RenderCounter(frame_buffer, use_oit);
    }

    // render the pipeline,
    // using paralleling method of trivial slicing up vertex buffer equally
    // @param[in] frame_buffer  frame buffer
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    void RenderSlice(FrameBuffer* frame_buffer, bool use_oit = false) {
        assert(vertex_buffer->size() % 3 == 0);

        // if single thread, do it directly
        if (thread_num == 1) {
            if (use_oit) RenderProcessSlice<true>(0, vertex_buffer->size(), frame_buffer, 0);
            else RenderProcessSlice<false>(0, vertex_buffer->size(), frame_buffer, 0);
            return;
        }

        int f_num = vertex_buffer->size() / 3;
        std::vector<int> split_points = ut::RangeSlice(0, f_num, thread_num);
        std::vector<std::thread> threads;
        if (use_oit) { // use OIT
            for (int i = 0; i < thread_num; i++){
                int v_begin = split_points[i] * 3;
                int v_end = split_points[i + 1] * 3;
                threads.emplace_back(&Pipeline::RenderProcessSlice<true>, this, v_begin, v_end, frame_buffer, i);
            }
        }
        else { // not use OIT
            for (int i = 0; i < thread_num; i++){
                int v_begin = split_points[i] * 3;
                int v_end = split_points[i + 1] * 3;
                threads.emplace_back(&Pipeline::RenderProcessSlice<false>, this, v_begin, v_end, frame_buffer, i);
            }
        }

        for (std::thread& thread: threads) thread.join();
    }

    // render the pipeline,
    // using paralleling method of atomic counter, fetching from vertex buffer by threads
    // @param[in] frame_buffer  frame buffer
    // @param[in] use_oit  whether to use OIT; if false, alpha channel will be ignored when rendering
    void RenderCounter(FrameBuffer* frame_buffer, bool use_oit = false) {
        assert(vertex_buffer->size() % 3 == 0);

        // if single thread, do it directly
        if (thread_num == 1) {
            if (use_oit) RenderProcessSlice<true>(0, vertex_buffer->size(), frame_buffer, 0);
            else RenderProcessSlice<false>(0, vertex_buffer->size(), frame_buffer, 0);
            return;
        }

        std::atomic<int> counter(0);
        std::vector<std::thread> threads;
        if (use_oit) { // use OIT
            for (int i = 0; i < thread_num; i++){
                threads.emplace_back(&Pipeline::RenderProcessCounter<true>, this, &counter, frame_buffer, i);
            }
        }
        else { // not use OIT
            for (int i = 0; i < thread_num; i++){
                threads.emplace_back(&Pipeline::RenderProcessCounter<false>, this, &counter, frame_buffer, i);
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
    void RenderProcessSlice(int v_begin, int v_end, FrameBuffer* frame_buffer, int thread_id) {
        for (int i = v_begin; i < v_end; i += 3){
            RenderTriangle<use_oit>(i, frame_buffer, thread_id);
        }
    }

    // render the pipeline, vertices fetched atomically and orderly by threads themselves
    // @param[in] counter  atomic counter for fetching vertices
    // @param[in] frame_buffer  frame buffer
    // @param[in] thread_id  thread id (0, 1, 2, ..., thread_num - 1)
    template <bool use_oit = false>
    void RenderProcessCounter(std::atomic<int>* counter, FrameBuffer* frame_buffer, int thread_id) {
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
        int width = frame_buffer->width;
        int height = frame_buffer->height;

        const auto* vs_input_v0 = vertex_buffer->at(idx);
        const auto* vs_input_v1 = vertex_buffer->at(idx + 1);
        const auto* vs_input_v2 = vertex_buffer->at(idx + 2);
        auto** vs_output = shader_io_group.GetAt(thread_id)->vs_output;
        auto* fs_input  = shader_io_group.GetAt(thread_id)->fs_input;
        auto* fs_output = shader_io_group.GetAt(thread_id)->fs_output;

        // call vertex-shader
        vertex_shader->Call(*vs_input_v0, *vs_output[0]);
        vertex_shader->Call(*vs_input_v1, *vs_output[1]);
        vertex_shader->Call(*vs_input_v2, *vs_output[2]);

        // save w for perspective division
        float w0 = vs_output[0]->__position__.w;
        float w1 = vs_output[1]->__position__.w;
        float w2 = vs_output[2]->__position__.w;

        // map to clipping space, where (-1, 1) is visible region
        glm::vec4 screen_pos_v0 = vs_output[0]->__position__ / w0;
        glm::vec4 screen_pos_v1 = vs_output[1]->__position__ / w1;
        glm::vec4 screen_pos_v2 = vs_output[2]->__position__ / w2;

        // calculate bounding box
        int pixel_min_x = ut::Screen2Pixel(std::min(screen_pos_v0.x, std::min(screen_pos_v1.x, screen_pos_v2.x)), width);
        int pixel_max_x = ut::Screen2Pixel(std::max(screen_pos_v0.x, std::max(screen_pos_v1.x, screen_pos_v2.x)), width);
        int pixel_min_y = ut::Screen2Pixel(std::min(screen_pos_v0.y, std::min(screen_pos_v1.y, screen_pos_v2.y)), height);
        int pixel_max_y = ut::Screen2Pixel(std::max(screen_pos_v0.y, std::max(screen_pos_v1.y, screen_pos_v2.y)), height);

        // clip bounding box
        pixel_min_x = std::max(0, pixel_min_x);
        pixel_max_x = std::min(width - 1, pixel_max_x);
        pixel_min_y = std::max(0, pixel_min_y);
        pixel_max_y = std::min(height - 1, pixel_max_y);

        // rasterization
        for (int x = pixel_min_x; x <= pixel_max_x; x++){
            for (int y = pixel_min_y; y <= pixel_max_y; y++){
                // map pixel to clipping space, where (-1, 1) is visible region
                float screen_x = ut::Pixel2Screen(x, width);
                float screen_y = ut::Pixel2Screen(y, height);

                // barycentric coordinates
                glm::vec3 barycentric= glm::inverse(
                    glm::mat3x3(screen_pos_v0.x, screen_pos_v0.y, 1.0f,
                                screen_pos_v1.x, screen_pos_v1.y, 1.0f,
                                screen_pos_v2.x, screen_pos_v2.y, 1.0f)
                )
                * glm::vec3(screen_x, screen_y, 1.0f) / glm::vec3(w0, w1, w2); // perspective division

                // clip triangle (decide whether to draw the pixel)
                if (!triangle_traversal->Call(
                    width, height, x, y, screen_x, screen_y, barycentric,
                    screen_pos_v0, screen_pos_v1, screen_pos_v2
                )) {
                    continue;
                }

                // perspective-correct interpolation
                fragment_shader->Interpolate(
                    *vs_output[0], *vs_output[1], *vs_output[2], barycentric, *fs_input);
                float screen_depth = fragment_shader->InterpolateAttr(
                    screen_pos_v0.z, screen_pos_v1.z, screen_pos_v2.z, barycentric);

                // depth clipping
                if (screen_depth < -1.0f || screen_depth > 1.0f) continue;

                // write color to frame buffer
                if (frame_buffer->DepthTestAt(screen_depth, x, y)){ // get max depth, note that z-axis points out of the screen
                    // call fragment-shader
                    fragment_shader->Call(*fs_input, *fs_output);

                    if constexpr (use_oit)
                        frame_buffer->InsertFragment_T(fs_output->__color__, screen_depth, x, y, thread_id);
                    else
                        frame_buffer->CoverFragment_T(fs_output->__color__, screen_depth, x, y);
                }
            }
        }
    }

private:
    const int thread_num;
    const VertexBuffer* vertex_buffer;
    ShaderIOGroup shader_io_group;
    VertexShader* vertex_shader = nullptr;
    FragmentShader* fragment_shader = nullptr;
    TriangleTraversal* triangle_traversal = nullptr;
};

