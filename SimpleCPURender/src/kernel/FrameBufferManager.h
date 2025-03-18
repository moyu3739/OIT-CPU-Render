#pragma once

#include <mutex>
#include "FrameBuffer.h"
#include "ListAllocator.h"


// maintain front and back frame buffer
class FrameBufferManager {
public:
    FrameBufferManager(int width, int height, int allocator_num, bool enable_oit = false) {
        front_buffer = new FrameBuffer(width, height, allocator_num, enable_oit);
        back_buffer  = new FrameBuffer(width, height, allocator_num, enable_oit);
    }

    ~FrameBufferManager() {
        delete front_buffer;
        delete back_buffer;
    }

    FrameBufferManager(const FrameBufferManager&) = delete;
    FrameBufferManager& operator=(const FrameBufferManager&) = delete;

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

