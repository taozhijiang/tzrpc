/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <xtra_rhel.h>

#include <Core/Message.h>

#include <RPC/RpcRequestMessage.h>
#include <RPC/RpcResponseMessage.h>


#include <Client/RpcClientImpl.h>
#include <Client/TcpConnSync.h>
#include <Client/TcpConnAsync.h>

using tzrpc::Message;
using tzrpc::RpcRequestMessage;
using tzrpc::RpcResponseMessage;
using tzrpc::RpcResponseStatus;
using tzrpc::kRpcHeaderVersion;
using tzrpc::kRpcHeaderMagic;


namespace tzrpc_client {

bool RpcClientImpl::init() {

    handler_ = client_setting_.handler_;

    io_service_ = make_unique<roo::IoService>();
    if (!io_service_ || !io_service_->init() ) {
        log_err("create and initialized IoService failed.");
        return false;
    }

    return true;
}

RpcClientImpl::~RpcClientImpl() {

    // 客户端必须主动触发这个socket的io取消
    // 和关闭操作，否则因为shared_from_this()导致客户端析钩之后
    // 对应的socket还没有释放，从而体现的状况就是在客户端和服务端
    // 之间建立了大量的socket连接
    if (conn_sync_) {
        conn_sync_->shutdown_and_close_socket();
    }

    if (conn_async_) {
        conn_async_->shutdown_and_close_socket();
    }
}

bool RpcClientImpl::send_rpc_message(const RpcRequestMessage& rpc_request_message) {
    Message net_msg(rpc_request_message.net_str());
    return conn_sync_->send_net_message(net_msg);
}

bool RpcClientImpl::recv_rpc_message(Message& net_message) {
    return conn_sync_->recv_net_message(net_message);
}

void RpcClientImpl::set_rpc_call_timeout(uint32_t sec, bool sync) {

    if (sec == 0) {
        return;
    }

    boost::system::error_code ignore_ec;
    if (rpc_call_timer_) {
        rpc_call_timer_->cancel(ignore_ec);
    } else {
        rpc_call_timer_.reset(new steady_timer(io_service_->get_io_service()));
    }

    SAFE_ASSERT( sec > 0 );
    was_timeout_ = false;
    rpc_call_timer_->expires_from_now(seconds(sec));
    rpc_call_timer_->async_wait(std::bind(&RpcClientImpl::rpc_call_timeout, shared_from_this(),
                                           std::placeholders::_1, sync));
}

void RpcClientImpl::rpc_call_timeout(const boost::system::error_code& ec, bool sync) {

    if (ec == 0){
        log_info("rpc_call_timeout called, call start at %lu", time_start_);
        was_timeout_ = true;
        if (sync) {
            conn_sync_->shutdown_and_close_socket();
        } else {
            conn_async_->shutdown_and_close_socket();
        }
    } else if ( ec == boost::asio::error::operation_aborted) {
        // normal cancel, request handled in-time
    } else {
        log_err("unknown and won't handle error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
}

RpcClientStatus RpcClientImpl::call_RPC(uint16_t service_id, uint16_t opcode,
                                        const std::string& payload, std::string& respload,
                                        uint32_t timeout_sec) {

    std::lock_guard<std::mutex> lock(call_mutex_);

    if (!conn_sync_) {

        boost::system::error_code ec;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr
                = std::make_shared<boost::asio::ip::tcp::socket>(io_service_->get_io_service());

        socket_ptr->connect(boost::asio::ip::tcp::endpoint(
                                boost::asio::ip::address::from_string(client_setting_.serv_addr_), client_setting_.serv_port_), ec);
        if (ec) {
            log_err("connect to %s:%u failed with {%d} %s.",
                    client_setting_.serv_addr_.c_str(), client_setting_.serv_port_,
                    ec.value(), ec.message().c_str() );
            return RpcClientStatus::NETWORK_CONNECT_ERROR;

        }

        conn_sync_.reset(new TcpConnSync(socket_ptr, io_service_->get_io_service(), client_setting_));
        if (!conn_sync_) {
            log_err("create socket %s:%u failed.",
                    client_setting_.serv_addr_.c_str(), client_setting_.serv_port_);
            return RpcClientStatus::NETWORK_BEFORE_ERROR;
        }
    }

    time_start_ = ::time(NULL);

    // 构建请求包
    RpcRequestMessage rpc_request_message(service_id, opcode, payload);

    if (timeout_sec > 0) {
        set_rpc_call_timeout(timeout_sec, true);
    }

    // 发送请求报文
    if(!send_rpc_message(rpc_request_message)){
        conn_sync_.reset();
        if (was_timeout_) {
            log_err("rpc_call was timeout with %d sec, start before %lu",
                    timeout_sec, (::time(NULL) - time_start_));
            return RpcClientStatus::RPC_CALL_TIMEOUT;
        }
        return RpcClientStatus::NETWORK_SEND_ERROR;
    }

    // 接收报文
    Message net_message;
    if(!recv_rpc_message(net_message)){
        conn_sync_.reset();
        if (was_timeout_) {
            log_err("rpc_call was timeout with %d sec, start before %lu",
                    timeout_sec, (::time(NULL) - time_start_));
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


bool RpcClientImpl::send_rpc_message_async(const RpcRequestMessage& rpc_request_message) {
    Message net_msg(rpc_request_message.net_str());
    return conn_async_->send_net_message(net_msg);
}


void RpcClientImpl::async_recv_wrapper(const tzrpc::Message& net_message) {

    RpcClientStatus status = RpcClientStatus::OK;
    uint16_t service_id = std::numeric_limits<uint16_t>::max();
    uint16_t opcode = std::numeric_limits<uint16_t>::max();
    std::string respload {};

    do {
       // 解析报文
        RpcResponseMessage rpc_response_message;
        if (!RpcResponseMessageParse(net_message.payload_, rpc_response_message)) {
            status = RpcClientStatus::RECV_FORMAT_ERROR;
            break;
        }

        // 返回参数校验
        if (rpc_response_message.header_.magic != kRpcHeaderMagic //||
             // rpc_response_message.header_.version != kRpcHeaderVersion ||
             // rpc_response_message.header_.service_id != service_id ||
             // rpc_response_message.header_.opcode != opcode
        ) {
            log_err("rpc_response_message header check error: %s", rpc_response_message.header_.dump().c_str());
            status = RpcClientStatus::RECV_FORMAT_ERROR;
            break;
        }

        // Service Status 校验
        if (rpc_response_message.header_.status != RpcResponseStatus::OK) {
            log_err("ServiceSide status: %u", rpc_response_message.header_.status);
            status = static_cast<RpcClientStatus>(static_cast<uint8_t>(rpc_response_message.header_.status));
            break;
        }

        service_id = static_cast<uint16_t>(rpc_response_message.header_.service_id);
        opcode = static_cast<uint16_t>(rpc_response_message.header_.opcode);
        respload = rpc_response_message.payload_;

    } while (0);

    // call
    if (handler_) {
        handler_(status, service_id, opcode, respload);
    }
}

RpcClientStatus RpcClientImpl::call_RPC(uint16_t service_id, uint16_t opcode,
                                        const std::string& payload,
                                        uint32_t timeout_sec) {

    std::lock_guard<std::mutex> lock(call_mutex_);

    if (!conn_async_) {

        boost::system::error_code ec;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr
                = std::make_shared<boost::asio::ip::tcp::socket>(io_service_->get_io_service());

        socket_ptr->connect(boost::asio::ip::tcp::endpoint(
                                boost::asio::ip::address::from_string(client_setting_.serv_addr_), client_setting_.serv_port_), ec);
        if (ec) {
            log_err("connect to %s:%u failed with {%d} %s.",
                    client_setting_.serv_addr_.c_str(), client_setting_.serv_port_,
                    ec.value(), ec.message().c_str() );
            return RpcClientStatus::NETWORK_CONNECT_ERROR;

        }

        conn_async_.reset(new TcpConnAsync(socket_ptr, io_service_->get_io_service(), client_setting_,
                                           std::bind(&RpcClientImpl::async_recv_wrapper, shared_from_this(),
                                                     std::placeholders::_1)));
        if (!conn_async_) {
            log_err("create socket %s:%u failed.",
                    client_setting_.serv_addr_.c_str(), client_setting_.serv_port_);
            return RpcClientStatus::NETWORK_BEFORE_ERROR;
        }

        conn_async_->recv_net_message();
    }

    time_start_ = ::time(NULL);

    // 构建请求包
    RpcRequestMessage rpc_request_message(service_id, opcode, payload);

    if (timeout_sec > 0) {
        set_rpc_call_timeout(timeout_sec, false);
    }

    // 发送请求报文
    if(!send_rpc_message_async(rpc_request_message)){
        conn_async_.reset();
        // 异步发送应该很快返回的，理论上不会在这里出现超时
        if (was_timeout_) {
            log_err("rpc_call was timeout with %d sec, start before %lu",
                    timeout_sec, (::time(NULL) - time_start_));
            return RpcClientStatus::RPC_CALL_TIMEOUT;
        }
        return RpcClientStatus::NETWORK_SEND_ERROR;
    }

    // 手动触发异步读取服务端响应
    //conn_async_->recv_net_message();

    return RpcClientStatus::OK;
}


} // end namespace tzrpc_client
