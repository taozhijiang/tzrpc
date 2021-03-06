/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __CORE_BUFFER_H__
#define __CORE_BUFFER_H__

#include <cstdint>
#include <string>
#include <memory>

#include <xtra_rhel.h>

#include <Core/Message.h>

namespace tzrpc {

class Buffer {

    __noncopyable__(Buffer)

public:
    // 构造函数

    Buffer() :
        data_({ }) {
    }

    explicit Buffer(const std::string& data) :
        data_(data.begin(), data.end()) {
    }

    explicit Buffer(const Message& msg) :
        data_({ }) {
        Header header = msg.header_;
        header.to_net_endian();

        std::string header_str(reinterpret_cast<char*>(&header), sizeof(Header));
        append_internal(header_str);
        append_internal(msg.payload_);
    }

    ~Buffer() = default;


    // used internally, user should prefer Message
    // 内部使用的接口，用户应该只使用下面的Message重载版本
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

    // 从队列的开头取出若干个(最多sz)字符，如果有数据返回就true
    bool consume(std::string& store, uint32_t sz) {

        if (sz == 0 || data_.empty()) {
            return false;
        }

        // 全部取走的优化操作
        if (sz >= data_.size()) {
            store = std::string(data_.begin(), data_.end());
            data_.clear();
            return true;
        }

        // 部分数据的情况
        std::vector<char> remain(data_.begin() + sz, data_.end());    // 剩余的数据
        store = std::string(data_.begin(), data_.begin() + sz);    // 要取的数据

        data_.swap(remain);
        return true;
    }

    // 调用者需要保证至少能够容纳 sz 数据，拷贝的同时原始数据不会改动
    // 需要使用下面的front_erase进行清除
    bool consume(char* store, uint32_t sz) {

        if (!store || sz == 0 || data_.empty()) {
            return false;
        }

        ::memcpy(store, data_.data(), sz);

        // 之前的设计思路:
        // 先将send_bound_中的数据拷贝到io_block_中进行发送，然后根据传输的结果
        // 从send_bound_中将这部份数据移走，没有发送成功的数据可以重发
        // 但是在异步调用中，调用1发起-调用2发起-调用1回调之间会产生竞争条件，导致
        // 调用2实际发送的是调用1没有移走的数据
        // 实际上boost::asio中是通过transfer_exactly发送的，如果返回时没有发送
        // 这么多数据，那么应该是网络层出现问题了，此时就直接socket错误返回了，不再
        // 考虑发送量小于请求量这种部分发送的情形了。
        front_erase(sz);
        return true;
    }

    void front_erase(uint32_t sz) {

        if (sz >= data_.size()) {
            data_.clear();
            return;
        }

        std::vector<char> remain(data_.begin() + sz, data_.end());    // 剩余的数据
        data_.swap(remain);
    }

    // 访问内部原始的字符串成员数据
    char* get_data() {
        if (data_.empty()) {
            return static_cast<char*>(nullptr);
        }
        return static_cast<char*>(data_.data());
    }

    uint32_t get_length() {
        return static_cast<uint32_t>(data_.size());
    }

private:

    std::vector<char> data_;
};

} // end namespace tzrpc

#endif // __CORE_BUFFER_H__
