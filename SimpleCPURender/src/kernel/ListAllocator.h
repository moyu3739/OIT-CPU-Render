#pragma once

#include <vector>
#include "Primitive.h"
#include "ThreadSafeInsertList.h"
#include "HiMemoryPool.h"
#include "HiAllocator.h"


template <int block_size, class T>
class BatchFreeAllocator {
public:
    constexpr static int block_size = block_size;
    using MemoryPool_t = MemoryPool<block_size>;

public:
    BatchFreeAllocator() {}

    BatchFreeAllocator(MemoryPool_t* memory_pool, int pre_alloc)
        : allocator(memory_pool, pre_alloc) {}

    BatchFreeAllocator(const BatchFreeAllocator&) = delete;
    BatchFreeAllocator& operator=(const BatchFreeAllocator&) = delete;

    void Init(MemoryPool_t* memory_pool, int pre_alloc) {
        allocator.Init(memory_pool, pre_alloc);
    }

    T* Allocate() {
        return allocator.Allocate();
    }

    void Deallocate(T* ptr) {} // do nothing

    // deallocate all blocks forcibly
    void DeallocateAll() {
        allocator.DeallocateAll();
    }

private:
    HiAllocator<block_size, T> allocator;
};


constexpr static int block_size = 4096;
using ListNode = ThreadSafeInsertListNode<Fragment>;
using ListAllocator = BatchFreeAllocator<block_size, ListNode>;
using ListAllocatorGroup = AllocatorGroup<ListAllocator>;

