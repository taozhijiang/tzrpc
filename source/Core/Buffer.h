#ifndef __CORE_BUFFER_H__
#define __CORE_BUFFER_H__

#include <cstdint>
#include <string>

#include <memory>
#include <boost/noncopyable.hpp>

#include <xtra_rhel6.h>

#include <Core/Message.h>

namespace tzrpc {

class Buffer: public boost::noncopyable {

public:
    // 构造函数

    Buffer():
        data_ ({}) {
    }

    explicit Buffer(const std::string& data):
        data_(data.begin(), data.end()) {
    }

    explicit Buffer(const Message& msg):
        data_({}) {
        Header header = msg.header_;
        header.to_net_endian();

        std::string header_str(reinterpret_cast<char*>(&header), sizeof(Header));
        append_internal(header_str);
        append_internal(msg.payload_);
    }

    // used internally, user should prefer Message
    uint32_t append_internal(const std::string& data) {
        std::copy(data.begin(), data.end(), std::back_inserter(data_));
        return static_cast<uint32_t>(data_.size());
    }


    uint32_t append(const Message& msg) {
        Header header = msg.header_;
        header.to_net_endian();

        std::string header_str(reinterpret_cast<char*>(&header), sizeof(Header));
        append_internal(header_str);
        append_internal(msg.payload_);
        return static_cast<uint32_t>(data_.size());
    }

    // 从队列的开头取出若干个字符，返回实际得到的字符数
    bool retrive(std::string& store, uint32_t sz) {
        if (sz == 0 || data_.empty()) {
            return false;
        }

        sz = sz > data_.size() ? data_.size() : sz;
        std::vector<char> tmp (data_.begin() + sz, data_.end());    // 剩余的数据
        store = std::string(data_.begin(), data_.begin() + sz);     // 要取的数据

        data_.swap(tmp);
        return true;
    }

    bool retrive(std::vector<char>& store, uint32_t sz) {
        if (sz == 0 || data_.empty()) {
            return false;
        }

        sz = sz > data_.size() ? data_.size() : sz;
        std::vector<char> tmp (data_.begin() + sz, data_.end());    // 剩余的数据
        store.assign(data_.begin(), data_.begin() + sz);     // 要取的数据

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
};

} // end tzrpc

#endif // __CORE_BUFFER_H__
