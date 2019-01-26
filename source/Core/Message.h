#ifndef __CORE_MESSAGE_H__
#define __CORE_MESSAGE_H__

#include <endian.h>

#include <cstdint>
#include <string>

namespace tzrpc {

const static uint16_t kHeaderMagic      = 0x746b;
const static uint16_t kHeaderVersion    = 0x1;

struct Header {

    uint16_t magic;         // "tk" == 0x74 0x6b
    uint16_t version;       // "1"
    uint32_t message_len;   // playload length ( NOT include header)
    uint64_t message_id;

    std::string dump() {
        char msg[64] {};
        snprintf(msg, sizeof(msg), "mgc:%0x, ver:%0x, len:%u, id:%lu",
                 magic, version, message_len, message_id);
        return msg;
    }

    // 网络大端字节序
    void from_net_endian() {
        magic   = be16toh(magic);
        version = be16toh(version);
        message_len = be32toh(message_len);
        message_id  = be64toh(message_id);
    }

    void to_net_endian() {
        magic   = htobe16(magic);
        version = htobe16(version);
        message_len = htobe32(message_len);
        message_id  = htobe64(message_id);
    }

} __attribute__ ((__packed__));


struct Message {

    Header header_;
    std::string playload_;

    Message():
        header_({}),
        playload_({}) {
    }

    Message(const std::string& data):
        header_({}),
        playload_(data) {
        header_.magic = kHeaderMagic;
        header_.version = kHeaderVersion;
        header_.message_len = data.size();
    }

    std::string dump() {
        std::string ret = "header: " + header_.dump();
        ret += ", msg: " + playload_;
        ret += "]]]";

        return ret;
    }

};


} // end tzrpc

#endif // __CORE_MESSAGE_H__
