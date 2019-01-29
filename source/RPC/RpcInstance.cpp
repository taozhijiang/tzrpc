
#include <RPC/RpcInstance.h>

namespace tzrpc {

bool RpcInstance::validate_request() {

    if (request_.get_length() < sizeof(RpcRequestHeader)) {
        return false;
    }

    // 解析头部
    std::string head_str;
    RpcRequestHeader header;
    request_.retrive(head_str, sizeof(RpcRequestHeader));
    ::memcpy(reinterpret_cast<char*>(&header), head_str.c_str(), sizeof(RpcRequestHeader));
    header.from_net_endian();

    if (header.magic != kRpcHeaderMagic ||
        header.version != kRpcHeaderVersion ) {
        return false;
    }

    service_id_ = header.service_id;
    opcode_ = header.opcode;

    std::string msg_str;
    request_.retrive(msg_str, setting.max_msg_size_ - sizeof(RpcRequestHeader));
    if (msg_str.empty()) {
        return false;
    }

    rpc_request_message_.header_ = header;
    rpc_request_message_.payload_ = msg_str;

    log_debug("validate/parse request_message for rpc_instance: %s", rpc_request_message_.dump().c_str());

    return true;
}


void RpcInstance::reply_rpc_message(const std::string& msg) {

    RpcResponseMessage rpc_response_message(service_id_, opcode_,  msg);
    Message net_msg(rpc_response_message.net_str());

    auto sock = full_socket_.lock();
    if (!sock) {
        log_err("socket already release before.");
        return;
    }

    sock->async_send_message(net_msg);
    return;
}

void RpcInstance::reject(RpcResponseStatus status) {

    RpcResponseMessage rpc_response_message(status);
    Message net_msg(rpc_response_message.net_str());

    auto sock = full_socket_.lock();
    if (!sock) {
        log_err("socket already release before.");
        return;
    }

    sock->async_send_message(net_msg);
    return;
}



} // end tzrpc
