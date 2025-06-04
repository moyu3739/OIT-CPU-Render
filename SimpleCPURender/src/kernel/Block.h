#pragma once


namespace oit {

struct BlockMeta {
    int next_pos; // the next byte to be allocated
    int in_use; // the number of bytes in use
};

template <int size> // `size` is the size of the block, must be 2^n (n >= 4)
class Block {
    using Block_t = Block<size>;

public:
    Block() = delete; // disable default constructor

    Block(const Block_t& other) = delete; // disable copy constructor

    Block(const Block_t&& other) = delete; // disable move constructor

    ~Block() = delete; // disable destructor

    void Reset(){
        meta.next_pos = 0;
        meta.in_use = 0;
    }

    void* Allocate(int bytes){
        if (meta.next_pos + bytes > capacity) return nullptr;
        void* ptr = data + meta.next_pos;
        meta.next_pos += bytes;
        meta.in_use += bytes;
        return ptr;
    }

    void Deallocate(int bytes){
        meta.in_use -= bytes;
    }

    template<class T>
    T* Allocate(){
        return reinterpret_cast<T*>(Allocate(sizeof(T)));
    }

    template<class T>
    void Deallocate(){
        Deallocate(sizeof(T));
    }

public:
    constexpr static int capacity = size - sizeof(BlockMeta); // the capacity of the block for data storage
    constexpr static long long mask = ~(size - 1); // mask to get the address of the block, e.g. 0xFFFFFFFFFFFFF000 for 4KB block
    
public:
    BlockMeta meta;
private:
    char data[capacity];
};

} // namespace oit
