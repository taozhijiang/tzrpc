/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_INSTANCE_H__
#define __RPC_INSTANCE_H__

#include <memory>

#include <Core/Buffer.h>
#include <Network/TcpConnAsync.h>

#include <RPC/RpcRequestMessage.h>
#include <RPC/RpcResponseMessage.h>

namespace tzrpc {

class RpcInstance {
public:
    RpcInstance(const std::string& str_request, std::shared_ptr<TcpConnAsync> socket, int max_msg_size):
        start_(::time(NULL)),
        full_socket_(socket),
        request_(str_request),
        rpc_request_message_(),
        response_(),
        rpc_response_message_(),
        max_msg_size_(max_msg_size),
        service_id_(-1),
        opcode_(-1) {
    }

    bool validate_request();

    void reply_rpc_message(const std::string& msg);
    // 返回系统性的错误
    void reject(RpcResponseStatus status);
    // 返回业务相关的错误
    void return_biz_error();


    uint16_t get_service_id() {
        return service_id_;
    }

    uint16_t get_opcode() {
        return opcode_;
    }

    RpcRequestMessage& get_rpc_request_message() {
        return rpc_request_message_;
    }

private:
    time_t start_;  // 请求创建的时间
    std::weak_ptr<TcpConnAsync> full_socket_; // 可能socket提前在网络层已经释放了

    Buffer request_;
    RpcRequestMessage rpc_request_message_;

    Buffer response_;
    RpcResponseMessage rpc_response_message_;

    const int max_msg_size_;

private:
    // these detail info were extract from request
    uint16_t service_id_;
    uint16_t opcode_;
};

} // end namespace tzrpc


#endif // __RPC_INSTANCE_H__
