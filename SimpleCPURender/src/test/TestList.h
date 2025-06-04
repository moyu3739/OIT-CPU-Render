#include <iostream>
#include <list>
#include <vector>
#include <random>
#include <algorithm>
#include <thread>
#include "Timer.h"
#include "ThreadSafeInsertList.h"

using namespace oit;


template <typename T>
class StandardAllocator{
public:
    StandardAllocator() = default;

    T* Allocate() {
        return reinterpret_cast<T*>(new unsigned char[sizeof(T)]);
    }

    void Deallocate(T* ptr) {
        delete[] reinterpret_cast<unsigned char*>(ptr);
    }
};


inline int SortedInsert(ThreadSafeInsertList<int>& list, int val){
    StandardAllocator<ThreadSafeInsertListNode<int>> allocator;
    ThreadSafeInsertList<int>::Iterator prev = list.Begin();
    ThreadSafeInsertList<int>::Iterator post = list.Begin().Next();
    
    int conflict = 0;
    while (true){
        // if `post` is at the end, or `*post >= val`,
        // then insert val between `prev` and `post`,
        // otherwise, move next
        if (post != list.End()){
            if (*post < val){
                prev = post;
                ++post;
                continue;
            }
        }

        bool res = list.TryInsertAt(prev, post, val, &allocator);
        if (res) break;
        else{
            post = prev.Next(); // get the next node again
            conflict++;
            continue; // retry
        }
    }

    return conflict;
}

inline void Shuffle(std::vector<int>& vals){
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(vals.begin(), vals.end(), g);
}

inline void InsertBatch(ThreadSafeInsertList<int>& list, const std::vector<int>& vals, int& conflict){
    conflict = 0;
    for (int val: vals){
        conflict += SortedInsert(list, val);
    }
}

inline void Check(const ThreadSafeInsertList<int>& list){
    int size = 0;
    bool flag = true;
    ThreadSafeInsertList<int>::Iterator record = list.Begin();

    for (auto iter = list.Begin(); iter != list.End(); ++iter){
        size++;
        auto next = iter.Next();
        if (next != list.End() && *iter > *next){
            flag = false;
            record = iter;
        }
    }

    printf("size: %d\n", size);
    if (flag){
        printf("Check passed\n");
    }
    else{
        printf("Check failed !!!\n");
        printf("record: %d\n", *record);
    }
}


inline void TestList0(){
    const int N = 100000;
    const int T = 8;
    
    std::vector<int> nums(N);
    for (int i = 0; i < N; i++) nums[i] = i;
    Shuffle(nums);

    std::vector<int> nums_t[T];
    std::thread threads[T];
    int conflicts[T];
    for (int i = 0; i < T; i++){
        nums_t[i].assign(nums.begin() + i * N / T, nums.begin() + (i + 1) * N / T);
    }
    
    ThreadSafeInsertList<int> list;
    StandardAllocator<ThreadSafeInsertListNode<int>> allocator;
    list.TryInsertHead(list.Begin(), -1, &allocator); // insert 0 at head

    Timer tm;
    tm.StartTimer();

    for (int i = 0; i < T; i++){
        threads[i] = std::thread(InsertBatch, std::ref(list), std::ref(nums_t[i]), std::ref(conflicts[i]));
    }
    printf("[TIMER] all threads start: %.6f s\n", tm.ReadTimer());
    for (int i = 0; i < T; i++){
        threads[i].join();
    }

    printf("[TIMER] all threads finish: %.4f s\n", tm.ReadTimer());
    for (int i = 0; i < T; i++){
        printf("conflicts[%d]: %d\n", i, conflicts[i]);
    }

    // for (auto iter = list.Begin(); iter != list.End(); ++iter){
    //     std::cout << *iter << " ";
    // }
    // std::cout << std::endl;

    Check(list);
}


inline void TestList() {
    printf("================ TEST LIST ================\n");
    TestList0();
    printf("\n");
}

