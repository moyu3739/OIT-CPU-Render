#pragma once

#include <list>
#include <mutex>
#include "Primitive.h"
#include "ThreadSafeInsertList.h"
#include "HiAllocator.h"
#include "ListAllocator.h"


class PerPixelListBuffer {
private:
    using PerPixelList = ThreadSafeInsertList<Fragment>;

public:
    PerPixelListBuffer(int width, int height, int allocator_num)
    : width(width), height(height), allocator_group(allocator_num, 64) {
        pplist_buffer = new PerPixelList[width * height];
    }

    ~PerPixelListBuffer() {
        Clear();
        delete[] pplist_buffer;
    }

    PerPixelListBuffer(const PerPixelListBuffer&) = delete;
    PerPixelListBuffer& operator=(const PerPixelListBuffer&) = delete;

    // clear all per-pixel linked lists
    // @note free all nodes in lists, and reset head pointers to nullptr
    void Clear() {
        allocator_group.DeallocateAll();
        for (int i = 0; i < width * height; i++) pplist_buffer[i].ResetHead();
    }

    void InsertSortedAt_T(const Fragment& fragment, int x, int y, int thread_id) {
        InsertSortedAt_T(fragment, y * width + x, thread_id);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int x, int y) const {
        BlendAt(base_color, base_depth, y * width + x);
    }

    void InsertSortedAt_T(const Fragment& fragment, int idx, int thread_id) {
        auto& pplist = pplist_buffer[idx];

        // if `pplist` is empty or `fragment.depth` is larger than the head node's depth,
        // keep trying inserting at head
        while (pplist.IsEmpty() || pplist.Begin()->depth < fragment.depth) {
            if (pplist.TryInsertHead(pplist.Begin(), fragment, allocator_group.GetAllocator(thread_id))) return;
        }

        // find the position to insert `fragment`
        auto prev = pplist.Begin();
        auto post = pplist.Begin().Next();
        while (true){
            // if `post` not at end, and `post->depth > fragment.depth`, move next
            if (post != pplist.End() && post->depth > fragment.depth){
                prev = post;
                ++post;
                continue;
            }
            // otherwise insert `fragment` between `prev` and `post`
            bool res = pplist.TryInsertAt(prev, post, fragment, allocator_group.GetAllocator(thread_id));
            if (res) return;
            else{
                post = prev.Next(); // get the next node again
                continue; // retry
            }
        }
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int idx) const {
        const auto& pplist = pplist_buffer[idx];

        // skip fragments with depth larger than `base_depth`
        auto iter = pplist.Begin();
        while (iter != pplist.End() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != pplist.End(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }

private:
    const int width;
    const int height;
    PerPixelList* pplist_buffer;
    ListAllocatorGroup allocator_group;
};

