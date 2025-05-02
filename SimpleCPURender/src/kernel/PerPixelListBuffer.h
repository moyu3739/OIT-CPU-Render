#pragma once

#include <list>
#include <mutex>
#include "Primitive.h"
#include "ThreadSafeInsertList.h"
#include "HiAllocator.h"
#include "ListAllocator.h"


class PerPixelListBuffer {
protected:
    using PerPixelList = ThreadSafeInsertList<Fragment>;

public:
    PerPixelListBuffer(int width, int height, int allocator_num)
    : width(width), height(height), allocator_group(allocator_num, 64) {
        pplist_buffer = new PerPixelList[width * height];
    }

    virtual ~PerPixelListBuffer() {
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

    void InsertSortedAt_T(const Fragment& fragment, int idx, int thread_id) {
        InsertSortedList_T(fragment, pplist_buffer[idx], thread_id);
    }

    void InsertSortedAt_T(const Fragment& fragment, int x, int y, int thread_id) {
        InsertSortedList_T(fragment, pplist_buffer[y * width + x], thread_id);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int idx) const {
        BlendList(base_color, base_depth, pplist_buffer[idx]);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int x, int y) const {
        BlendList(base_color, base_depth, pplist_buffer[y * width + x]);
    }

    virtual void InsertSortedList_T(const Fragment& fragment, PerPixelList& pplist, int thread_id) const = 0;

    virtual void BlendList(glm::vec3& base_color, float base_depth, const PerPixelList& pplist) const = 0;

protected:
    const int width;
    const int height;
    PerPixelList* pplist_buffer;
    ListAllocatorGroup allocator_group;
};


class ForwardPerPixelListBuffer: public PerPixelListBuffer {
public:
    ForwardPerPixelListBuffer(int width, int height, int allocator_num)
    : PerPixelListBuffer(width, height, allocator_num) {}

    virtual ~ForwardPerPixelListBuffer() {}

    ForwardPerPixelListBuffer(const ForwardPerPixelListBuffer&) = delete;
    ForwardPerPixelListBuffer& operator=(const ForwardPerPixelListBuffer&) = delete;

    virtual void InsertSortedList_T(const Fragment& fragment, PerPixelList& pplist, int thread_id) const override {
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

    virtual void BlendList(glm::vec3& base_color, float base_depth, const PerPixelList& pplist) const override {
        // skip fragments with depth larger than `base_depth`
        auto iter = pplist.Begin();
        while (iter != pplist.End() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != pplist.End(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }
};


class BackwardPerPixelListBuffer: public PerPixelListBuffer {
public:
    // @param[in] blend_alpha_threshold  when blending, stop if the alpha value of blended fragments
    //              reaches this threshold, which means the deeper fragments will be ignored.
    BackwardPerPixelListBuffer(int width, int height, int allocator_num, float blend_alpha_threshold = 1.0f)
    : PerPixelListBuffer(width, height, allocator_num), blend_alpha_threshold(blend_alpha_threshold) {}

    virtual ~BackwardPerPixelListBuffer() {}

    BackwardPerPixelListBuffer(const BackwardPerPixelListBuffer&) = delete;
    BackwardPerPixelListBuffer& operator=(const BackwardPerPixelListBuffer&) = delete;

    virtual void InsertSortedList_T(const Fragment& fragment, PerPixelList& pplist, int thread_id) const override {
        // if `pplist` is empty or `fragment.depth` is less than the head node's depth,
        // keep trying inserting at head
        while (pplist.IsEmpty() || pplist.Begin()->depth > fragment.depth) {
            if (pplist.TryInsertHead(pplist.Begin(), fragment, allocator_group.GetAllocator(thread_id))) return;
        }

        // find the position to insert `fragment`
        auto prev = pplist.Begin();
        auto post = pplist.Begin().Next();
        while (true){
            // if `post` not at end, and `post->depth < fragment.depth`, move next
            if (post != pplist.End() && post->depth < fragment.depth){
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

    virtual void BlendList(glm::vec3& base_color, float base_depth, const PerPixelList& pplist) const override {
        // blend color
        glm::vec3 color_sum(0.0f);
        float alpha_sum = 0.0f;
        for (auto iter = pplist.Begin(); iter != pplist.End(); ++iter) {
            // blend until the depth of the fragment reaches `base_depth`,
            // or the alpha value of blended fragments reaches `blend_alpha_threshold`
            if (iter->depth > base_depth || alpha_sum > blend_alpha_threshold) break;

            // blend color in the list
            color_sum = color_sum + iter->color.a * glm::vec3(iter->color) * (1.0f - alpha_sum);
            alpha_sum = alpha_sum + iter->color.a * (1.0f - alpha_sum);
        }

        // blend base color
        base_color = color_sum + base_color * (1.0f - alpha_sum); // alpha value of base color is 1.0f
    }

private:
    float blend_alpha_threshold;
};

