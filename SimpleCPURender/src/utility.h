#pragma once

#define CheckDel(ptr) if(ptr){delete ptr; ptr = nullptr;} // check and delete

#define CheckDelArr(ptr) if(ptr){delete[] ptr; ptr = nullptr;} // check and delete array

// clamp x to [a, b]
template <typename T>
inline T Clamp(T x, T a, T b){
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
