/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __CORE_MESSAGE_H__
#define __CORE_MESSAGE_H__

#include <endian.h>

#include <cstdint>
#include <string>

#include <Utils/Utils.h>

namespace tzrpc {

const static uint16_t kHeaderMagic      = 0x746b;
const static uint16_t kHeaderVersion    = 0x01;

struct Header {

    uint16_t magic;         // "tk" == 0x74 0x6b
    uint16_t version;       // "1"
    uint32_t length;        // playload length ( NOT include header)

    uint32_t rev1;          // 当前保留空间，后续升级使用
    uint32_t rev2;

    std::string dump() const {
        char msg[64] {};
        snprintf(msg, sizeof(msg), "mgc:%0x, ver:%0x, len:%u",
                 magic, version, length);
        return msg;
    }

    // 网络大端字节序
    void from_net_endian() {
        magic   = be16toh(magic);
        version = be16toh(version);
        length  = be32toh(length);
    }

    void to_net_endian() {
        magic   = htobe16(magic);
        version = htobe16(version);
        length  = htobe32(length);
    }

} __attribute__ ((__packed__));


struct Message {

    Header header_;
    std::string payload_;

    Message():
        header_({}),
        payload_({}) {
    }

    explicit Message(const std::string& data):
        header_({}),
        payload_(data) {
        header_.magic = kHeaderMagic;
        header_.version = kHeaderVersion;
        header_.length = data.size();
    }

    std::string dump() const {
        std::string ret = "header: " + header_.dump();
        ret += ", msg_len: " + convert_to_string(payload_.size());
        ret += "]]]";

        return ret;
    }

    std::string net_str() const {
        Header header = header_;
        header.to_net_endian();
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(Header));

        return header_str + payload_;
    }

};


} // end namespace tzrpc

#endif // __CORE_MESSAGE_H__
