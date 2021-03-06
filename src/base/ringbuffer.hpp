#pragma once

#include <memory>
#include <mutex>
#include <iostream>

template <class T>
class RingBuffer {
private:
    std::mutex mutex_;
    std::unique_ptr<T[]> buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_;
    size_t length_ = 0;
public:
    RingBuffer(size_t size) :
        buf_(std::unique_ptr<T[]>(new T[size + 1])),
        size_(size) {}

    void put(T item) {
        std::lock_guard<std::mutex> lock(mutex_);

        buf_[head_] = item;
        head_ = (head_ + 1) % size_;
        ++length_;

        if (head_ == tail_) {
            tail_ = (tail_ + 1) % size_;
        }

        // std::cout << "put " << head_ << "\t" << tail_ << "\t" << length_ << std::endl;
    }

    T get() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (empty()) {
            return T();
        }

        --length_;

        // read data and advance the tail (we now have a free space)
        auto val = buf_[tail_];
        tail_ = (tail_ + 1) % size_;

        // std::cout << "get " << head_ << "\t" << tail_ << "\t" << length_ << std::endl;

        return val;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = tail_;
        length_ = 0;
    }

    T* buffer() const {
        // std::cout << "buf " << head_ << "\t" << tail_ << "\t" << length_ << std::endl;
        return buf_.get();
    }

    bool empty() const {
        // if head and tail are equal, we are empty
        return head_ == tail_;
    }

    bool full() const {
        // if tail is ahead the head by 1, we are full
        return ((head_ + 1) % size_) == tail_;
    }

    size_t size() const {
        return size_ - 1;
    }
    size_t head() const {
        return head_;
    }
    size_t tail() const {
        return tail_;
    }
    size_t length() const {
        return length_;
    }
};
