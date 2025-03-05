#pragma once

#include <cassert>
#include <unordered_map>
#include "Allocator.h"


template <class T>
class AllocatorTester{
public:
    AllocatorTester() {}

    // ADD
    // called after allocating memory for an object of type `T`
    void RecordAllocate(T* ptr){
        assert(allocated_table.find(ptr) == allocated_table.end()); // before allocation, `ptr` must not in `allocated_table`
        allocated_table[ptr] = *ptr;
    }
    
    // DELETE
    // called when deallocating memory for `ptr` of type `T`
    void RecordDeallocate(T* ptr){
        assert(allocated_table.find(ptr) != allocated_table.end()); // before deallocation, `ptr` must in `allocated_table`
        allocated_table.erase(ptr);
    }

    // BATCH DELETE
    // called when deallocating all the allocated memory
    void RecordDeallocateAll(){
        allocated_table.clear();
    }

    // SUBMIT
    // called when allocator is destructed
    void RecordDestruct(){
        allocated_table.clear();
    }

    // MODIFY
    // modify the memory for `ptr` of type `T`
    void Modify(T* ptr, const T& new_value){
        assert(allocated_table.find(ptr) != allocated_table.end()); // when modification, `ptr` must in `allocated_table`
        *ptr = new_value;
        allocated_table[ptr] = new_value;
    }

    // CHECK
    // chech whether what `ptr` points at is equal to what `allocated_table` records
    void Check(T* ptr) const{
        assert(allocated_table.find(ptr) != allocated_table.end()); // when checking, `ptr` must in `allocated_table`
        assert(*ptr == allocated_table.at(ptr)); // chech whether what `ptr` points at is equal to what `allocated_table` records
    }

    // CHECK ALL
    // check whether all the allocated memory is equal to what `allocated_table` records
    void CheckAll() const{
        for(auto& pair : allocated_table){
            assert(*pair.first == pair.second);
        }
    }


public:
    std::unordered_map<T*, T>allocated_table;
};


