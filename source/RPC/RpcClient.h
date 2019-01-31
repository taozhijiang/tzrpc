#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include <Network/TcpConnSync.h>

#include <RPC/RpcClientStatus.h>

namespace tzrpc {

class RpcRequestMessage;
class RpcResponseMessage;

struct Error {
    int32_t status;
    std::string errmsg;
};

class RpcClient {
public:
    RpcClient(const std::string& addr_ip, uint16_t addr_port):
        addr_ip_(addr_ip),
        addr_port_(addr_port) {
    }

    ~RpcClient();

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload);

private:
    bool send_rpc_message(const RpcRequestMessage& rpc_request_message);
    bool recv_rpc_message(Message& net_message);

    time_t start_;        // 请求创建的时间
    const std::string addr_ip_;
    const uint16_t addr_port_;

    std::shared_ptr<TcpConnSync> conn_;
};

}  // end tzrpc


#endif // __RPC_CLIENT_H__
