/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_IMPL_H__
#define __RPC_CLIENT_IMPL_H__

#include <boost/asio/steady_timer.hpp>
using boost::asio::steady_timer;

#include <other/Log.h>

#include <concurrency/IoService.h>
#include <Client/include/RpcClientStatus.h>
#include <Client/include/RpcClient.h>

namespace tzrpc {

class RpcRequestMessage;
class RpcResponseMessage;
class Message;

}

namespace tzrpc_client {

class TcpConnSync;
class TcpConnAsync;

///////////////////////////
//
// 实现类 RpcClientImpl
//
//////////////////////////

typedef std::function<void(const tzrpc::Message& net_message)> rpc_wrapper_t;

class RpcClientImpl : public std::enable_shared_from_this<RpcClientImpl> {
public:
    RpcClientImpl(const RpcClientSetting& client_setting) :
        client_setting_(client_setting),
        io_service_(),
        roo_io_service_(),
        call_mutex_(),
        time_start_(0),
        was_timeout_(false),
        rpc_call_timer_(),
        conn_sync_(),
        conn_async_(),
        handler_() {
    }

    ~RpcClientImpl();

    // 可以传递出来给其他模块使用
    std::shared_ptr<boost::asio::io_service> get_ioservice() const {
        return client_setting_.io_service_;
    }

    bool init();

    // 禁止拷贝
    RpcClientImpl(const RpcClientImpl&) = delete;
    RpcClientImpl& operator=(const RpcClientImpl&) = delete;

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload,
                             uint32_t timeout_sec);

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload,
                             uint32_t timeout_sec);

private:

    RpcClientSetting client_setting_;
    std::shared_ptr<boost::asio::io_service> io_service_;
    std::unique_ptr<roo::IoService> roo_io_service_;
    

    // 确保不会客户端多线程调用，导致底层的连接串话
    std::mutex call_mutex_;

    bool send_rpc_message(const tzrpc::RpcRequestMessage& rpc_request_message);
    bool recv_rpc_message(tzrpc::Message& net_message);

    //
    // rpc调用超时相关的配置
    //
    time_t time_start_;        // 请求创建的时间
    bool was_timeout_;
    std::unique_ptr<steady_timer> rpc_call_timer_;
    void set_rpc_call_timeout(uint32_t msec, bool sync); // 区分同步和异步的操作超时
    void rpc_call_timeout(const boost::system::error_code& ec, bool sync);

    // 请求到达后按照需求自动创建
    std::shared_ptr<TcpConnSync>  conn_sync_;

    // 异步处理的连接

    bool send_rpc_message_async(const tzrpc::RpcRequestMessage& rpc_request_message);

    // 这里进行一些RPC数据包的解析操作，业务层不做包细节的处理
    void async_recv_wrapper(const tzrpc::Message& net_message);
    std::shared_ptr<TcpConnAsync> conn_async_;
    rpc_handler_t handler_;
};


} // end namespace tzrpc_client


#endif // __RPC_CLIENT_IMPL_H__
