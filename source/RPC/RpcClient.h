#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include <RPC/RpcMessage.h>
#include <Network/TcpConnSync.h>

namespace tzrpc {

class RpcClient {
public:
    RpcClient(const std::string& addr_ip, uint16_t addr_port):
        addr_ip_(addr_ip),
        addr_port_(addr_port),
        request_(),
        response_(),
        service_id_(-1),
        opcode_(-1) {
    }

    bool send_rpc_message(const RpcMessage& rpc_message);
    bool recv_rpc_message(RpcMessage& rpc_message);

    int call_RPC(uint16_t service_id, uint16_t opcode, const std::string& playload, std::string& respload);

private:
    time_t start_;        // 请求创建的时间
    const std::string addr_ip_;
    const uint16_t addr_port_;

    std::shared_ptr<TcpConnSync> conn_;

    Buffer request_;
    Buffer response_;

private:
    // these detail info were extract from request
    uint16_t service_id_;
    uint16_t opcode_;
};

}  // end tzrpc


#endif // __RPC_CLIENT_H__
