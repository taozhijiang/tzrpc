/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __CLIENT_TCP_CONN_ASYNC_H__
#define __CLIENT_TCP_CONN_ASYNC_H__

#include <xtra_rhel.h>

#include <Network/NetConn.h>
#include <other/Log.h>

namespace tzrpc_client {

using tzrpc::Message;
using tzrpc::NetConn;
using tzrpc::IOBound;
using tzrpc::ConnStat;

class RpcClientSetting;

typedef std::function<void(const tzrpc::Message& net_message)> rpc_wrapper_t;

class TcpConnAsync : public NetConn,
    public std::enable_shared_from_this<TcpConnAsync> {

    friend class RpcClientImpl;

public:

    /// Construct a connection with the given socket.
    TcpConnAsync(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                 boost::asio::io_service& io_service,
                 RpcClientSetting& client_setting,
                 const rpc_wrapper_t& handler);

    virtual ~TcpConnAsync();

    // 禁止拷贝
    TcpConnAsync(const TcpConnAsync&) = delete;
    TcpConnAsync& operator=(const TcpConnAsync&) = delete;

    bool send_net_message(const Message& msg) {
        if (client_setting_.send_max_msg_size_ != 0 &&
            msg.header_.length > client_setting_.send_max_msg_size_) {
            log_err("send_max_msg_size %d, but we recv %d",
                    static_cast<int>(client_setting_.send_max_msg_size_), static_cast<int>(msg.header_.length));
            return false;
        }
        send_bound_.buffer_.append(msg);
        return do_write();
    }

    bool recv_net_message() {
        return do_read();
    }


    // between shutdown and close on a socket is the behavior when the socket is shared by other processes.
    // A shutdown() affects all copies of the socket while close() affects only the file descriptor in one process.
    void shutdown_and_close_socket() {
        sock_shutdown_and_close(tzrpc::ShutdownType::kBoth);
    }

private:

    virtual bool do_write()override;
    virtual void write_handler(const boost::system::error_code& ec, std::size_t bytes_transferred)override;

    virtual bool do_read()override;
    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred)override;

    void do_read_msg();
    void read_msg_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);

    int parse_header();
    int parse_msg_body(Message& msg);

private:

    RpcClientSetting& client_setting_;

    // 主要用来进行Rpc拆包，然后再调用内部嵌套的业务层回调函数
    rpc_wrapper_t wrapper_handler_;


    boost::asio::io_service& io_service_;

    // Strand to ensure the connection's handlers are not called concurrently. ???
    std::shared_ptr<boost::asio::io_service::strand> strand_;

    // http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
    bool handle_socket_ec(const boost::system::error_code& ec);


    IOBound recv_bound_;
    IOBound send_bound_;
};


} // end namespace tzrpc_client

#endif // __CLIENT_TCP_CONN_ASYNC_H__
