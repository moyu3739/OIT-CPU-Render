#pragma once

#include <mutex>
#include "FrameBuffer.h"
#include "ListAllocator.h"


// maintain front and back frame buffer
class DoubleFrameBufferManager {
public:
    DoubleFrameBufferManager(int width, int height, int allocator_num, bool enable_oit = false) {
        front_buffer = new FrameBuffer(width, height, allocator_num, enable_oit);
        back_buffer  = new FrameBuffer(width, height, allocator_num, enable_oit);
    }

    ~DoubleFrameBufferManager() {
        delete front_buffer;
        delete back_buffer;
    }

    DoubleFrameBufferManager(const DoubleFrameBufferManager&) = delete;
    DoubleFrameBufferManager& operator=(const DoubleFrameBufferManager&) = delete;

    // swap front and back buffer
    void SwapBuffer() {
        std::swap(front_buffer, back_buffer);
    }

    // get front buffer
    FrameBuffer* GetFrontBuffer() const {
        return front_buffer;
    }

    // get back buffer
    FrameBuffer* GetBackBuffer() const {
        return back_buffer;
    }

private:
    FrameBuffer* front_buffer;
    FrameBuffer* back_buffer;
};



// maintain render, load and clear frame buffer
class TripleFrameBufferManager {
public:
    TripleFrameBufferManager(int width, int height, int allocator_num, bool enable_oit = false) {
        back_buffer  = new FrameBuffer(width, height, allocator_num, enable_oit);
        mid_buffer   = new FrameBuffer(width, height, allocator_num, enable_oit);
        front_buffer = new FrameBuffer(width, height, allocator_num, enable_oit);
    }

    ~TripleFrameBufferManager() {
        delete back_buffer;
        delete mid_buffer;
        delete front_buffer;
    }

    TripleFrameBufferManager(const TripleFrameBufferManager&) = delete;
    TripleFrameBufferManager& operator=(const TripleFrameBufferManager&) = delete;

    // rotate buffer
    void RotateBuffer() {
        auto tmp = front_buffer;
        front_buffer = mid_buffer;
        mid_buffer = back_buffer;
        back_buffer = tmp;
    }

    // get render buffer
    FrameBuffer* GetBackBuffer() const {
        return back_buffer;
    }

    // get load buffer
    FrameBuffer* GetMidBuffer() const {
        return mid_buffer;
    }

    // get clear buffer
    FrameBuffer* GetFrontBuffer() const {
        return front_buffer;
    }

private:
    FrameBuffer* back_buffer;
    FrameBuffer* mid_buffer;
    FrameBuffer* front_buffer;
};



// maintain `N` frame buffer,
// index at 0 for backmost buffer, N-1 for frontmost buffer
template <int N>
class NbufferManager {
public:
    NbufferManager(int width, int height, int allocator_num, bool enable_oit = false) {
        for (int i = 0; i < N; i++)
            buffers[i] = new FrameBuffer(width, height, allocator_num, enable_oit);
    }

    ~NbufferManager() {
        for (int i = 0; i < N; i++) delete buffers[i];
    }

    NbufferManager(const NbufferManager&) = delete;
    NbufferManager& operator=(const NbufferManager&) = delete;

    // rotate buffer
    void RotateBuffer() {
        auto tmp = buffers[N - 1];
        for (int i = N - 1; i > 0; i--) buffers[i] = buffers[i - 1];
        buffers[0] = tmp;
    }

    // get buffer
    FrameBuffer* GetBufferAt(int idx) const {
        return buffers[idx];
    }

private:
    FrameBuffer* buffers[N]; // N buffers, 0 for backmost buffer, N-1 for frontmost buffer
};


using FrameBufferManager = DoubleFrameBufferManager;

