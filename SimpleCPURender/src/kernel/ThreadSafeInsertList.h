#pragma once

#include <atomic>


template <typename T>
struct ThreadSafeInsertListNode{
    T data;
    std::atomic<ThreadSafeInsertListNode<T>*> next;

    ThreadSafeInsertListNode(const T& val): data(val), next(nullptr) {}

    void Init(const T& val) {
        data = val;
        next.store(nullptr, std::memory_order_relaxed);
    }
};


template <typename T>
class ThreadSafeInsertList{
    using Node = ThreadSafeInsertListNode<T>;

///////////////////////////////////////////////////////////////////////////////////
//   iterator
///////////////////////////////////////////////////////////////////////////////////
public:
    class Iterator {
        friend class ThreadSafeInsertList;

    public:
        explicit Iterator(Node* node): ptr(node) {}

        Iterator(const Iterator& other) = default;

        // @return  the iterator to the next node
        Iterator Next() const {
            return Iterator(ptr->next.load(std::memory_order_acquire));
        }
    
        T& operator*() const {
            return ptr->data;
        }

        T* operator->() const {
            return &(ptr->data);
        }
    
        Iterator& operator++() {
            // use acquire semantics to read the next pointer
            ptr = ptr->next.load(std::memory_order_acquire);
            return *this;
        }

        Iterator& operator++(int) {
            Iterator old = *this;
            ptr = ptr->next.load(std::memory_order_acquire);
            return old;
        }
    
        bool operator!=(const Iterator& other) const {
            return ptr != other.ptr;
        }

        bool operator==(const Iterator& other) const {
            return ptr == other.ptr;
        }

    private:
        Node* ptr;  // pointer to current node
    };

///////////////////////////////////////////////////////////////////////////////////
//   List
///////////////////////////////////////////////////////////////////////////////////
public:
    ThreadSafeInsertList(): head(nullptr) {}

    // destructor do nothing; user should call `Clear` to free all nodes by themselves
    ~ThreadSafeInsertList() {}

    ThreadSafeInsertList(const ThreadSafeInsertList&) = delete;
    ThreadSafeInsertList& operator=(const ThreadSafeInsertList&) = delete;

    bool IsEmpty() const{
        return head.load(std::memory_order_acquire) == nullptr;
    }

    void ResetHead() {
        head.store(nullptr, std::memory_order_relaxed);
    }

    // @brief  insert a new node after `prev_iter`
    // @param[in] prev_iter: the previous iterator
    // @param[in] val: the value of the new node
    template <class Allocator>
    void InsertAt(Iterator prev_iter, const T& val, Allocator* allocator){
        InsertAt(prev_iter.ptr, val, allocator);
    }

    // @brief  try inserting a new node between `prev_iter` and `post_iter`
    // @param[in] prev_iter: the previous iterator
    // @param[in] post_iter: the iterator next to the previous iterator before inserting
    //                       (used to check satisfying `prev_iter->next == post_iter`)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    template <class Allocator>
    bool TryInsertAt(Iterator prev_iter, Iterator post_iter, const T& val, Allocator* allocator){
        return TryInsertAt(prev_iter.ptr, post_iter.ptr, val, allocator);
    }

    // @brief  insert a new node at head of the list
    // @param[in] val: the value of the new node
    template <class Allocator>
    void InsertHead(const T& val, Allocator* allocator){
        Node* new_node = CreateNode(val, allocator);
        Node* curr_head = head.load(std::memory_order_acquire);
        while (true){
            new_node->next.store(curr_head, std::memory_order_relaxed);
            if (head.compare_exchange_strong(curr_head, new_node, std::memory_order_release, std::memory_order_acquire)){
                break;
            }
        }
    }

    // @brief to insert a new node at the head of the list
    // @param[in] curr_head: the current head (can be null)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    template <class Allocator>
    bool TryInsertHead(Iterator curr_head, const T& val, Allocator* allocator){
        return TryInsertHead(curr_head.ptr, val, allocator);
    }

    // @brief  clear all nodes in the list
    // @note free all nodes in the list, and reset head to nullptr
    template <class Allocator>
    void Clear(Allocator* allocator) {
        Node* curr = head.load(std::memory_order_acquire);
        // delete all nodes
        while (curr != nullptr) {
            Node* next = curr->next.load(std::memory_order_relaxed);
            DestroyNode(curr, allocator);
            curr = next;
        }
        // set head to nullptr
        head.store(nullptr, std::memory_order_relaxed);
    }

    // begin iterator
    Iterator Begin() const{
        // use acquire semantics to read the head pointer
        return Iterator(head.load(std::memory_order_acquire));
    }

    // end iterator
    Iterator End() const{
        return Iterator(nullptr);
    }

private:
    template <class Allocator>
    Node* CreateNode(const T& val, Allocator* allocator) const {
        // return new Node(val);
        Node* new_node = allocator->Allocate();
        new_node->Init(val);
        return new_node;
    }

    template <class Allocator>
    void DestroyNode(Node* node, Allocator* allocator) const {
        allocator->Deallocate(node);
    }

    // @brief to insert a new node after `prev_node`
    // @param[in] prev_node: the previous node
    // @param[in] val: the value of the new node
    template <class Allocator>
    void InsertAt(Node* prev_node, const T& val, Allocator* allocator){
        Node* new_node = CreateNode(val, allocator, allocator);

        // read the current next pointer of the previous node (with acquire semantics)
        Node* curr_next = prev_node->next.load(std::memory_order_acquire);
    
        while (true) {
            // set the next pointer of the new node to curr_next (no synchronization needed)
            new_node->next.store(curr_next, std::memory_order_relaxed);
    
            // try to update the next pointer of the previous node using CAS
            if (prev_node->next.compare_exchange_strong(
                curr_next,         // expected value: `prev_node->next` should be `curr_next`
                new_node,          // target value: update `prev_node->next` to `new_node`
                std::memory_order_release,  // memory order when successful: release the new node
                std::memory_order_acquire   // memory order when failed: acquire the new node
            )) {
                break; // CAS succeeded, exit the loop
            }

            // CAS failed: another thread modified `prev_node->next`, retry
        }
    }

    // @brief to insert a new node between `prev_node` and `post_node`
    // @param[in] prev_node: the previous node
    // @param[in] post_node: the node next to the previous node before inserting
    //                       (used to check satisfying `prev_node->next == post_node`)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    template <class Allocator>
    bool TryInsertAt(Node* prev_node, Node* post_node, const T& val, Allocator* allocator){
        Node* new_node = CreateNode(val, allocator);
        new_node->next.store(post_node, std::memory_order_relaxed);
        if (prev_node->next.compare_exchange_strong(
            post_node, new_node,
            std::memory_order_release, std::memory_order_acquire
        )) {
            return true;
        }
        else {
            DestroyNode(new_node, allocator);
            return false;
        }
    }

    // @brief to insert a new node at the head of the list
    // @param[in] curr_head: the current head node (used to check satisfying `head == curr_head`, can be nullptr)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    template <class Allocator>
    bool TryInsertHead(Node* curr_head, const T& val, Allocator* allocator){
        Node* new_node = CreateNode(val, allocator);
        new_node->next.store(curr_head, std::memory_order_relaxed);
        if (head.compare_exchange_strong(
            curr_head, new_node,
            std::memory_order_release, std::memory_order_acquire
        )) {
            return true;
        }
        else{
            DestroyNode(new_node, allocator);
            return false;
        }
    }

private:
    std::atomic<Node*> head;
};