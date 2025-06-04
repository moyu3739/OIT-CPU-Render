#include <iostream>
#include "Timer.h"
#include "MemoryPool.h"
#include "HiAllocator.h"

using namespace oit;


constexpr int block_size_test = 4096;
using MemoryPool_4K = MemoryPool<block_size_test>;
template <class T>
using HiAllocator_4K = HiAllocator<block_size_test, T>;

////////////////////////////////////////////////////////////////////////////////////////////////
// Test 0
// 测试分配器的性能对比（与 c++ 标准分配器对比）
////////////////////////////////////////////////////////////////////////////////////////////////
inline void TestAllocator0() {
    printf("======== TEST 0 ========\n");
    using Td = int;
    MemoryPool_4K mp(10000);
    HiAllocator_4K<Td> allocator(&mp, 10);

    const int N = 10000000;
    Td** arr = new Td*[N];
    Timer tm;
    
    printf("---- std::allocator ----\n");
    tm.StartTimer();
    for (int i = 0; i < N; i++) arr[i] = new Td;
    printf("Allocate: %.4f s\n", tm.RecordTimer());
    for (int i = 0; i < N; i++) delete arr[i];
    printf("Deallocate: %.4f s\n", tm.RecordTimer() - tm.records[1]);
    printf("Allocate + Deallocate: %.4f s\n\n", tm.records[2]);

    printf("---- My HiAllocator ----\n");
    tm.StartTimer();
    for (int i = 0; i < N; i++) arr[i] = allocator.Allocate();
    printf("Allocate: %.4f s\n", tm.RecordTimer());
    // for (int i = N - 1; i >= 0; i--) allocator.Deallocate(arr[i]); // deallocate one by one
    allocator.DeallocateAll(); // deallocate all at once
    printf("Deallocate: %.4f s\n", tm.RecordTimer() - tm.records[1]);
    printf("Allocate + Deallocate: %.4f s\n\n", tm.records[2]);

    delete[] arr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Test 1
// 测试自定义结构
////////////////////////////////////////////////////////////////////////////////////////////////
#include <cassert>

struct TestStruct {
    int x;
    double y;
};

inline void TestAllocator1() {
    printf("======== TEST 1 ========\n");
    // 1. 创建一个内存池
    MemoryPool_4K mp(10); // 预分配10个块，每块4KB

    // 2. 创建一个分配器，分配类型为TestStruct
    HiAllocator_4K<TestStruct> alloc(&mp, 2);

    // 3. 测试单个对象的分配与释放
    TestStruct* obj = alloc.Allocate();
    assert(obj != nullptr);
    obj->x = 10;
    obj->y = 3.14;
    assert(obj->x == 10 && obj->y == 3.14);
    alloc.Deallocate(obj);

    // 4. 批量分配、使用与释放
    const int batchSize = 1000;
    TestStruct** arr = new TestStruct*[batchSize];
    for(int i = 0; i < batchSize; i++) {
        arr[i] = alloc.Allocate();
        arr[i]->x = i;
        arr[i]->y = i * 1.5;
    }
    for(int i = 0; i < batchSize; i++) {
        assert(arr[i]->x == i);
        assert(arr[i]->y == i * 1.5);
        alloc.Deallocate(arr[i]);
    }

    // 5. 测试一次性释放（DeallocateAll）
    //   先分配一批，再一次性全部释放
    const int many = 2000;
    TestStruct* ptrList[many];
    for(int i = 0; i < many; i++) {
        ptrList[i] = alloc.Allocate();
    }
    // 一次性释放所有
    alloc.DeallocateAll();

    // 6. 再次分配，检验是否可正常回收并复用
    TestStruct* reused = alloc.Allocate();
    assert(reused != nullptr);
    alloc.Deallocate(reused);

    printf("All tests passed successfully.\n\n");
    delete[] arr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Test 2
// 测试分配器的多轮分配与释放
////////////////////////////////////////////////////////////////////////////////////////////////
#include "PointerRecorder.h"

inline void TestAllocator2(){
    printf("======== TEST 2 ========\n");
    using Td = int;
    MemoryPool_4K mp(7);
    PointerRecorder<Td> recorder;
    HiAllocator_4K<Td> allocator(&mp, 10);
    
    const int N = 100000;
    Td** arr = new Td * [N];

    const int turn = 10; // repeat 10 times
    for (int k = 0; k < turn; k++){
        printf("[Round %d]\n", k);

        // allocate all memory, modify with a original value, and check
        for (int i = 0; i < N; i++){
            arr[i] = allocator.Allocate();
            recorder.RecordAllocate(arr[i]);

            recorder.Modify(arr[i], k * i);
        }
        recorder.CheckAll();

        // modify memory at even index, and check
        for (int i = 0; i < N; i += 2){
            recorder.Modify(arr[i], k * i * 2);
        }
        recorder.CheckAll();

        // modify memory at 3-multiple index, and check
        for (int i = 0; i < N; i += 3){
            recorder.Modify(arr[i], k * i * 3);
        }
        recorder.CheckAll();

        // modify memory at 5-multiple index, and check
        for (int i = 0; i < N; i += 5){
            recorder.Modify(arr[i], k * i * 5);
        }
        recorder.CheckAll();

        // deallocate all memory
        allocator.DeallocateAll();
        recorder.RecordDeallocateAll();
    }
}


inline void TestAllocator() {
    printf("================ TEST ALLOCATOR ================\n");
    TestAllocator0();
    TestAllocator1();
    TestAllocator2();
    printf("\n");
}

