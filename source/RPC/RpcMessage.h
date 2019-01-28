#ifndef __RPC_MESSAGE_H__
#define __RPC_MESSAGE_H__

#include <endian.h>

#include <cstdint>
#include <string>

#include <Utils/Utils.h>
#include <Scaffold/Setting.h>

namespace tzrpc {

const static uint16_t kRpcHeaderMagic      = 0x7472;
const static uint16_t kRpcHeaderVersion    = 0x01;


// Message已经能保证RPC的消息被完整的接收了，所以这边不需要保存msg的长度了
struct RpcHeader {

    uint16_t magic;         // "tr" == 0x74 0x72
    uint16_t version;       // "1"  == 0x01
    uint16_t service_id;
    uint16_t opcode;

    std::string dump() const {
        char msg[64] {};
        snprintf(msg, sizeof(msg), "mgc:%0x, ver:%0x, sid:%0x, opd:%0x",
                 magic, version, service_id, opcode);
        return msg;
    }

    // 网络大端字节序
    void from_net_endian() {
        magic   = be16toh(magic);
        version = be16toh(version);
        service_id = be16toh(service_id);
        opcode  = be16toh(opcode);
    }

    void to_net_endian() {
        magic   = htobe16(magic);
        version = htobe16(version);
        service_id = htobe16(service_id);
        opcode  = htobe16(opcode);
    }

} __attribute__ ((__packed__));


struct RpcMessage {

    RpcHeader header_;
    std::string playload_;

    RpcMessage():
        header_({}),
        playload_({}) {
    }

    explicit RpcMessage(uint16_t serviceid, uint16_t opcd, const std::string& data):
        header_({}),
        playload_(data) {
        header_.magic = kRpcHeaderMagic;
        header_.version = kRpcHeaderVersion;
        header_.service_id = serviceid;
        header_.opcode = opcd;
    }

    std::string dump() const {
        std::string ret = "rpc_header: " + header_.dump();
        ret += ", rpc_msg_len: " + convert_to_string(playload_.size());
        ret += "]]]";

        return ret;
    }

    // 转换成网络发送字节序
    std::string net_str() const {
        RpcHeader header = header_;
        header.to_net_endian();
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(RpcHeader));

        return header_str + playload_;
    }
};

static inline bool RpcMessageParse(const std::string& str, RpcMessage& rpc_message) {

    SAFE_ASSERT(str.size() >= sizeof(RpcHeader));
    if (str.size() < sizeof(RpcHeader)) {
        return false;
    }

    // 解析头部
    ::memcpy(reinterpret_cast<char*>(&rpc_message.header_), str.c_str(), sizeof(RpcHeader));
    rpc_message.header_.from_net_endian();

    rpc_message.playload_ = str.substr(sizeof(RpcHeader));
    return true;
}

} // end tzrpc

#endif // __RPC_MESSAGE_H__
