/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include <libconfig.h++>
#include <syslog.h>

#include <memory>
#include <string>

#include "RpcClientStatus.h"


typedef void(* CP_log_store_func_t)(int priority, const char *format, ...);


namespace tzrpc_client {

extern CP_log_store_func_t checkpoint_log_store_func_impl_;
void set_checkpoint_log_store_func(CP_log_store_func_t func);


struct RpcClientSetting {

    std::string serv_addr_;
    uint32_t    serv_port_;

    uint32_t    send_max_msg_size_;
    uint32_t    recv_max_msg_size_;

    uint32_t    log_level_;

    RpcClientSetting():
        serv_addr_(),
        serv_port_(),
        send_max_msg_size_(0),
        recv_max_msg_size_(0),
        log_level_(7) {
    }

} __attribute__ ((aligned (4)));

// class forward
class RpcClientImpl;

class RpcClient {

public:

    RpcClient();
    ~RpcClient();

    // 禁止拷贝
    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    RpcClient(const std::string& addr, uint16_t port, CP_log_store_func_t log_func = syslog);
    RpcClient(const std::string& cfgFile, CP_log_store_func_t log_func = syslog);
    RpcClient(const libconfig::Setting& setting, CP_log_store_func_t log_func = syslog);

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload);

    // 带客户端超时支持
    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload,
                             uint32_t timeout_sec);

private:

    bool init(const std::string& addr, uint16_t port, CP_log_store_func_t log_func);
    bool init(const std::string& cfgFile, CP_log_store_func_t log_func);
    bool init(const libconfig::Setting& setting, CP_log_store_func_t log_func);

    bool initialized_;
    RpcClientSetting client_setting_;

    std::shared_ptr<RpcClientImpl> impl_;
};

}  // end namespace tzrpc_client


#endif // __RPC_CLIENT_H__
