/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __NETWORK_TCP_CONN_SYNC_H__
#define __NETWORK_TCP_CONN_SYNC_H__

// 同步的TCP连接，主要用于客户端使用

#include <Network/NetConn.h>

namespace tzrpc_client {

using tzrpc::Message;
using tzrpc::NetConn;
using tzrpc::IOBound;
using tzrpc::ConnStat;
class RpcClientSetting;

class TcpConnSync: public NetConn,
                   public std::enable_shared_from_this<TcpConnSync> {

    friend class RpcClientImpl;

public:

    /// Construct a connection with the given socket.
    explicit TcpConnSync(std::shared_ptr<ip::tcp::socket> socket,
                         boost::asio::io_service& io_service,
                         RpcClientSetting& client_setting);
    virtual ~TcpConnSync();

    // 禁止拷贝
    TcpConnSync(const TcpConnSync&) = delete;
    TcpConnSync& operator=(const TcpConnSync&) = delete;

    bool recv_net_message(Message& msg) {
        return do_read(msg);
    }

    bool send_net_message(const Message& msg) {
        if (client_setting_.send_max_msg_size_ != 0 &&
            msg.header_.length > client_setting_.send_max_msg_size_)
        {
            log_err("send_max_msg_size %d, but we recv %d",
            static_cast<int>(client_setting_.send_max_msg_size_), static_cast<int>(msg.header_.length));
            return false;
        }
        send_bound_.buffer_.append(msg);
        return do_write();
    }

    // between shutdown and close on a socket is the behavior when the socket is shared by other processes.
    // A shutdown() affects all copies of the socket while close() affects only the file descriptor in one process.
    void shutdown_and_close_socket() {
        sock_shutdown_and_close(tzrpc::ShutdownType::kBoth);
    }

private:

    virtual bool do_read(Message& msg);
    bool do_read_msg(Message& msg);
    virtual bool do_write();

    int parse_header();
    int parse_msg_body(Message& msg);

private:

    RpcClientSetting& client_setting_;
    boost::asio::io_service& io_service_;

    // http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
    bool handle_socket_ec(const boost::system::error_code& ec);

private:

    IOBound recv_bound_;
    IOBound send_bound_;

};


} // end namespace tzrpc_client


#endif // __NETWORK_TCP_CONN_SYNC_H__
