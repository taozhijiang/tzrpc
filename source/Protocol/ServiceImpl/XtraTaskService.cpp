
#include <Utils/Log.h>

#include "XtraTaskService.h"

namespace tzrpc {

void XtraTaskService::handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) override {

    using XtraTask::OpCode;

    // Call the appropriate RPC handler based on the request's opCode.
    switch (rpc_instance->get_opcode()) {
        case OpCode::CMD_GET:
            read_impl(rpc_instance);
            break;
        case OpCode::CMD_SET:
            write_impl(rpc_instance);
            break;

        default:
            log_err("Received RPC request with unknown opcode %u: "
                    "rejecting it as invalid request",
                    rpc_instance->get_opcode());
    }
}


/**
 * Place this at the top of each RPC handler. Afterwards, 'request' will refer
 * to the protocol buffer for the request with all required fields set.
 * 'response' will be an empty protocol buffer for you to fill in the response.
 */
#define PRELUDE(rpcClass) \
    XtraTask::rpcClass::Request request; \
    XtraTask::rpcClass::Response response; \
    if (!rpc.getRequest(request)) \
        return;

////////// RPC handlers //////////


void XtraTaskService::read_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    #if 0
    PRELUDE(XtraReadOps);

    rpc.reply(response);
    #endif

    const RpcRequestMessage& msg = rpc_instance->get_rpc_request_message();
    log_debug(" read ops recv: %s", msg.dump().c_str());

    rpc_instance->reply_rpc_message("nicol_read_reply");
}

void XtraTaskService::write_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    #if 0
    PRELUDE(XtraWriteCmd);


    rpc.reply(response);
    #endif

    const RpcRequestMessage& msg = rpc_instance->get_rpc_request_message();
    log_debug(" write ops recv: %s", msg.dump().c_str());

    rpc_instance->reply_rpc_message("nicol_write_reply");
}


} // namespace tzrpc
