#pragma once
#include <vector>


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

// split the range [`begin`, `start`) into `n` parts
// @param[in] begin  the begin of the range
// @param[in] start  the end of the range
// @param[in] n  the number of parts
// @return  the split points (size = `n` + 1)
inline std::vector<int> RangeSplit(int begin, int start, int n) {
    std::vector<int> res(n + 1);
    for (int i = 0; i <= n; i++) res[i] = begin + (start - begin) * i / n;
    return res;
}

