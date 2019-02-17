/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __UTILS_EQUEUE_H__
#define __UTILS_EQUEUE_H__

#include <vector>
#include <deque>

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>


namespace tzrpc {

template<typename T>
class EQueue {
public:
    EQueue() {}
    virtual ~EQueue() {}

    void PUSH(const T& t){
        std::lock_guard<std::mutex> lock(lock_);
        items_.push_back(t);
        item_notify_.notify_one();
    }
    template< typename InputIt >
    void PUSH(InputIt first, InputIt last) {
        std::lock_guard<std::mutex> lock(lock_);
        items_.insert(items_.end(), first, last);
        item_notify_.notify_all();
    }

    void POP(T& t) {
        t = POP();
    }

    T POP() {
        std::unique_lock<std::mutex> lock(lock_);
        while (items_.empty()) {
            item_notify_.wait(lock);
        }

        T t = items_.front();
        items_.pop_front();
        return t;
    }
    size_t TRY_POP(std::vector<T>& vec) {
        std::unique_lock<std::mutex> lock(lock_);
        if (items_.empty()) {
            return 0;
        }
        vec.clear();
        vec.assign(items_.begin(), items_.end());
        items_.clear();
        return vec.size();
    }

    size_t POP(std::vector<T>& vec, size_t max_count, uint64_t msec) {
        std::unique_lock<std::mutex> lock(lock_);

        while (items_.empty()) {
            if (!item_notify_.wait_for(lock, std::chrono::milliseconds(msec))){
                goto check;
            }
        }
check:
        if (items_.empty()) {
            return 0;
        }

        size_t ret_count = 0;
        do {
            T t = items_.front();
            items_.pop_front();
            vec.emplace_back(t);
            ++ ret_count;
        } while ( ret_count < max_count && !items_.empty());

        return ret_count;
    }

    bool POP(T& t, uint64_t msec) {
        std::unique_lock<std::mutex> lock(lock_);

        // if(!item_notify_.timed_wait(lock, timeout, std::bind(&EQueue::EMPTY, this))){
        while (items_.empty()) {
            if (!item_notify_.wait_for(lock, std::chrono::milliseconds(msec))){
                goto check;
            }
        }

check:
        if (items_.empty()) {
            return false;
        }

        t = items_.front();
        items_.pop_front();
        return true;
    }

    // 只有t不存在的时候才添加
    bool UNIQUE_PUSH(const T& t) {
        std::lock_guard<std::mutex> lock(lock_);
        if (std::find(items_.begin(), items_.end(), t) == items_.end()) {
            items_.push_back(t);
            item_notify_.notify_one();
            return true;
        }
        return false;
    }

    size_t SIZE() {
        std::lock_guard<std::mutex> lock(lock_);
        return items_.size();
    }

    bool EMPTY() {
        std::lock_guard<std::mutex> lock(lock_);
        return items_.empty();
    }

    size_t SHRINK_FRONT(size_t sz) {
        std::lock_guard<std::mutex> lock(lock_);

        size_t orig_sz = items_.size();
        if (orig_sz <= sz)
            return 0;

        auto iter = items_.end() - sz;
        items_.erase(items_.begin(), iter);

        return orig_sz - items_.size();
    }

private:
    std::mutex lock_;
    std::condition_variable item_notify_;

    std::deque<T> items_;
};


} // end namespace tzrpc

#endif // __UTILS_EQUEUE_H__

