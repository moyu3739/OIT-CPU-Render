#pragma once

#include "Primitive.h"
#include "ThreadSafeInsertList.h"
#include "MemoryPool.h"
#include "HiAllocator.h"


template <int block_size, typename T>
class BatchFreeAllocator: public HiAllocator<block_size, T> {
    using HiAllocator_t = HiAllocator<block_size, T>;
    using MemoryPool_t = MemoryPool<block_size>;

public:
    BatchFreeAllocator() {}

    BatchFreeAllocator(MemoryPool_t* memory_pool, int pre_alloc)
        : HiAllocator_t(memory_pool, pre_alloc) {}

    BatchFreeAllocator(const BatchFreeAllocator&) = delete;
    BatchFreeAllocator& operator=(const BatchFreeAllocator&) = delete;

    void Deallocate(T* ptr) {} // do nothing
};


constexpr int block_size = 1024*1024; // 1 MB block
using ListNode = ThreadSafeInsertListNode<Fragment>;
using ListAllocator = BatchFreeAllocator<block_size, ListNode>;
using ListAllocatorGroup = AllocatorGroup<ListAllocator>;

