/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_REQUEST_MESSAGE_H__
#define __RPC_REQUEST_MESSAGE_H__

#include <xtra_rhel.h>

#include <endian.h>

#include <cstdint>
#include <string>

namespace tzrpc {

const uint16_t kRpcHeaderMagic      = 0x7472;
const uint16_t kRpcHeaderVersion    = 0x01;


// Message已经能保证RPC的消息被完整的接收了，所以这边不需要保存msg的长度了
struct RpcRequestHeader {

    uint16_t magic;         // "tr" == 0x74 0x72
    uint16_t version;       // "1"  == 0x01
    uint16_t service_id;
    uint16_t opcode;

    uint32_t rev1;          // 当前保留空间，后续升级使用
    uint32_t rev2;

    std::string dump() const {
        char msg[64]{};
        snprintf(msg, sizeof(msg), "rpc_request_header mgc:%0x, ver:%0x, sid:%0x, opd:%0x ",
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

} __attribute__((__packed__));


struct RpcRequestMessage {

    RpcRequestHeader header_;
    std::string payload_;

    RpcRequestMessage() :
        header_({ }),
        payload_({ }) {
    }

    RpcRequestMessage(uint16_t serviceid, uint16_t opcd, const std::string& data) :
        header_({ }),
        payload_(data) {
        header_.magic = kRpcHeaderMagic;
        header_.version = kRpcHeaderVersion;
        header_.service_id = serviceid;
        header_.opcode = opcd;
    }

    std::string dump() const {
        std::string ret = "rpc_request_header: " + header_.dump();
        ret += ", rpc_request_message_len: " +
            std::to_string(static_cast<long long unsigned int>(payload_.size()));
        ret += ".";

        return ret;
    }

    // 转换成网络发送字节序
    std::string net_str() const {
        RpcRequestHeader header = header_;
        header.to_net_endian();
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(RpcRequestHeader));

        return header_str + payload_;
    }
};

static inline bool RpcRequestMessageParse(const std::string& str, RpcRequestMessage& rpc_request_message) {

    SAFE_ASSERT(str.size() >= sizeof(RpcRequestHeader));
    if (str.size() < sizeof(RpcRequestHeader)) {
        return false;
    }

    // 解析头部
    ::memcpy(reinterpret_cast<char*>(&rpc_request_message.header_), str.c_str(), sizeof(RpcRequestHeader));
    rpc_request_message.header_.from_net_endian();

    rpc_request_message.payload_ = str.substr(sizeof(RpcRequestHeader));
    return true;
}

} // end namespace tzrpc

#endif // __RPC_REQUEST_MESSAGE_H__
