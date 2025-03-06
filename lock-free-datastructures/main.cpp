#include <iostream>
#include <type_traits>
#include <atomic>
#include <array>
#include <type_traits>
#include <thread>

/*
Notes:
- Implement a lock-free SPSC ring buffer
- Implement a lock-free MPMC ring buffer
*/

template<typename T, size_t Size = 256>
class SPSCQueue {
    std::array<T, Size + 1> _buffer;
    std::atomic<size_t> _head = 0;
    std::atomic<size_t> _tail = 0;

public:

    bool isEmpty() { return _head.load(std::memory_order_acquire) == _tail.load(std::memory_order_acquire); }

    bool isFull() {
        size_t headVal = _head.load(std::memory_order_acquire);
        size_t tailVal = _tail.load(std::memory_order_acquire);
        return headVal == 0 ? tailVal == _buffer.size() - 1 : tailVal == (headVal + (_buffer.size() - 1))%_buffer.size();
    }

    void push(T& val) {
        if (isFull()) { return; }

        if constexpr (std::is_assignable_v<T, T> || std::is_move_assignable_v<T>) {
            _buffer[_tail.load(std::memory_order_acquire)] = { std::move(val) };
        } else if constexpr (std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>) {
            _buffer[_tail.load(std::memory_order_acquire)](std::move(val));
        }
        size_t currTail = (_tail.load(std::memory_order_acquire) + 1) % _buffer.size();
        _tail.store(currTail, std::memory_order_release);
    }

    void push(T&& val) {
        if (isFull()) { return; }

        if constexpr (std::is_assignable_v<T, T> || std::is_move_assignable_v<T>) {
            _buffer[_tail.load(std::memory_order_acquire)] = { std::forward<T>(val) };
        } else if constexpr (std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>) {
            _buffer[_tail.load(std::memory_order_acquire)](std::forward<T>(val));
        }
        size_t currTail = (_tail.load(std::memory_order_acquire) + 1) % _buffer.size();
        _tail.store(currTail, std::memory_order_release);
    }

    T pop() {
        T retVal;
        if constexpr (std::is_default_constructible_v<T>) {
            if (isEmpty()) { return retVal; }
        }
        
        if constexpr (std::is_assignable_v<T, T> || std::is_move_assignable_v<T>) {
            retVal = std::move(_buffer[_head.load(std::memory_order_acquire)]);
        } else if constexpr (std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>) {
            retVal = std::move(T(_buffer[_head.load(std::memory_order_acquire)]));
        }
        size_t currHead = (_head.load(std::memory_order_acquire) + 1) % _buffer.size();
        _head.store(currHead, std::memory_order_release);
        return retVal;
    }
};


template<typename QueueType>
void testQueuePush(QueueType &myQueue) {
    for (int i = 0; i < 20; i++) {
        myQueue.push(i);
    }
}

template<typename QueueType>
void testQueuePop(QueueType &myQueue) {
    while (!myQueue.isEmpty()) {
        auto i = myQueue.pop();
    }
}

int main() {
    {
        SPSCQueue<int> myQueue;
        std::jthread pushThread(testQueuePush<SPSCQueue<int>>, std::ref(myQueue));
        std::jthread popThread(testQueuePop<SPSCQueue<int>>, std::ref(myQueue));
    }

    std::cout << "end run" << std::endl;
    return 0;
}