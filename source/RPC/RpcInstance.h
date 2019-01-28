
#ifndef __RPC_INSTANCE_H__
#define __RPC_INSTANCE_H__

#include <memory>

#include <Core/Buffer.h>
#include <Network/TcpConnAsync.h>
#include <RPC/RpcMessage.h>

namespace tzrpc {

class RpcInstance {
public:
    RpcInstance(const std::string& str_request, std::shared_ptr<TcpConnAsync> socket):
        start_(::time(NULL)),
        full_socket_(socket),
        request_(str_request),
        rpc_message_(),
        response_(),
        service_id_(-1),
        opcode_(-1) {
    }

    bool validate_request() {

        if (request_.get_length() < sizeof(RpcHeader)) {
            return false;
        }

        // 解析头部
        std::string head_str;
        RpcHeader header;
        request_.retrive(head_str, sizeof(RpcHeader));
        ::memcpy(reinterpret_cast<char*>(&header), head_str.c_str(), sizeof(RpcHeader));
        header.from_net_endian();

        if (header.magic != kRpcHeaderMagic ||
            header.version != kRpcHeaderVersion ) {
            return false;
        }

        service_id_ = header.service_id;
        opcode_ = header.opcode;

        std::string msg_str;
        request_.retrive(msg_str, setting.max_msg_size_ - sizeof(RpcHeader));
        if (msg_str.empty()) {
            return false;
        }

        rpc_message_.header_ = header;
        rpc_message_.playload_ = msg_str;

        log_debug("validate/parse rpc_instance: %s", rpc_message_.dump().c_str());

        return true;
    }

    uint16_t get_service_id() {
        return service_id_;
    }

    uint16_t get_opcode() {
        return opcode_;
    }

    RpcMessage& get_rpc_message() {
        return rpc_message_;
    }

    bool reply_rpc_message(const std::string& msg) {

        RpcMessage rpc_message(service_id_, opcode_,  msg);
        Message net_msg(rpc_message.net_str());

        auto sock = full_socket_.lock();
        if (!sock) {
            log_err("socket already release before.");
            return false;
        }

        sock->async_send_message(net_msg);
        return true;
    }

private:
    time_t start_;  // 请求创建的时间
    std::weak_ptr<TcpConnAsync> full_socket_; // 可能socket提前在网络层已经释放了

    Buffer request_;
    RpcMessage rpc_message_;

    Buffer response_;

private:
    // these detail info were extract from request
    uint16_t service_id_;
    uint16_t opcode_;
};

} // tzrpc


#endif // __RPC_INSTANCE_H__
