#pragma once

#include <list>
#include <mutex>
#include "Primitive.h"
#include "ThreadSafeInsertList.h"


class AtomicFragmentList_MTX {
public:
    AtomicFragmentList_MTX() {}

    void Clear() {
        fragments.clear();
    }

    void InsertSorted(const Fragment& fragment, int x, int y) {
        std::lock_guard<std::mutex> lock(mtx);
        auto iter = fragments.begin();
        while (iter != fragments.end() && iter->depth > fragment.depth) ++iter;
        fragments.insert(iter, fragment);
    }

    void Blend(glm::vec3& base_color, float base_depth) {
        // skip fragments with depth larger than `base_depth`
        auto iter = fragments.begin();
        while (iter != fragments.end() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != fragments.end(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }

private:
    std::list<Fragment> fragments;
    std::mutex mtx;
};



class AtomicFragmentList_CAS {
public:
    // add a guard node (with depth INFINITY) when constructing
    AtomicFragmentList_CAS(): fragments(Fragment{glm::vec4(0.0f), INFINITY}) {}

    void Clear() {
        fragments.Clear();
    }

    void InsertSorted(const Fragment& fragment, int x, int y) {
        auto prev = fragments.Begin();
        auto post = fragments.Begin().Next();
        
        while (true){
            // if `post` not at end, and `post->depth > fragment.depth`, move next
            if (post != fragments.End() && post->depth > fragment.depth){
                prev = post;
                ++post;
                continue;
            }
            // otherwise insert `fragment` between `prev` and `post`
            bool res = fragments.TryInsertAt(prev, post, fragment);
            if (res) break;
            else{
                post = prev.Next(); // get the next node again
                continue; // retry
            }
        }
    }

    void Blend(glm::vec3& base_color, float base_depth) {
        // skip fragments with depth larger than `base_depth`
        auto iter = fragments.Begin().Next(); // skip the guard node
        while (iter != fragments.End() && iter->depth > base_depth) ++iter;
        // blend color
        for (; iter != fragments.End(); ++iter)
            base_color = base_color * (1.0f - iter->color.a) + iter->color.a * glm::vec3(iter->color);
    }

private:
    ThreadSafeInsertList<Fragment> fragments;
};
    

