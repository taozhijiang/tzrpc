/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <xtra_asio.h>

#include "include/RpcClient.h"

#include <Core/Message.h>

#include <RPC/RpcRequestMessage.h>
#include <RPC/RpcResponseMessage.h>

#include <Client/TcpConnSync.h>
#include <Client/LogClient.h>


namespace tzrpc_client {

using namespace tzrpc;

// 创建一个静态对象，用于全部的同步IO请求
static boost::asio::io_service IO_SERVICE;


///////////////////////////
//
// 实现类 RpcClientImpl
//
//////////////////////////


class RpcClientImpl {
public:
    RpcClientImpl(const RpcClientSetting& client_setting):
        client_setting_(client_setting),
        was_timeout_(false),
        rpc_call_timer_() {
    }

    ~RpcClientImpl() {

        // 客户端必须主动触发这个socket的io取消
        // 和关闭操作，否则因为shared_from_this()导致客户端析钩之后
        // 对应的socket还没有释放，从而体现的状况就是在客户端和服务端
        // 之间建立了大量的socket连接
        if (conn_) {
            conn_->shutdown_socket();
        }
    }

    RpcClientStatus call_RPC(uint16_t service_id, uint16_t opcode,
                             const std::string& payload, std::string& respload,
                             uint32_t timeout_sec);

private:
    bool send_rpc_message(const RpcRequestMessage& rpc_request_message) {
        Message net_msg(rpc_request_message.net_str());
        return conn_->send_net_message(net_msg);
    }

    bool recv_rpc_message(Message& net_message) {
        return conn_->recv_net_message(net_message);
    }

    RpcClientSetting client_setting_;

    time_t start_;        // 请求创建的时间
    bool was_timeout_;
    std::unique_ptr<steady_timer> rpc_call_timer_;
    void set_rpc_call_timeout(uint32_t msec);
    void rpc_call_timeout(const boost::system::error_code& ec);

    std::shared_ptr<TcpConnSync> conn_;
};

void RpcClientImpl::set_rpc_call_timeout(uint32_t sec) {

    if (sec == 0) {
        return;
    }

    boost::system::error_code ignore_ec;
    if (rpc_call_timer_) {
        rpc_call_timer_->cancel(ignore_ec);
    } else {
        rpc_call_timer_.reset(new steady_timer(io_service_));
    }

    SAFE_ASSERT( sec > 0 );
    was_timeout_ = false;
    rpc_call_timer_->expires_from_now(boost::chrono::seconds(sec));
    rpc_call_timer_->async_wait(std::bind(&RpcClientImpl::rpc_call_timeout, shared_from_this(),
                                           std::placeholders::_1));
}

void RpcClientImpl::rpc_call_timeout(const boost::system::error_code& ec) {

    if (ec == 0){
        log_info("rpc_call_timeout called, call start at %lu", start_);
        was_timeout_ = true;
        conn_->ops_cancel();
    } else if ( ec == boost::asio::error::operation_aborted) {
        // normal cancel
    } else {
        log_err("unknown and won't handle error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
}

RpcClientStatus RpcClientImpl::call_RPC(uint16_t service_id, uint16_t opcode,
                                        const std::string& payload, std::string& respload,
                                        uint32_t timeout_sec) {
    if (!conn_) {

        boost::system::error_code ec;
        std::shared_ptr<ip::tcp::socket> socket_ptr = std::make_shared<ip::tcp::socket>(IO_SERVICE);

        socket_ptr->connect(ip::tcp::endpoint(ip::address::from_string(client_setting_.addr_ip_), client_setting_.addr_port_), ec);
        if (ec) {
            log_err("connect to %s:%u failed with {%d} %s.",
                    client_setting_.addr_ip_.c_str(), client_setting_.addr_port_,
                    ec.value(), ec.message().c_str() );
            return RpcClientStatus::NETWORK_CONNECT_ERROR;

        }

        conn_.reset(new TcpConnSync(socket_ptr, IO_SERVICE, client_setting_));
        if (!conn_) {
            log_err("create socket %s:%u failed.",
                    client_setting_.addr_ip_.c_str(), client_setting_.addr_port_);
            return RpcClientStatus::NETWORK_BEFORE_ERROR;
        }
    }

    start_ = ::time(NULL);

    // 构建请求包
    RpcRequestMessage rpc_request_message(service_id, opcode, payload);

    if (timeout_sec > 0) {
        set_rpc_call_timeout(timeout_sec);
    }

    // 发送请求报文
    if(!send_rpc_message(rpc_request_message)){
        conn_.reset();
        if (was_timeout_) {
            log_err("rpc_call was timeout with %d sec, start before %lu", timeout_sec, (::time(NULL) - start_));
            return RpcClientStatus::RPC_CALL_TIMEOUT;
        }
        return RpcClientStatus::NETWORK_SEND_ERROR;
    }

    // 接收报文
    Message net_message;
    if(!recv_rpc_message(net_message)){
        conn_.reset();
        if (was_timeout_) {
            log_err("rpc_call was timeout with %d sec, start before %lu", timeout_sec, (::time(NULL) - start_));
            return RpcClientStatus::RPC_CALL_TIMEOUT;
        }
        return RpcClientStatus::NETWORK_RECV_ERROR;
    }

    // 解析报文
    RpcResponseMessage rpc_response_message;
    if (!RpcResponseMessageParse(net_message.payload_, rpc_response_message)) {
        return RpcClientStatus::RECV_FORMAT_ERROR;
    }

    // 返回参数校验
    if (rpc_response_message.header_.magic != kRpcHeaderMagic ||
         // rpc_response_message.header_.version != kRpcHeaderVersion ||
        rpc_response_message.header_.service_id != service_id ||
        rpc_response_message.header_.opcode != opcode ) {
        log_err("rpc_response_message header check error: %s", rpc_response_message.header_.dump().c_str());
        return RpcClientStatus::RECV_FORMAT_ERROR;
    }

    // Service Status 校验
    if (rpc_response_message.header_.status != RpcResponseStatus::OK) {
        log_err("ServiceSide status: %u", rpc_response_message.header_.status);
        return static_cast<RpcClientStatus>(static_cast<uint8_t>(rpc_response_message.header_.status));
    }

    respload = rpc_response_message.payload_;
    return RpcClientStatus::OK;
}


///////////////////////////
//
// 接口类 RpcClient
//
//////////////////////////

RpcClient::RpcClient(const std::string& addr_ip, uint16_t addr_port):
    client_setting_(),
    impl_() {

    client_setting_.addr_ip_ = addr_ip;
    client_setting_.addr_port_ = addr_port;
    client_setting_.max_msg_size_ = 0;
    client_setting_.client_ops_cancel_time_out_ = 20;

    impl_.reset(new RpcClientImpl(client_setting_));
}

RpcClient::~RpcClient() {}


RpcClientStatus RpcClient::call_RPC(uint16_t service_id, uint16_t opcode,
                                    const std::string& payload, std::string& respload ) {
    if (!impl_) {
        log_err("RpcClientImpl not initialized, please check.");
        return RpcClientStatus::NETWORK_BEFORE_ERROR;
    }

    return impl_->call_RPC(service_id, opcode, 0, payload, respload);
}

RpcClientStatus RpcClient::call_RPC(uint16_t service_id, uint16_t opcode,
                                    const std::string& payload, std::string& respload,
                                    uint32_t timeout_sec) {
    if (!impl_) {
        log_err("RpcClientImpl not initialized, please check.");
        return RpcClientStatus::NETWORK_BEFORE_ERROR;
    }

    return impl_->call_RPC(service_id, opcode, 0, payload, respload);
}

} // end namespace tzrpc_client
