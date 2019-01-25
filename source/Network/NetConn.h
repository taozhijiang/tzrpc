#ifndef __NETWORK_NET_CONN_H__
#define __NETWORK_NET_CONN_H__

#include <xtra_asio.h>

namespace tzrpc {

enum ConnStat {
    kWorking = 1,
    kPending,
    kError,
    kClosed,
};

enum ShutdownType {
    kSend = 1,
    kRecv = 2,
    kBoth = 3,
};

class NetConn {

public:

    /// Construct a connection with the given socket.
    explicit NetConn(std::shared_ptr<ip::tcp::socket> sock):
        conn_stat_(kPending),
        socket_(sock)
    {
        set_tcp_nonblocking(false);
    }

    virtual ~NetConn() {}

public:

    // some general tiny settings function

    bool set_tcp_nonblocking(bool set_value) {
        socket_base::non_blocking_io command(set_value);
        socket_->io_control(command);

        return true;
    }

    bool set_tcp_nodelay(bool set_value) {

        boost::asio::ip::tcp::no_delay nodelay(set_value);
        socket_->set_option(nodelay);
        boost::asio::ip::tcp::no_delay option;
        socket_->get_option(option);

        return (option.value() == set_value);
    }

    bool set_tcp_keepalive(bool set_value) {

        boost::asio::socket_base::keep_alive keepalive(set_value);
        socket_->set_option(keepalive);
        boost::asio::socket_base::keep_alive option;
        socket_->get_option(option);

        return (option.value() == set_value);
    }

    void sock_shutdown_and_close(enum ShutdownType s) {

        std::lock_guard<std::mutex> lock(conn_mutex_);

        if ( conn_stat_ == ConnStat::kClosed )
            return;

        boost::system::error_code ignore_ec;
        if (s == kSend) {
            socket_->shutdown(boost::asio::socket_base::shutdown_send, ignore_ec);
        } else if (s == kRecv) {
            socket_->shutdown(boost::asio::socket_base::shutdown_receive, ignore_ec);
        } else if (s == kBoth) {
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

        if ( conn_stat_ == ConnStat::kClosed )
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
    std::shared_ptr<ip::tcp::socket> socket_;
};


typedef std::shared_ptr<NetConn> NetConnPtr;
typedef std::weak_ptr<NetConn>   NetConnWeakPtr;


} // end tzrpc

#endif // __NETWORK_NET_CONN_H__
