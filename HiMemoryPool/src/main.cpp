#include <iostream>
#include "MemoryPool.h"
#include "Allocator.h"
#include "Timer.h"


const int block_size = 4096;
using MemoryPool = MemoryPool<block_size>;
template <class T>
using Allocator = Allocator<block_size, T>;


////////////////////////////////////////////////////////////////////////////////////////////////
// Test 0
////////////////////////////////////////////////////////////////////////////////////////////////
void Test0() {
    printf("-------- TEST 0 --------\n");
    using Td = int;
    MemoryPool mp(10000);
    Allocator<Td> allocator(&mp, 10);

    const int N = 10000000;
    Td** arr = new Td*[N];

    Timer tm;
    double start;
    tm.StartTimer();


    start = tm.ReadTimer();
    for (int i = 0; i < N; i++) arr[i] = new Td;
    std::cout << "after Allocate: " << tm.ReadTimer() - start << " s" << std::endl;
    for (int i = 0; i < N; i++) delete arr[i];
    std::cout << "after Deallocate: " << tm.ReadTimer() - start << " s" << std::endl;

    start = tm.ReadTimer();
    for (int i = 0; i < N; i++) arr[i] = allocator.Allocate();
    std::cout << "after Allocate: " << tm.ReadTimer() - start << " s" << std::endl;
    // for (int i = N - 1; i >= 0; i--) allocator.Deallocate(arr[i]);
    allocator.DeallocateAll();
    std::cout << "after Deallocate: " << tm.ReadTimer() - start << " s" << std::endl;


    // start = tm.ReadTimer();
    // for (int i = 0; i < N; i++) arr[i] = new Td;
    // std::cout << "after Allocate: " << tm.ReadTimer() - start << " s" << std::endl;
    // for (int i = 0; i < N; i++) delete arr[i];
    // std::cout << "after Deallocate: " << tm.ReadTimer() - start << " s" << std::endl;

    delete[] arr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Test 1
////////////////////////////////////////////////////////////////////////////////////////////////
#include <cassert>

// 自定义结构，用于测试构造/析构调用是否正常
struct TestStruct {
    int x;
    double y;
};

void Test1() {
    printf("-------- TEST 1 --------\n");
    // 1. 创建一个内存池
    MemoryPool mp(10); // 预分配10个块，每块4KB

    // 2. 创建一个分配器，分配类型为TestStruct
    Allocator<TestStruct> alloc(&mp, 2);

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

    std::cout << "All tests passed successfully." << std::endl;
    delete[] arr;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Test 2
////////////////////////////////////////////////////////////////////////////////////////////////
#include "AllocatorTester.h"

void Test2(){
    printf("-------- TEST 2 --------\n");
    using Td = int;
    MemoryPool mp = MemoryPool(7);
    AllocatorTester<Td> tester;
    Allocator<Td> allocator(&mp, 10);
    
    const int N = 100000;
    Td** arr = new Td * [N];

    const int turn = 10; // repeat 10 times
    for (int k = 0; k < turn; k++){
        printf("[TURN %d]\n", k);
        // Allocator<Td> allocator(&mp, 10);

        // allocate all memory, modify with a original value, and check
        for (int i = 0; i < N; i++){
            arr[i] = allocator.Allocate();
            tester.RecordAllocate(arr[i]);

            tester.Modify(arr[i], k * i);
        }
        tester.CheckAll();

        // modify memory at even index, and check
        for (int i = 0; i < N; i += 2){
            tester.Modify(arr[i], k * i * 2);
        }
        tester.CheckAll();

        // modify memory at 3-multiple index, and check
        for (int i = 0; i < N; i += 3){
            tester.Modify(arr[i], k * i * 3);
        }
        tester.CheckAll();

        // modify memory at 5-multiple index, and check
        for (int i = 0; i < N; i += 5){
            tester.Modify(arr[i], k * i * 5);
        }
        tester.CheckAll();

        // deallocate all memory
        allocator.DeallocateAll();
        tester.RecordDeallocateAll();

        // tester.RecordDestruct();
    }
}


int main() {
    Test0();
    Test1();
    Test2();
    return 0;
}



