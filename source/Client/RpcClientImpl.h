/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_IMPL_H__
#define __RPC_CLIENT_IMPL_H__

#include <xtra_asio.h>

#include <Client/LogClient.h>
#include <Client/include/RpcClientStatus.h>
#include <Client/include/RpcClient.h>

namespace tzrpc {

class RpcRequestMessage;
class RpcResponseMessage;
class Message;

}

namespace tzrpc_client {

class TcpConnSync;

///////////////////////////
//
// 实现类 RpcClientImpl
//
//////////////////////////

class RpcClientImpl: public std::enable_shared_from_this<RpcClientImpl> {
public:
    RpcClientImpl(const RpcClientSetting& client_setting):
        client_setting_(client_setting),
        time_start_(0),
        was_timeout_(false),
        rpc_call_timer_(),
        conn_() {
    }

    ~RpcClientImpl();

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload,
                             uint32_t timeout_sec);

private:
    bool send_rpc_message(const tzrpc::RpcRequestMessage& rpc_request_message);
    bool recv_rpc_message(tzrpc::Message& net_message);

    RpcClientSetting client_setting_;

    //
    // rpc调用超时相关的配置
    //
    time_t time_start_;        // 请求创建的时间
    bool was_timeout_;
    std::unique_ptr<steady_timer> rpc_call_timer_;
    void set_rpc_call_timeout(uint32_t msec);
    void rpc_call_timeout(const boost::system::error_code& ec);

    // 请求到达后按照需求自动创建
    std::shared_ptr<TcpConnSync> conn_;
};


} // end namespace tzrpc_client


#endif // __RPC_CLIENT_IMPL_H__
