#pragma once

#include <mutex>
#include "FrameBuffer.h"
#include "ListAllocator.h"


class FrameBufferManager {
public:
    FrameBufferManager() {}

    virtual ~FrameBufferManager() {}

    FrameBufferManager(const FrameBufferManager&) = delete;
    FrameBufferManager& operator=(const FrameBufferManager&) = delete;

    virtual int GetBufferNumber() const = 0;

    // rotate buffer
    virtual void RotateBuffer() = 0;

    // get front buffer
    virtual FrameBuffer* GetFrontBuffer() const = 0;

    // get back buffer
    virtual FrameBuffer* GetBackBuffer() const = 0;

    // get buffer at index `idx`
    // @param[in] idx  index of the buffer, 0 for backmost buffer, maximum value for frontmost buffer
    virtual FrameBuffer* GetBufferAt(int idx) const = 0;
};


// maintain `N` frame buffer,
// index at 0 for backmost buffer, N-1 for frontmost buffer
template <int N>
class NframeBufferManager: public FrameBufferManager {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] allocator_num  number of allocators for per-pixel linked list buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    NframeBufferManager(int width, int height, int allocator_num,
                        const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
                        bool enable_oit = false,
                        bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f) {
        for (int i = 0; i < N; i++) {
            buffers[i] = new FrameBuffer(width, height, allocator_num, bg_color, bg_depth, enable_oit,
                                         use_backward_pplist, backward_blend_alpha_threshold);
        }
    }

    virtual ~NframeBufferManager() {
        for (int i = 0; i < N; i++) delete buffers[i];
    }

    NframeBufferManager(const NframeBufferManager&) = delete;
    NframeBufferManager& operator=(const NframeBufferManager&) = delete;

    virtual int GetBufferNumber() const override {
        return N;
    }

    // rotate buffer
    virtual void RotateBuffer() override {
        auto tmp = buffers[N - 1];
        for (int i = N - 1; i > 0; i--) buffers[i] = buffers[i - 1];
        buffers[0] = tmp;
    }

    // get front buffer
    virtual FrameBuffer* GetFrontBuffer() const override {
        return buffers[N - 1];
    }

    // get back buffer
    virtual FrameBuffer* GetBackBuffer() const override {
        return buffers[0];
    }

    // get buffer at index `idx`
    // @param[in] idx  index of the buffer, 0 for backmost buffer, N-1 for frontmost buffer
    virtual FrameBuffer* GetBufferAt(int idx) const override {
        return buffers[idx];
    }

protected:
    FrameBuffer* buffers[N]; // N buffers, 0 for backmost buffer, N-1 for frontmost buffer
};


using SingleFrameBufferManager = NframeBufferManager<1>;


// maintain front and back frame buffer
class DoubleFrameBufferManager: public NframeBufferManager<2> {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] allocator_num  number of allocators for per-pixel linked list buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    DoubleFrameBufferManager(int width, int height, int allocator_num,
                             const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
                             bool enable_oit = false,
                             bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f)
    : NframeBufferManager<2>(width, height, allocator_num, bg_color, bg_depth, enable_oit,
                             use_backward_pplist, backward_blend_alpha_threshold) {}

    virtual ~DoubleFrameBufferManager() {}

    DoubleFrameBufferManager(const DoubleFrameBufferManager&) = delete;
    DoubleFrameBufferManager& operator=(const DoubleFrameBufferManager&) = delete;

    // swap front and back buffer, actually alias of `RotateBuffer` for 2-frame buffer manager
    void SwapBuffer() {
        RotateBuffer();
    }
};


// maintain render, load and clear frame buffer
class TripleFrameBufferManager: public NframeBufferManager<3> {
public:
    // @param[in] width  width of the frame buffer
    // @param[in] height  height of the frame buffer
    // @param[in] allocator_num  number of allocators for per-pixel linked list buffer
    // @param[in] enable_oit  whether to enable order-independent transparency;
    //              if false, the frame buffer will always ignore alpha channel,
    //              which means fragment will be covered whenever it is in front of the existing fragment.
    // @param[in] use_backward_pplist  if true, the frame buffer will use backward per-pixel linked list buffer;
    //              if false, the frame buffer will use forward per-pixel linked list buffer.
    //              (only valid when `enable_oit` is true)
    // @param[in] backward_blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    //              (only valid when `enable_oit` is true and `use_backward_pplist` is true)
    TripleFrameBufferManager(int width, int height, int allocator_num,
                             const glm::vec3& bg_color = glm::vec3(0.0f), float bg_depth = INFINITY,
                             bool enable_oit = false,
                             bool use_backward_pplist = false, float backward_blend_alpha_threshold = 1.0f)
    : NframeBufferManager<3>(width, height, allocator_num, bg_color, bg_depth, enable_oit,
                             use_backward_pplist, backward_blend_alpha_threshold) {}

    virtual ~TripleFrameBufferManager() {}

    TripleFrameBufferManager(const TripleFrameBufferManager&) = delete;
    TripleFrameBufferManager& operator=(const TripleFrameBufferManager&) = delete;

    // get load buffer
    FrameBuffer* GetMidBuffer() const {
        return buffers[1];
    }
};

