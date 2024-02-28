#pragma once
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>

namespace util::collections {

/*
* CappedQueue is a simple queue but when the size exceeds a limit, it removes the oldest element
*/

template <typename T, size_t MaxSize>
class CappedQueue {
public:
    CappedQueue() = default;
    CappedQueue(const CappedQueue& other) = default;
    CappedQueue& operator=(const CappedQueue&) = default;

    CappedQueue(CappedQueue&& other) = default;
    CappedQueue& operator=(CappedQueue&&) = default;

    void push(T&& element) {
        if (queue.size() >= MaxSize) {
            queue.pop();
        }

        queue.push(std::move(element));
    }

    size_t size() const {
        return queue.size();
    }

    bool empty() const {
        return queue.empty();
    }

    void clear() {
        queue = {}; // queue does not have .clear() for whatever funny reason
    }

    T front() const {
        return queue.front();
    }

    T back() const {
        return queue.back();
    }

    std::vector<T> extract() const {
        std::vector<T> out;

        std::queue<T> qcopy(queue);

        while (!qcopy.empty()) {
            out.push_back(qcopy.front());
            qcopy.pop();
        }

        return out;
    }
protected:
    std::queue<T> queue;
};

// i dont know if this works at all
template<typename T, size_t N> requires std::is_move_constructible_v<T>
class SmallVector {
public:
    SmallVector() : size_(0), capacity_(N) {}

    ~SmallVector() {
        if (this->heapBacked()) {
            delete[] heapStorage;
        }
    }

    void push_back(const T& value) {
        if (size_ == capacity_) {
            this->expand();
        }

        T* dataptr = this->heapBacked() ? heapStorage : stackStorage;
        dataptr[size_++] = value;
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    const T& at(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index out of range");
        }

        if (this->heapBacked()) {
            return stackStorage[index];
        } else {
            return heapStorage[index];
        }
    }

    const T& operator[](size_t index) const {
        return this->at(index);
    }
private:
    bool heapBacked() {
        return capacity_ != N;
    }

    void expand() {
        bool heap = this->heapBacked();
        T* oldStorage = heap ? heapStorage : stackStorage;

        capacity_ *= 2;
        T* newStorage = new T[capacity_];
        // move old elements
        for (size_t i = 0; i < size_; i++) {
            newStorage[i] = std::move(oldStorage[i]);
        }

        // if we were heap backed already before that (and are reallocating, free the previous pointer)

        if (heap) {
            delete[] heapStorage;
        }

        heapStorage = newStorage;
    }

    size_t size_, capacity_;
    union {
        T stackStorage[N];
        T* heapStorage;
    };

    // iterators
public:
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator(pointer ptr) : ptr_(ptr) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() { return ptr_; }
        iterator& operator++() {
            ptr_++;
            return *this;
        }
        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        pointer ptr_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator(pointer ptr) : ptr_(ptr) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() { return ptr_; }
        const_iterator& operator++() {
            ptr_++;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const { return !(*this == other); }

    private:
        pointer ptr_;
    };

    iterator begin() {
        if (this->heapBacked()) {
            return iterator(heapStorage);
        } else {
            return iterator(stackStorage);
        }
    }

    iterator end() {
        if (this->heapBacked()) {
            return iterator(heapStorage + size_);
        } else {
            return iterator(stackStorage + size_);
        }
    }

    const_iterator begin() const {
        if (this->heapBacked()) {
            return const_iterator(heapStorage);
        } else {
            return const_iterator(stackStorage);
        }
    }

    const_iterator end() const {
        if (this->heapBacked()) {
            return const_iterator(heapStorage + size_);
        } else {
            return const_iterator(stackStorage + size_);
        }
    }

    const_iterator cbegin() const {
        return this->begin();
    }

    const_iterator cend() const {
        return this->end();
    }
};


template <typename K, typename V>
std::vector<K> mapKeys(const std::map<K, V>& map) {
    std::vector<K> out;
    for (const auto& [key, _] : map) {
        out.push_back(key);
    }

    return out;
}

template <typename K, typename V>
std::vector<K> mapKeys(const std::unordered_map<K, V>& map) {
    std::vector<K> out;
    for (const auto& [key, _] : map) {
        out.push_back(key);
    }

    return out;
}

template <typename K, typename V>
std::vector<V> mapValues(const std::map<K, V>& map) {
    std::vector<V> out;
    for (const auto& [_, value] : map) {
        out.push_back(value);
    }

    return out;
}

template <typename K, typename V>
std::vector<V> mapValues(const std::unordered_map<K, V>& map) {
    std::vector<V> out;
    for (const auto& [_, value] : map) {
        out.push_back(value);
    }

    return out;
}

};