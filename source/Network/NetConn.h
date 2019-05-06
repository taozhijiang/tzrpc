/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __NETWORK_NET_CONN_H__
#define __NETWORK_NET_CONN_H__

#include <Core/Buffer.h>
#include <Core/Message.h>

#include <boost/asio.hpp>

#include <mutex>

namespace tzrpc {

enum class ConnStat : uint8_t {
    kWorking = 1,
    kPending,
    kError,
    kClosed,
};

enum class ShutdownType : uint8_t {
    kSend = 1,
    kRecv = 2,
    kBoth = 3,
};

class NetConn {

public:

    /// Construct a connection with the given socket.
    explicit NetConn(std::shared_ptr<boost::asio::ip::tcp::socket> sock) :
        conn_stat_(ConnStat::kPending),
        socket_(sock) {
        // 默认是阻塞类型的socket，异步调用的时候自行设置
        set_tcp_nonblocking(false);
    }

    virtual ~NetConn() { }

public:

    virtual bool do_read() = 0;
    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) = 0;

    virtual bool do_write() = 0;
    virtual void write_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) = 0;


    // some general tiny function
    // some general tiny settings function

    bool set_tcp_nonblocking(bool set_value) {

        boost::system::error_code ignore_ec;

        boost::asio::socket_base::non_blocking_io command(set_value);
        socket_->io_control(command, ignore_ec);

        return true;
    }

    bool set_tcp_nodelay(bool set_value) {

        boost::system::error_code ignore_ec;

        boost::asio::ip::tcp::no_delay nodelay(set_value);
        socket_->set_option(nodelay, ignore_ec);
        boost::asio::ip::tcp::no_delay option;
        socket_->get_option(option, ignore_ec);

        return (option.value() == set_value);
    }

    bool set_tcp_keepalive(bool set_value) {

        boost::system::error_code ignore_ec;

        boost::asio::socket_base::keep_alive keepalive(set_value);
        socket_->set_option(keepalive, ignore_ec);
        boost::asio::socket_base::keep_alive option;
        socket_->get_option(option, ignore_ec);

        return (option.value() == set_value);
    }

    void sock_shutdown_and_close(enum ShutdownType s) {

        std::lock_guard<std::mutex> lock(conn_mutex_);

        if (conn_stat_ == ConnStat::kClosed)
            return;

        boost::system::error_code ignore_ec;
        if (s == ShutdownType::kSend) {
            socket_->shutdown(boost::asio::socket_base::shutdown_send, ignore_ec);
        } else if (s == ShutdownType::kRecv) {
            socket_->shutdown(boost::asio::socket_base::shutdown_receive, ignore_ec);
        } else if (s == ShutdownType::kBoth) {
            socket_->shutdown(boost::asio::socket_base::shutdown_both, ignore_ec);
        }

        socket_->close(ignore_ec);

        conn_stat_ = ConnStat::kClosed;
    }

    void sock_cancel() {

        std::lock_guard<std::mutex> lock(conn_mutex_);

        boost::system::error_code ignore_ec;
        socket_->cancel(ignore_ec);
    }

    void sock_close() {

        std::lock_guard<std::mutex> lock(conn_mutex_);

        if (conn_stat_ == ConnStat::kClosed)
            return;

        boost::system::error_code ignore_ec;
        socket_->close(ignore_ec);
        conn_stat_ = ConnStat::kClosed;
    }

    enum ConnStat get_conn_stat() { return conn_stat_; }
    void set_conn_stat(enum ConnStat stat) { conn_stat_ = stat; }

private:
    std::mutex conn_mutex_;
    enum ConnStat conn_stat_;

protected:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
};



const static uint32_t kFixedIoBufferSize = 2048;

struct IOBound {
    IOBound() :
        io_block_({ }),
        header_({ }),
        buffer_() {
    }

    char io_block_[kFixedIoBufferSize];    // 读写操作的固定缓存
    Header header_;                 // 如果 > sizeof(Header), head转换成host order
    Buffer buffer_;                 // 已经传输字节
};


} // end namespace tzrpc

#endif // __NETWORK_NET_CONN_H__
