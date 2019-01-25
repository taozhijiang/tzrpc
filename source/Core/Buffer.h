#ifndef __CORE_BUFFER_H___
#define __CORE_BUFFER_H__

#include <cstdint>
#include <string>

#include <memory>

#include <xtra_rhel6.h>

class Buffer {
public:

    // 构造函数

    Buffer():
        data_ ({}) {
    }

    explicit Buffer(const std::string& data):
        data_(data.begin(), data.end()) {
    }


    uint32_t append(const std::string& data) {
        std::copy(data.begin(), data.end(), std::back_inserter(data_));
        return static_cast<uint32_t>(data_.size());
    }

    // 从队列的开头取出若干个字符，返回实际得到的字符数
    bool retrive(std::string& store, uint32_t sz) {
        if (sz == 0 || data_.empty()) {
            return false;
        }

        sz = sz > data_.size() ? data_.size() : sz;
        std::vector<char> tmp (data_.begin() + sz, data_.end()); // 剩余的数据
        store = std::string(data_.begin(), data_.begin() + sz);         // 要取的数据

        data_.swap(tmp);
        return true;
    }

    // 移动优化
    Buffer& operator=(Buffer&& other) {
        data_ = std::move(other.data_);
    }

    Buffer(Buffer&& other) {
        data_ = std::move(other.data_);
    }

    // 访问成员数据
    char* get_data() {
        if (data_.empty()) {
            return static_cast<char *>(nullptr);
        }
        return static_cast<char *>(data_.data());
    }

    uint32_t get_length() {
        return static_cast<uint32_t>(data_.size());
    }

private:
    std::vector<char> data_;


    // Buffer is non-copyable.
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};


#endif // __CORE_BUFFER_H__
