#pragma once
#include <atomic>
#include <memory>
#include <vector>

using namespace std;

/*
template< class T >
bool atomic_compare_exchange_weak( std::atomic* obj,
                                   T* expected, T desired );
template< class T >
bool atomic_compare_exchange_weak( volatile std::atomic* obj,
                                   T* expected, T desired );

          ::__sync_bool_compare_and_swap
*/

template <typename T>
class LockFreeQueue
{
private:
    struct Node
    {
        T m_data;
        Node *m_next;
        Node(T data) : m_data(data), m_next(nullptr){};
        Node() : m_data(), m_next(nullptr){};
        // Node(T &data) : m_data(data), m_next(nullptr){};
        // Node(const T &data) : m_data(data), m_next(nullptr){};
    };

public:
    Node *m_head, *m_tail;
    LockFreeQueue();
    // 多个worker线程都会写日志queue
    void push(const T &data);

    // 一个线程从日志queue中取出日志
    bool pop(T &data);
};

template <typename T>
inline LockFreeQueue<T>::LockFreeQueue() : m_head(new Node()), m_tail(m_head)
{
}

template <typename T>
inline void LockFreeQueue<T>::push(const T &data)
{

    // 加入队列相比于取出简单一些，因为只用操作tail指针
    decltype(m_head) newNode = new Node(data);
    Node *tail = nullptr;
    while (true)
    {
        tail = m_tail;
        Node *nextNode = tail->m_next; // 注意这里要去tail的next，不能取m_tail的next，因为m_tail可能刚好在这里更新了，
        // 那么next可能在此时就等于真正的m_tail，这样的话next不为null，就会尝试后移m_tail，hhh，这么一分析好像也是可以的
        // 不过还是按照标准来写吧

        // m_tail已经更新了，则重试
        if (tail != m_tail)
        {
            continue;
        }

        // next不为null
        if (nextNode != nullptr)
        {
            // 这里不用判断返回值，因为这里只是尝试将tail往后更新，能更新就更新，不能更新就算了
            //  这里的主要目的是重新循环来重试
            //  虽然是“尝试”但是也是必须的，因为如其他线程可能会挂掉导致死锁。
            ::__sync_bool_compare_and_swap(&m_tail, tail, nextNode);
            // atomic_compare_exchange_weak(&m_tail, &tail, nextNode);
            continue;
        }
        // 尝试加入节点
        if (::__sync_bool_compare_and_swap(&(m_tail->m_next), nextNode, newNode))
        {
            break;
        }
    }
    ::__sync_bool_compare_and_swap(&m_tail, tail, newNode); // 更新尾结点，不用强制，因为就算该节点挂掉，其他节点也会往后更新尾节点的
}

template <typename T>
inline bool LockFreeQueue<T>::pop(T &data)
{
    while (true)
    {
        // 取出节点的操作相对还复杂一些，因为涉及到了头指针和尾指针两个指针的移动和判断
        // 本质上还是因为m_tail更新慢的问题
        decltype(m_head) head = m_head;
        decltype(m_head) tail = m_tail;
        decltype(m_head) next = head->m_next; // 既然m_tail有延迟的问题，那么就用next来判断有没有延迟呗，那么为什么只判断next（后面一位）
        // 而不再往后判断好几位呢？ 因为m_tail最多只会延迟一位，想一下，如果延迟一位其他线程就会尝试往后更新m_tail，因此m_tail
        // 不会延迟超过一位，因此只需要一个next即可。

        if (head != m_head)
        {
            continue;
        }

        if (head == tail && next == nullptr)
        {
            return false; // 没有数据
        }
        if (head == tail && next != nullptr)
        {
            continue;
        }

        if (::__sync_bool_compare_and_swap(&m_head, head, next))
        {
            data = next->m_data; // 取出数据
            delete head;
            return true;
        }
    }
}
