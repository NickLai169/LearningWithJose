#include <iostream>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <new>

/*
Notes:
- we're implementing a single fixed-size block
    - let's assume it's a explicit free list since those are a little more complicated to implemnet
    - let's not make a segment list

*/

template<typename T, size_t Volume = 256>
class FixedSizeAllocator {
    struct Block {
        size_t size;
        Block* next;
        Block* prev;
        size_t leftSize;
        T data[];
    };

    Block* _head; // Free list head
    char* _buffer; // Raw buffer storage
    size_t _capacity;

public:
    using value_type = T;

    template<typename U>
    struct rebind { using other = FixedSizeAllocator<U, Volume>; };

    FixedSizeAllocator() {
        // Allocate memory buffer
        _capacity = (sizeof(Block)) * Volume;
        _buffer = static_cast<char*>(std::malloc(_capacity));
        if (!_buffer) throw std::bad_alloc();

        // Initialize free list
        _head = reinterpret_cast<Block*>(_buffer);
        _head->size = _capacity;
        _head->next = nullptr;
        _head->prev = nullptr;
        _head->leftSize = 0;
    }

    ~FixedSizeAllocator() { std::free(_buffer); }

    T* allocate(size_t n) {
        // std::cout << "allocate(" << n << ")" << std::endl;

        // Find a sufficiently large free block
        Block* block = _head;
        while (block && (block->size & ~1) < (sizeof(Block) * n)) {
            block = block->next;
        }
        if (!block) throw std::bad_alloc();

        // Remove block from free list, or splice if not completely occupied
        size_t dataSize = n * sizeof(T);
        if (block->size >  + dataSize) {
            // there is enough allocateable space for data, splice
            Block* newBlock = reinterpret_cast<Block*>(block + 4 + dataSize);
            newBlock->size = (block->size - dataSize) & ~1;
            newBlock->leftSize = block->size;
            newBlock->next = block->next;
            if (newBlock->next) { newBlock->next->prev = newBlock; }
            newBlock->prev = block->prev;
            if (newBlock->prev) { newBlock->prev->next = newBlock; }
        } else {
            // there is just enough allocatable space for data, reassign head
            if (block->prev) block->prev->next = block->next;
            if (block->next) block->next->prev = block->prev;
            if (_head == block) _head = block->next;
        }
        block->prev = nullptr;
        block->next = nullptr;

        block->size |= 1; // Mark as allocated
        return reinterpret_cast<T*>(block->data);
    }

    void deallocate(T* ptr, std::size_t) {
        // std::cout << "deallocate(" << ptr << ")" << std::endl;
        if (!ptr) return;

        // Recover block metadata
        Block* block = reinterpret_cast<Block*>(reinterpret_cast<char*>(ptr) - offsetof(Block, data));
        block->size &= ~1; // Mark as free

        // consider whether the current freed block is mergeable with adjacent free blocks
        bool merged = false;
        if (block != reinterpret_cast<Block*>(_buffer)) {
            // consider merging left
            Block* leftBlock = reinterpret_cast<Block*>(block - block->leftSize);
            if (!(leftBlock->size & 1)) {
                // leftBlock free ... merge left
                leftBlock->size += block->size;
                block = leftBlock;
                merged = true;
            }
        }
        if (block + block->size < reinterpret_cast<Block*>(_buffer + _capacity)) {
            // consider merging right
            Block* rightBlock = reinterpret_cast<Block*>(block + block->size);
            std::cout << rightBlock->size;
            if (!(rightBlock->size & 1)) {
                // rightBlock free ... merge right
                block->size = block->size + rightBlock->size;
                if (block->prev || block->next) {
                    // this block is currently joined somewhere, join rightBlock's neighbours
                    if (rightBlock->prev) { rightBlock->prev->next = rightBlock->next; }
                    if (rightBlock->next) { rightBlock->next->prev = rightBlock->prev; }
                    merged = true;
                } else {
                    // this block is not joined anywhere, set this as the new rightBlock
                    block->prev = rightBlock->prev;
                    block->next = rightBlock->next;
                    merged = true;
                }
            }
        }

        // Add back to free list
        if (!merged) {
            block->next = _head;
            block->prev = nullptr;  
            if (_head) _head->prev = block;
            _head = block;
        }
    }
};


int main() {
    FixedSizeAllocator<int> allocator;
    std::vector<int, FixedSizeAllocator<int>> newVec;

    newVec.push_back(4);
    newVec.push_back(3);
    newVec.push_back(33);
    newVec.push_back(4);
    // newVec.push_back(5);
    
    std::cout << "end run" << std::endl;
    return 0;
}