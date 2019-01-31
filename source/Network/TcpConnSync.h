#ifndef __NETWORK_TCP_CONN_SYNC_H__
#define __NETWORK_TCP_CONN_SYNC_H__

// 同步的TCP连接，主要用于客户端使用

#include <Network/NetConn.h>
#include <Utils/Log.h>

namespace tzrpc {

class TcpConnSync: public NetConn, public boost::noncopyable,
                   public std::enable_shared_from_this<TcpConnSync> {

public:

    /// Construct a connection with the given socket.
    explicit TcpConnSync(std::shared_ptr<ip::tcp::socket> socket, boost::asio::io_service& io_service);
    virtual ~TcpConnSync();

    bool recv_net_message(Message& msg) {
        return do_read(msg);
    }

    bool send_net_message(const Message& msg) {
        send_bound_.buffer_.append(msg);
        return do_write();
    }

    void shutdown_socket() {
        ops_cancel();
        sock_close();
    }

private:

    virtual bool do_read(Message& msg);
    bool do_read_msg(Message& msg);
    virtual bool do_write();

    int parse_header();
    int parse_msg_body(Message& msg);

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

private:

    boost::asio::io_service& io_service_;

    bool was_cancelled_;
    std::mutex ops_cancel_mutex_;
    std::unique_ptr<boost::asio::deadline_timer> ops_cancel_timer_;

    // http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
    bool handle_socket_ec(const boost::system::error_code& ec);

private:

    IOBound recv_bound_;
    IOBound send_bound_;

};


} // end tzrpc


#endif // __NETWORK_TCP_CONN_SYNC_H__
