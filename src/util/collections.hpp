#pragma once
#include <queue>


namespace util::collections {

/*
* CappedQueue is a simple queue but when the size exceeds a limit, it removes the oldest element
*/

template <typename T, size_t MaxSize>
class CappedQueue {
public:
    void push(const T& element) {
        if (queue.size() >= MaxSize) {
            queue.pop();
        }

        queue.push(element);
    }

    size_t size() const {
        return queue.size();
    }

    bool empty() const {
        return queue.empty();
    }

    T front() const {
        return queue.front();
    }

    T back() const {
        return queue.back();
    }

    std::vector<T> extract() const {
        std::vector<T> out(queue.size());

        std::queue<T> qcopy(queue);

        while (!qcopy.empty()) {
            out.push_back(qcopy.front());
            qcopy.pop();
        }

        return out;
    }
private:
    std::queue<T> queue;
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