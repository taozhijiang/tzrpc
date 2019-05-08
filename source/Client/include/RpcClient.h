/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include <libconfig.h++>

#include <memory>
#include <string>

#include "RpcClientStatus.h"


namespace tzrpc_client {



// RPC异步调用的回调函数，status是请求处理状态，rsp是服务端返回的数据
// 如果发生了异常，那么status会给予提示
// 注意：由于是异步处理，所以在高流量的请求下不能保证响应按照原请求的顺序得到执行，所以
//       rcp_handler_t没有保留req信息
typedef std::function<int(const RpcClientStatus status, uint16_t service_id, uint16_t opcode, const std::string& rsp)> rpc_handler_t;
extern rpc_handler_t dummy_handler_;

struct RpcClientSetting {

    std::string serv_addr_;
    uint32_t    serv_port_;

    uint32_t    send_max_msg_size_;
    uint32_t    recv_max_msg_size_;

    uint32_t    log_level_;

    rpc_handler_t handler_;

    RpcClientSetting() :
        serv_addr_(),
        serv_port_(),
        send_max_msg_size_(0),
        recv_max_msg_size_(0),
        log_level_(7),
        handler_() {
    }

} __attribute__((aligned(4)));


// class forward
class RpcClientImpl;

class RpcClient {

public:

    RpcClient();
    ~RpcClient();

    // 禁止拷贝
    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    RpcClient(const std::string& addr, uint16_t port, const rpc_handler_t& handler = dummy_handler_);
    RpcClient(const std::string& cfgFile, const rpc_handler_t& handler = dummy_handler_);
    RpcClient(const libconfig::Setting& setting, const rpc_handler_t& handler = dummy_handler_);

    // 带客户端超时支持
    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload,
                             uint32_t timeout_sec = 0);

    // 异步调用的接口，底层调用完成后会自动调用handler来处理
    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload,
                             uint32_t timeout_sec = 0);

private:

    bool init(const std::string& addr, uint16_t port);
    bool init(const std::string& cfgFile);
    bool init(const libconfig::Setting& setting);

    bool initialized_;
    RpcClientSetting client_setting_;

    std::shared_ptr<RpcClientImpl> impl_;
};

}  // end namespace tzrpc_client


#endif // __RPC_CLIENT_H__
