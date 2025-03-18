#pragma once

#include <list>
#include <mutex>
#include "Primitive.h"
#include "ThreadSafeInsertList.h"
#include "MemoryPool.h"
#include "Allocator.h"


class PerPixelListBuffer_MTX {
public:
    PerPixelListBuffer_MTX(int width, int height): width(width), height(height) {
        pplist_buffer = new std::list<Fragment>[width * height];
        buffer_mtx = new std::mutex[width * height];
    }

    ~PerPixelListBuffer_MTX() {
        delete[] pplist_buffer;
        delete[] buffer_mtx;
    }

    void Clear() {
        for (int i = 0; i < width * height; i++) pplist_buffer[i].clear();
    }

    template <class Allocator>
    void InsertSortedAt_T(const Fragment& fragment, int x, int y, Allocator* allocator = nullptr) {
        InsertSortedAt_T(fragment, y * width + x);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int x, int y) const {
        BlendAt(base_color, base_depth, y * width + x);
    }

    template <class Allocator>
    void InsertSortedAt_T(const Fragment& fragment, int idx, Allocator* allocator = nullptr) {
        auto& pplist = pplist_buffer[idx];

        std::lock_guard<std::mutex> lock(buffer_mtx[idx]);
        auto iter = pplist.begin();
        while (iter != pplist.end() && iter->depth > fragment.depth) ++iter;
        pplist.insert(iter, fragment);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int idx) const {
        const auto& pplist = pplist_buffer[idx];
        // skip fragments with depth larger than `base_depth`
        auto iter = pplist.begin();
        while (iter != pplist.end() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != pplist.end(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }

private:
    const int width;
    const int height;
    std::list<Fragment>* pplist_buffer;
    std::mutex* buffer_mtx;
};





class PerPixelListBuffer_CAS {
public:
    constexpr static int block_size = 4096;
    using Node = ThreadSafeInsertListNode<Fragment>;
    using MemoryPool = MemoryPool<block_size>;

    // using Allocator = TrivialAllocator<Node>;
    using Allocator = BatchFreeAllocator<block_size, Node>;
    using PerPixelList = ThreadSafeInsertList<Fragment, Allocator>;

public:
    PerPixelListBuffer_CAS(int width, int height, int allocator_num): width(width), height(height) {
        InitAllocator(allocator_num);
        pplist_buffer = new PerPixelList[width * height];

        // allocator_test = new Allocator;
    }

    ~PerPixelListBuffer_CAS() {
        Clear();
        // delete allocator_test;
        delete[] pplist_buffer;
        FreeAllocator();
    }

    void InitAllocator(int allocator_num) {
        memory_pool = new MemoryPool(64);
        allocators.resize(allocator_num);
        for (int i = 0; i < allocator_num; i++) allocators[i] = new Allocator(memory_pool, 32);
    }

    Allocator* GetAllocator(int idx) {
        return allocators[idx];
    }

    void FreeAllocator() {
        for (Allocator* allocator : allocators) delete allocator;
        delete memory_pool;
    }

    // clear all per-pixel linked lists
    // @note free all nodes in lists, and reset head pointers to nullptr
    void Clear() {
        // for (int i = 0; i < width * height; i++) pplist_buffer[i].Clear(allocators[0]);
        for (Allocator* allocator: allocators) allocator->DeallocateAll();
        for (int i = 0; i < width * height; i++) pplist_buffer[i].ResetHead();
    }

    template <class Allocator>
    void InsertSortedAt_T(const Fragment& fragment, int x, int y, Allocator* allocator) {
        InsertSortedAt_T(fragment, y * width + x, allocator);
    }

    void BlendAt(glm::vec3& base_color, float base_depth, int x, int y) const {
        BlendAt(base_color, base_depth, y * width + x);
    }

    template <class Allocator>
    void InsertSortedAt_T(const Fragment& fragment, int idx, Allocator* allocator) {
        auto& pplist = pplist_buffer[idx];

        // if `pplist` is empty or `fragment.depth` is larger than the head node's depth,
        // keep trying inserting at head
        while (pplist.IsEmpty() || pplist.Begin()->depth < fragment.depth) {
            if (pplist.TryInsertHead(pplist.Begin(), fragment, allocator)) return;
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
            bool res = pplist.TryInsertAt(prev, post, fragment, allocator);
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

    MemoryPool* memory_pool;
    std::vector<Allocator*> allocators;
};

