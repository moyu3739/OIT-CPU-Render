#pragma once

#include <iostream>
#include <list>
#include <exception>
#include <mutex>
#include <cassert>
#include "Block.h"


template <int block_size>
class MemoryPool {
    using Block_t = Block<block_size>;
    
public:
    // @param[in] pre_alloc  the number of blocks to pre-allocate
    MemoryPool(int pre_alloc){
        if (pre_alloc < 1) pre_alloc = 1;
        this->pre_alloc = pre_alloc;
        Expand(pre_alloc);
    }

    ~MemoryPool(){
        // free all available blocks
        for (Block_t* block : available_blocks) FreeBlock(block);
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // allocate a block
    Block_t* AllocateBlock(){
        std::lock_guard<std::mutex> lock(mtx);

        // double-expand `available_blocks` if no available block
        if (available_blocks.empty()) DoubleExpand();

        // pop the first block from the available list, and return it
        Block_t* block = available_blocks.front();
        available_blocks.pop_front();
        block->Reset();
        return block;
    }

    // deallocate a block
    // @param[in] block  the block to deallocate
    void DeallocateBlock(Block_t* block){
        std::lock_guard<std::mutex> lock(mtx);

        // push `block` to the available list
        available_blocks.emplace_back(block);

        // half-shrink `available_blocks` if use-rate is less than 1/4
        if (total_blocks > pre_alloc // make sure the total number of blocks is at least `pre_alloc`
            && available_blocks.size() >  3 * total_blocks / 4) HalfShrink();
    }

protected:
    // allocate a new block
    // @return the address of the new block
    // @note the new block is aligned with the size of the block (4KB)
    static Block_t* NewBlock(){
        Block_t* ptr = reinterpret_cast<Block_t*>(_aligned_malloc(block_size, block_size));
        if (ptr == nullptr) throw std::bad_alloc();
        return ptr;
    }

    // free a block
    // @param[in] block  the address of the block to free
    static void FreeBlock(Block_t* block){
        _aligned_free(block);
    }

    // expand the list of available blocks
    // @param[in] n  the number of blocks to expand
    void Expand(int n){        
        for(int i = 0; i < n; i++) available_blocks.emplace_back(NewBlock());
        total_blocks += n;
    }

    // double-expand the list of available blocks
    void DoubleExpand(){
        Expand(total_blocks);
        std::clog << "[Info] MemoryPool: Double-expanded to " << total_blocks << " blocks." << std::endl;
    }

    // shrink the list of available blocks
    // @param[in] n  the number of blocks to shrink
    void Shrink(int n){
        for(int i = 0; i < n; i++){
            Block_t* block = available_blocks.back();
            available_blocks.pop_back();
            FreeBlock(block);
        }
        total_blocks -= n;
    }

    // half-shrink the list of available blocks
    void HalfShrink(){
        Shrink(total_blocks / 2);
        std::clog << "[Info] MemoryPool: Half-shrinked to " << total_blocks << " blocks." << std::endl;
    }

protected:
    int pre_alloc; // the number of blocks to pre-allocate
    int total_blocks = 0; // the total number of blocks (both available and in-use)
    std::list<Block_t*> available_blocks;
    std::mutex mtx;
};
