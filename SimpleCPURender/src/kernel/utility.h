#pragma once
#include <vector>
#include <thread>


namespace ut {

// check and delete
template <typename T>
inline void CheckDel(T*& ptr){
    if(ptr){
        delete ptr;
        ptr = nullptr;
    }
}

// check and delete array
template <typename T>
inline void CheckDelArr(T*& ptr){
    if(ptr){
        delete[] ptr;
        ptr = nullptr;
    }
}

// clamp x to [a, b]
template <typename T>
inline const T& Clamp(const T& x, const T& a, const T& b){
    return x < a ? a : (x > b ? b : x);
}

// euclidean modulo, in [0, m)
//                       ^ return
//                       |
//               +       m       +       + 
//             +       + |     +       +
//   ...     +       +   |   +       +     ...
//         +       +     | +       +
// ------+-------+-------+-------m-------------> x
inline int EuMod(int x, int m){
    return (x % m + m) % m;
}

// mirror-reflection modulo, in [0, m)
//                       ^ return
//                       |
//               +       m       +
//             +   +     |     +   +
//   ...     +       +   |   +       +     ...
//         +           + | +           +
// ------+---------------+-------m-------+-----> x
inline int MrrMod(int x, int m){
    int r = EuMod(x, 2 * m);
    return r < m ? r : 2 * m - r - 1;
}

// split the range [`begin`, `start`) into `n` slices
// @param[in] begin  the begin of the range
// @param[in] start  the end of the range
// @param[in] n  the number of parts
// @return  the splitting points (size = `n` + 1, include `begin` at index 0 and `start` at index `n`)
inline std::vector<int> RangeSlice(int begin, int start, int n) {
    std::vector<int> res(n + 1);
    for (int i = 0; i <= n; i++) res[i] = begin + (start - begin) * i / n;
    return res;
}

// map `coord`, from float(-1, 1) to int[0, `range`)
inline int Screen2Pixel(float coord, int range){
    return static_cast<int>((coord + 1.0f) * 0.5f * range);
}

// map `coord`, from float(-1, 1) to int[0, `range`)
inline float Screen2PixelFloat(float coord, int range){
    return (coord + 1.0f) * 0.5f * range;
}

// map `pixel`, from int[0, `range`) to float(-1, 1)
inline float Pixel2Screen(int pixel, int range){
    return (2.0f * pixel + 1.0f) / range - 1.0f;
}

}

