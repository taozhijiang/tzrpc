
#include <Utils/Log.h>
#include <Core/ProtoBuf.h>

#include "XtraTaskService.h"

namespace tzrpc {

void XtraTaskService::handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) override {

    using XtraTask::OpCode;

    // Call the appropriate RPC handler based on the request's opCode.
    switch (rpc_instance->get_opcode()) {
        case OpCode::CMD_READ:
            read_ops_impl(rpc_instance);
            break;
        case OpCode::CMD_WRITE:
            write_ops_impl(rpc_instance);
            break;

        default:
            log_err("Received RPC request with unknown opcode %u: "
                    "rejecting it as invalid request",
                    rpc_instance->get_opcode());
            rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
    }
}


void XtraTaskService::read_ops_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    // 再做一次opcode校验
    RpcRequestMessage& rpc_request_message = rpc_instance->get_rpc_request_message();
    if (rpc_request_message.header_.opcode != XtraTask::OpCode::CMD_READ) {
        log_err("invalid opcode %u in service XtraTask.", rpc_request_message.header_.opcode);
        rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
    }

    XtraTask::XtraReadOps::Response response;
    response.set_code(0);
    response.set_msg("OK");

    do {

        // 消息体的unmarshal
        XtraTask::XtraReadOps::Request request;
        if (!ProtoBuf::unmarshalling_from_string(rpc_request_message.payload_, &request)) {
            log_err("unmarshal request failed.");
            response.set_code(-1);
            response.set_msg("参数错误");
            break;
        }

        // 相同类目下的子RPC分发
        if (request.has_ping()) {
            log_debug("XtraTask::XtraReadOps::ping -> %s", request.ping().msg().c_str());
            response.mutable_ping()->set_msg("[[[pong]]]");
            break;
        } else if (request.has_gets()) {
            log_debug("XtraTask::XtraReadOps::get -> %s", request.gets().key().c_str());
            response.mutable_gets()->set_value("[[[pong]]]");
            break;
        } else if (request.has_echo()) {
            std::string real_msg = request.echo().msg();
            log_debug("XtraTask::XtraReadOps::echo -> %s", real_msg.c_str());
            response.mutable_echo()->set_msg("echo:" + real_msg);
        } else {
            log_err("undetected specified service call.");
            rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
            return;
        }

    } while (0);

    std::string response_str;
    ProtoBuf::marshalling_to_string(response, &response_str);
    rpc_instance->reply_rpc_message(response_str);
}

void XtraTaskService::write_ops_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    #if 0
    PRELUDE(XtraWriteCmd);


    rpc.reply(response);
    #endif

    const RpcRequestMessage& msg = rpc_instance->get_rpc_request_message();
    log_debug(" write ops recv: %s", msg.dump().c_str());

    rpc_instance->reply_rpc_message("nicol_write_reply");
}


} // namespace tzrpc
