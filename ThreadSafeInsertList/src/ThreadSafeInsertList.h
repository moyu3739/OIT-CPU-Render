#include <atomic>



template <typename T>
struct ThreadSafeInsertListNode{
    T data;
    std::atomic<ThreadSafeInsertListNode<T>*> next;

    ThreadSafeInsertListNode(const T& val): data(val), next(nullptr) {}
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
    
        // 解引用操作符
        T& operator*() const {
            return ptr->data;
        }
    
        // 前向迭代操作符
        Iterator& operator++() {
            // 使用 acquire 语义读取最新 next
            ptr = ptr->next.load(std::memory_order_acquire);
            return *this;
        }

        Iterator& operator++(int) {
            Iterator old = *this;
            ptr = ptr->next.load(std::memory_order_acquire);
            return old;
        }
    
        // 不等比较
        bool operator!=(const Iterator& other) const {
            return ptr != other.ptr;
        }

        // 等比较
        bool operator==(const Iterator& other) const {
            return ptr == other.ptr;
        }

    private:
        Node* ptr;  // 当前节点指针
    };

///////////////////////////////////////////////////////////////////////////////////
//   List
///////////////////////////////////////////////////////////////////////////////////
public:
    ThreadSafeInsertList(): head(nullptr) {}

    ThreadSafeInsertList(const T& guard){
        head.store(CreateNode(guard), std::memory_order_relaxed);
    }

    ~ThreadSafeInsertList(){
        Clear();
    }

    bool IsEmpty() const{
        return head.load(std::memory_order_acquire) == nullptr;
    }

    void InsertAt(Iterator prev_node, const T& val){
        InsertAt(prev_node.ptr, val);
    }

    bool TryInsertAt(Iterator prev, Iterator post, const T& val){
        return TryInsertAt(prev.ptr, post.ptr, val);
    }

    void InsertHead(const T& val){
        Node* new_node = CreateNode(val);
        Node* curr_head = head.load(std::memory_order_acquire);
        while (true){
            new_node->next.store(curr_head, std::memory_order_relaxed);
            if (head.compare_exchange_strong(curr_head, new_node, std::memory_order_release, std::memory_order_acquire)){
                break;
            }
        }
    }

    bool TryInsertHead(Iterator curr_head, const T& val){
        return TryInsertHead(curr_head.ptr, val);
    }

    void Clear() {
        Node* curr = head.load(std::memory_order_relaxed);
        while (curr != nullptr) {
            Node* next = curr->next.load(std::memory_order_relaxed);
            DestroyNode(curr);
            curr = next;
        }
    }

    // 获取起始迭代器
    Iterator Begin() const{
        // 使用 acquire 语义读取头节点
        return Iterator(head.load(std::memory_order_acquire));
    }

    // 获取终止迭代器
    Iterator End() const{
        return Iterator(nullptr);
    }

private:
    Node* CreateNode(const T& val) const {
        return new Node(val);
    }

    void DestroyNode(Node* node) const {
        delete node;
    }

    void InsertAt(Node* prev_node, const T& val){
        Node* new_node = CreateNode(val);

        // 读取前驱节点的当前 next 指针（带 acquire 语义）
        Node* curr_next = prev_node->next.load(std::memory_order_acquire);
    
        while (true) {
            // 设置新节点的 next 为 curr_next（无需同步）
            new_node->next.store(curr_next, std::memory_order_relaxed);
    
            // 尝试通过 CAS 更新前驱节点的 next
            if (prev_node->next.compare_exchange_strong(
                curr_next,         // 期望值：prev_node->next 应为 curr_next
                new_node,          // 目标值：将其更新为 new_node
                std::memory_order_release,  // 成功时的内存序：发布新节点
                std::memory_order_acquire   // 失败时的内存序：获取当前 next 指针
            )) {
                break; // CAS 成功，插入完成
            }

            // CAS 失败：其他线程修改了 prev_node->next，循环重试
        }
    }

    // @brief to insert a new node between `prev_node` and `post_node`
    // @param[in] prev_node: the previous node
    // @param[in] post_node: the node next to the previous node before inserting
    //                       (used to check satisfying `prev_node->next == post_node`)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    bool TryInsertAt(Node* prev_node, Node* post_node, const T& val){
        Node* new_node = CreateNode(val);
        // Node* curr_next = prev_node->next.load(std::memory_order_acquire);
        new_node->next.store(post_node, std::memory_order_relaxed);
        if (prev_node->next.compare_exchange_strong(
            post_node, new_node,
            std::memory_order_release, std::memory_order_acquire
        )) {
            return true;
        }
        else {
            DestroyNode(new_node);
            return false;
        }
    }

    // @brief to insert a new node at the head of the list
    // @param[in] curr_head: the current head node (used to check satisfying `head == curr_head`)
    // @param[in] val: the value of the new node
    // @return  true if the new node is inserted successfully, false otherwise
    bool TryInsertHead(Node* curr_head, const T& val){
        Node* new_node = CreateNode(val);
        // Node* curr_head = head.load(std::memory_order_acquire);
        new_node->next.store(curr_head, std::memory_order_relaxed);
        if (head.compare_exchange_strong(
            curr_head, new_node,
            std::memory_order_release, std::memory_order_acquire
        )){
            return true;
        }
        else{
            DestroyNode(new_node);
            return false;
        }
    }

private:
    std::atomic<Node*> head;

};