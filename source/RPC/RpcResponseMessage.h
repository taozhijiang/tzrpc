/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_RESPONSE_MESSAGE_H__
#define __RPC_RESPONSE_MESSAGE_H__

#include <endian.h>

#include <cstdint>
#include <string>

#include <Utils/Utils.h>

namespace tzrpc {

extern const uint16_t kRpcHeaderMagic;
extern const uint16_t kRpcHeaderVersion;

enum class RpcResponseStatus : uint8_t {

    OK = 0,

    /**
     * An error specific to the particular service. The format of the remainder
     * of the message is specific to the particular service.
     * 暂未使用
     */
    SERVICE_SPECIFIC_ERROR = 1,

    INVALID_VERSION = 2,

    INVALID_SERVICE = 3,

    INVALID_REQUEST = 4,
    SYSTEM_ERROR    = 5,
};

// Message已经能保证RPC的消息被完整的接收了，所以这边不需要保存msg的长度了
struct RpcResponseHeader {

    RpcResponseStatus   status;

    uint16_t magic;         // "tr" == 0x74 0x72
    uint16_t version;       // "1"  == 0x01
    uint16_t service_id;
    uint16_t opcode;

    uint32_t rev1;          // 当前保留空间，后续升级使用
    uint32_t rev2;

    std::string dump() const {
        char msg[64] {};
        snprintf(msg, sizeof(msg), "rpc_response_header mgc:%0x, ver:%0x, sid:%0x, opd:%0x",
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


struct RpcResponseMessage {

    RpcResponseHeader header_;
    std::string payload_;

    RpcResponseMessage():
        header_({}),
        payload_({}) {
    }

    RpcResponseMessage(uint16_t serviceid, uint16_t opcd, const std::string& data):
        header_({}),
        payload_(data) {
        header_.status = RpcResponseStatus::OK;
        header_.magic = kRpcHeaderMagic;
        header_.version = kRpcHeaderVersion;
        header_.service_id = serviceid;
        header_.opcode = opcd;
    }

    explicit RpcResponseMessage(enum RpcResponseStatus status):
        header_({}),
        payload_() {
        header_.status = status;
        header_.magic = kRpcHeaderMagic;
        header_.version = kRpcHeaderVersion;
    }

    std::string dump() const {
        std::string ret = "rpc_response_header: " + header_.dump();
        ret += ", rpc_response_message_len: " + convert_to_string(payload_.size());
        ret += ".";

        return ret;
    }

    // 转换成网络发送字节序
    std::string net_str() const {
        RpcResponseHeader header = header_;
        header.to_net_endian();
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(RpcResponseHeader));

        return header_str + payload_;
    }
};

static inline bool RpcResponseMessageParse(const std::string& str, RpcResponseMessage& rpc_response_message) {

    SAFE_ASSERT(str.size() >= sizeof(RpcResponseHeader));
    if (str.size() < sizeof(RpcResponseHeader)) {
        return false;
    }

    // 解析头部
    ::memcpy(reinterpret_cast<char*>(&rpc_response_message.header_), str.c_str(), sizeof(RpcResponseHeader));
    rpc_response_message.header_.from_net_endian();

    rpc_response_message.payload_ = str.substr(sizeof(RpcResponseHeader));
    return true;
}

} // end namespace tzrpc

#endif // __RPC_RESPONSE_MESSAGE_H__
