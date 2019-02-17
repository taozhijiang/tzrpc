/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#include <memory>
#include <string>

#include "RpcClientStatus.h"

namespace tzrpc_client {

struct RpcClientSetting {
    std::string addr_ip_;
    uint16_t    addr_port_;
    uint32_t    max_msg_size_;
    uint32_t    client_ops_cancel_time_out_;
};

// class forward
class RpcClientImpl;

class RpcClient {

public:

    RpcClient(const std::string& addr_ip, uint16_t addr_port);
    ~RpcClient();


    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload);

private:
    RpcClientSetting client_setting_;
    std::unique_ptr<RpcClientImpl> impl_;
};

}  // end namespace tzrpc_client


#endif // __RPC_CLIENT_H__
