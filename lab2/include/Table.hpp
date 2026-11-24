#pragma once

#include <cstddef>
#include <memory>

template <typename T>
class Table {
private:
    std::unique_ptr<T[]> items_;
    std::size_t size_ = 0;
public:
    static constexpr std::size_t capacity = 1e5;

    Table() : items_(std::make_unique<T[]>(capacity)) {}

    bool add(const T& item) {
        if (size_ == capacity) return false;
        
        items_[size_] = item;
        ++size_;

        return true;
    }

    std::size_t size() const {
        return size_;
    }

    T* get(std::size_t index) {
        return index < size_ ? &items_[index] : nullptr;
    }

    const T* get(std::size_t index) const {
        return index < size_ ? &items_[index] : nullptr;
    }
};
