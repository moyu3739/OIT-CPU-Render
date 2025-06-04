#pragma once

#include <iostream>
#include <list>
#include <unordered_set>
#include <cassert>
#include "MemoryPool.h"


namespace oit {

template <int block_size, typename T>
class HiAllocator{
public:
    constexpr static int block_size = block_size;
    using Block_t = Block<block_size>;
    using MemoryPool_t = MemoryPool<block_size>;

public:
    HiAllocator() {
        assert(Block_t::capacity >= sizeof(T)); // make sure the block size is large enough
        if (Block_t::capacity / sizeof(T) < 8) {
            std::cerr << "[Warning] HiAllocator: block size is so small that can only contain "
                      << Block_t::capacity / sizeof(T) << " objects. "
                      << "Consider to increase the block size." << std::endl;
        }
    }

    HiAllocator(MemoryPool_t* memory_pool, int pre_alloc){
        this->memory_pool = memory_pool;
        FetchBlocks(pre_alloc);
    }

    HiAllocator(const HiAllocator&) = delete;
    HiAllocator& operator=(const HiAllocator&) = delete;

    ~HiAllocator(){
        SubmitAll();
    }

    // allocate memory for an object of type `T`
    // @return  the pointer to the object
    T* Allocate(){
        if (available_blocks.empty()) FetchBlocks(1); // fetch a new block from the memory pool if no available block
        Block_t* block = available_blocks.front(); // get the first available block
        T* ptr = block->template Allocate<T>(); // allocate memory for the object

        // if `block` becomes not available
        if (!BlockAvailable(block)){
            available_blocks.pop_front(); // remove `block` from `available_blocks`
            in_use_blocks.emplace(block); // add `block` to `in_use_blocks`
        }

        return ptr;
    }

    // deallocate memory for an object of type `T`
    // @param[in] ptr  the pointer to the object
    void Deallocate(T* ptr){
        // reset low bits of `ptr` to 0, to get the address of the block
        Block_t* block = reinterpret_cast<Block_t*>(reinterpret_cast<long long>(ptr) & Block_t::mask);
        // deallocate memory for the object
        block->template Deallocate<T>();

        // if this time deallocate the last object in the block,
        // the block becomes brand-new
        if (block->meta.in_use == 0){
            if (!BlockAvailable(block)){ // if `block` was in-use before deallocation
                in_use_blocks.erase(block); // remove `block` from `in_use_blocks`
                available_blocks.emplace_back(block); // add `block` to `available_blocks`
            }
            block->meta.next_pos = 0; // reset `next_pos` to 0 (=block->Reset)
        }
    }

    // deallocate all blocks forcibly 
    void DeallocateAll(){
        // deallocate the first block in `available_blocks`
        if (!available_blocks.empty()) available_blocks.front()->Reset();

        // deallocate all in-use blocks
        for (Block_t* block : in_use_blocks){
            block->Reset();
            available_blocks.emplace_back(block);
        }
        in_use_blocks.clear();
    }

private:
    // fetch `n` blocks from the memory pool
    void FetchBlocks(int n){
        for (int i = 0; i < n; i++)
            available_blocks.emplace_back(memory_pool->AllocateBlock());
        // printf("[HiAllocator]\tFetch %d blocks (%d / %d)\n", n, available_blocks.size(), available_blocks.size() + in_use_blocks.size());
    }

    // check if the block is available for allocation
    bool BlockAvailable(Block_t* block) const{
        return block->capacity - block->meta.next_pos >= sizeof(T);
    }

    // submit all blocks to the memory pool forcibly
    void SubmitAll(){
        for (Block_t* block : in_use_blocks) memory_pool->DeallocateBlock(block);
        for (Block_t* block : available_blocks) memory_pool->DeallocateBlock(block);
    }

private:
    MemoryPool_t* memory_pool;
    std::list<Block_t*> available_blocks;
    std::unordered_set<Block_t*> in_use_blocks;
};


template <class Allocator>
class AllocatorGroup {
    using MemoryPool_t = MemoryPool<Allocator::block_size>;

public:
    // @param[in] n  number of allocators
    // @param[in] pre_alloc  number of blocks pre-allocated for each allocator
    // @note  if use this constructor, this allocator group will has a exclusive memory pool,
    //        and the memory pool will be freed when the allocator group is destructed
    AllocatorGroup(int n, int pre_alloc): memory_pool_exclusive(true) {
        memory_pool = new MemoryPool_t(n * pre_alloc);
        allocators.resize(n);
        for (int i = 0; i < n; i++) allocators[i] = new Allocator(memory_pool, pre_alloc);
    }

    // @param[in] memory_pool  shared memory pool
    // @note  if use this constructor, this allocator group will use the shared memory pool,
    //        and the memory pool will NOT be freed when the allocator group is destructed
    AllocatorGroup(MemoryPool_t* memory_pool, int n, int pre_alloc)
    : memory_pool(memory_pool), memory_pool_exclusive(false) {
        allocators.resize(n);
        for (int i = 0; i < n; i++) allocators[i] = new Allocator(memory_pool, pre_alloc);
    }

    ~AllocatorGroup() {
        for (Allocator* allocator: allocators) delete allocator;
        if (memory_pool_exclusive) delete memory_pool;
    }

    AllocatorGroup(const AllocatorGroup&) = delete;
    AllocatorGroup& operator=(const AllocatorGroup&) = delete;

    // get the allocator at index `idx`
    Allocator* GetAllocator(int idx) const {
        return allocators[idx];
    }

    // deallocate all blocks forcibly
    void DeallocateAll() {
        for (Allocator* allocator: allocators) allocator->DeallocateAll();
    }

private:
    const bool memory_pool_exclusive;
    MemoryPool_t* memory_pool;
    std::vector<Allocator*> allocators;
};

} // namespace oit
