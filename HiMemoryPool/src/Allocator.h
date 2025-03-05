#pragma once

#include <iostream>
#include <list>
#include <unordered_set>
#include "MemoryPool.h"


template <int block_size, class T>
class _Allocator{
    using Block_t = _Block<block_size>;
    using MemoryPool_t = _MemoryPool<block_size>;

public:
    _Allocator(MemoryPool_t* memory_pool, int pre_alloc){
        this->memory_pool = memory_pool;
        FetchBlocks(pre_alloc);
    }

    ~_Allocator(){
        SubmitAll();
    }

    // allocate memory for an object of type `T`
    // @return the pointer to the object
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
        available_blocks.front()->Reset();

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
        // printf("[Allocator]\tFetch %d blocks (%d / %d)\n", n, available_blocks.size(), available_blocks.size() + in_use_blocks.size());
    }

    // check if the block is available for allocation
    bool BlockAvailable(Block_t* block) const{
        return block->capacity - block->meta.next_pos >= sizeof(T);
    }

    // submit all blocks to the memory pool forcibly
    void SubmitAll(){
        for (Block_t* block : in_use_blocks)
            memory_pool->DeallocateBlock(block);
        for (Block_t* block : available_blocks)
            memory_pool->DeallocateBlock(block);
    }

private:
    MemoryPool_t* memory_pool;
    std::list<Block_t*> available_blocks;
    std::unordered_set<Block_t*> in_use_blocks;
};