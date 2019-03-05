/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __NETWORK_TCP_CONN_ASYNC_H__
#define __NETWORK_TCP_CONN_ASYNC_H__


#include <boost/atomic/atomic.hpp>

#include <Network/NetConn.h>
#include <Utils/Log.h>

namespace tzrpc {

class NetServer;

class TcpConnAsync;

typedef std::shared_ptr<TcpConnAsync> TcpConnAsyncPtr;
typedef std::weak_ptr<TcpConnAsync>   TcpConnAsyncWeakPtr;



class TcpConnAsync: public NetConn,
                    public std::enable_shared_from_this<TcpConnAsync> {

public:

    // 当前并发连接数目
    static boost::atomic<int32_t> current_concurrency_;

    /// Construct a connection with the given socket.
    TcpConnAsync(std::shared_ptr<ip::tcp::socket> socket, NetServer& server);
    virtual ~TcpConnAsync();

    // 禁止拷贝
    TcpConnAsync(const TcpConnAsync&) = delete;
    TcpConnAsync& operator=(const TcpConnAsync&) = delete;

    virtual void start();
    void stop();

    int async_send_message(const Message& msg);

private:

    virtual void do_read();
    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void do_read_msg();
    void read_msg_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);

    int parse_header();
    int parse_msg_body(Message& msg);

    virtual void do_write();
    virtual void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred);

    void set_ops_cancel_timeout();
    void revoke_ops_cancel_timeout();
    bool was_ops_cancelled() {
        std::lock_guard<std::mutex> lock(ops_cancel_mutex_);
        return was_cancelled_;
    }

    bool ops_cancel() {
        std::lock_guard<std::mutex> lock(ops_cancel_mutex_);
        sock_cancel();
        set_conn_stat(ConnStat::kError);
        was_cancelled_ = true;
        return was_cancelled_;
    }
    void ops_cancel_timeout_call(const boost::system::error_code& ec);

    // 是否Connection长连接
    bool keep_continue();


private:

    bool was_cancelled_;
    std::mutex ops_cancel_mutex_;
    std::unique_ptr<steady_timer> ops_cancel_timer_;

    // Of course, the handlers may still execute concurrently with other handlers that
    // were not dispatched through an boost::asio::strand, or were dispatched through
    // a different boost::asio::strand object.

    // Where there is a single chain of asynchronous operations associated with a
    // connection (e.g. in a half duplex protocol implementation like HTTP) there
    // is no possibility of concurrent execution of the handlers. This is an implicit strand.

    NetServer& server_;

    // Strand to ensure the connection's handlers are not called concurrently. ???
    std::shared_ptr<io_service::strand> strand_;

private:

    // http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
    bool handle_socket_ec(const boost::system::error_code& ec);


    IOBound recv_bound_;
    IOBound send_bound_;
};


} // end namespace tzrpc

#endif // __NETWORK_TCP_CONN_ASYNC_H__
