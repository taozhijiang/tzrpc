#ifndef __NETWORK_TCP_CONN_ASYNC_H__
#define __NETWORK_TCP_CONN_ASYNC_H__

#include <Network/NetConn.h>
#include <Utils/Log.h>

#include <Core/Message.h>
#include <Core/Buffer.h>

namespace tzrpc {

class NetServer;

class TcpConnAsync;

typedef std::shared_ptr<TcpConnAsync> TcpConnAsyncPtr;
typedef std::weak_ptr<TcpConnAsync>   TcpConnAsyncWeakPtr;


struct IOBound {
    IOBound():
        io_block_({}),
        header_({}),
        buffer_() {
        io_block_.resize(8*1024);
    }

    std::vector<char> io_block_;    // 读写操作的固定缓存
    Header header_;                 // 如果 > sizeof(Header), head转换成host order
    Buffer buffer_;                 // 已经传输字节
};

class TcpConnAsync: public NetConn, public boost::noncopyable,
                    public std::enable_shared_from_this<TcpConnAsync> {

public:

    /// Construct a connection with the given socket.
    TcpConnAsync(std::shared_ptr<ip::tcp::socket> socket, NetServer& server);
    virtual ~TcpConnAsync();

    virtual void start();
    void stop();

    // http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
    bool handle_socket_ec(const boost::system::error_code& ec);

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
    std::unique_ptr<boost::asio::deadline_timer> ops_cancel_timer_;

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

    IOBound recv_bound_;
    IOBound send_bound_;
};


} // end tzrpc

#endif // __NETWORK_TCP_CONN_ASYNC_H__
