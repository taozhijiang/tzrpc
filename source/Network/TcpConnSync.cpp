#include <thread>
#include <functional>

#include <boost/algorithm/string.hpp>

#include <Scaffold/Setting.h>
#include <Network/TcpConnSync.h>

namespace tzrpc {

TcpConnSync::TcpConnSync(std::shared_ptr<ip::tcp::socket> socket, boost::asio::io_service& io_service):
    NetConn(socket),
    io_service_(io_service),
    was_cancelled_(false),
    ops_cancel_mutex_(),
    ops_cancel_timer_() {

    set_tcp_nodelay(true);
    set_tcp_nonblocking(false);

    set_conn_stat(ConnStat::kWorking);
}

TcpConnSync::~TcpConnSync() {
    log_debug("TcpConnSync SOCKET RELEASED!!!");
}


int TcpConnSync::parse_header() {

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= sizeof(Header));

    if (recv_bound_.buffer_.get_length() < sizeof(Header)) {
        log_err("we expect at least header read: %d, but get %d",
                static_cast<int>(sizeof(Header)), static_cast<int>(recv_bound_.buffer_.get_length()));
        return 1;
    }

    // 解析头部
    std::string head_str;
    recv_bound_.buffer_.retrive(head_str, sizeof(Header));
    ::memcpy(reinterpret_cast<char*>(&recv_bound_.header_), head_str.c_str(), sizeof(Header));

    recv_bound_.header_.from_net_endian();

    if (recv_bound_.header_.magic != kHeaderMagic ||
        recv_bound_.header_.version != kHeaderVersion ||
        recv_bound_.header_.length > setting.max_msg_size_) {
        log_err("message header check error!");
        return -1;
    }

    return 0;
}

int TcpConnSync::parse_msg_body(Message& msg) {

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= recv_bound_.header_.length);

    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        log_err("we expect at least message read: %d, but get %d",
                static_cast<int>(recv_bound_.header_.length), static_cast<int>(recv_bound_.buffer_.get_length()));
        return 1;
    }

    std::string msg_str;
    recv_bound_.buffer_.retrive(msg_str, recv_bound_.header_.length);

    msg.header_ = recv_bound_.header_;
    msg.playload_ = msg_str;

    return 0;
}


bool TcpConnSync::do_read(Message& msg) override {

    log_debug("read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    while (recv_bound_.buffer_.get_length() < sizeof(Header)) {

        set_ops_cancel_timeout();

        uint32_t bytes_read = recv_bound_.buffer_.get_length();
        boost::system::error_code ec;
        size_t bytes_transferred = boost::asio::read(*socket_, buffer(recv_bound_.io_block_.data(), recv_bound_.io_block_.size()),
                                                    boost::asio::transfer_at_least(sizeof(Header) - bytes_read),
                                                    ec );

        revoke_ops_cancel_timeout();

        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        std::string str(recv_bound_.io_block_.begin(), recv_bound_.io_block_.begin() + bytes_transferred);
        recv_bound_.buffer_.append_internal(str);

    }

    if (parse_header() == 0) {
        return do_read_msg(msg);
    }

    log_err("read error found, shutdown connection...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return false;
}

bool TcpConnSync::do_read_msg(Message& msg) {

    log_debug("read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    while (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {

        set_ops_cancel_timeout();
        uint32_t to_read = std::min((uint32_t)(recv_bound_.header_.length - recv_bound_.buffer_.get_length()),
                                    (uint32_t)(recv_bound_.io_block_.size()));

        boost::system::error_code ec;
        size_t bytes_transferred = boost::asio::read(*socket_, buffer(recv_bound_.io_block_.data(), recv_bound_.io_block_.size()),
                                                     boost::asio::transfer_at_least(to_read),
                                                     ec);

        revoke_ops_cancel_timeout();

        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        std::string str(recv_bound_.io_block_.begin(), recv_bound_.io_block_.begin() + bytes_transferred);
        recv_bound_.buffer_.append_internal(str);
    }

    if (parse_msg_body(msg) == 0) {
        return true;
    }

    log_err("read_msg error found, shutdown connection...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return false;
}

bool TcpConnSync::do_write() override {

    log_debug("write ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    if (was_ops_cancelled()) {
        log_err("socket operation canceled");
        return false;
    }

    // 循环发送缓冲区中的数据
    while (send_bound_.buffer_.get_length() > 0) {
        uint32_t to_write = std::min((uint32_t)(send_bound_.buffer_.get_length()),
                                     (uint32_t)(send_bound_.io_block_.size()));

        send_bound_.buffer_.retrive(send_bound_.io_block_, to_write);
        set_ops_cancel_timeout();

        boost::system::error_code ec;
        size_t bytes_transferred = boost::asio::write(*socket_, buffer(send_bound_.io_block_.data(), to_write),
                                                      boost::asio::transfer_at_least(to_write),
                                                      ec);
        revoke_ops_cancel_timeout();

        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        // TODO
        // 传输返回值的检测喝处理

    }

    return true;
}



// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool TcpConnSync::handle_socket_ec(const boost::system::error_code& ec ) {

    boost::system::error_code ignore_ec;
    bool close_socket = false;

    if (ec == boost::asio::error::eof ||
        ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::timed_out ||
        ec == boost::asio::error::bad_descriptor )
    {
        log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
        close_socket = true;
    }
    else if (ec == boost::asio::error::operation_aborted)
    {
        // like itimeout trigger
        log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
    else
    {
        log_err("undetected error %d, %s ...", ec.value(), ec.message().c_str());
        close_socket = true;
    }


    if (close_socket || was_ops_cancelled()) {
        revoke_ops_cancel_timeout();
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    }

    return close_socket;
}


void TcpConnSync::set_ops_cancel_timeout() {

    std::lock_guard<std::mutex> lock(ops_cancel_mutex_);

    if (setting.client_ops_cancel_time_out_  == 0) {
        SAFE_ASSERT(!ops_cancel_timer_);
        return;
    }

    ops_cancel_timer_.reset( new boost::asio::deadline_timer (io_service_,
                                      boost::posix_time::seconds(setting.client_ops_cancel_time_out_)) );
    SAFE_ASSERT(setting.client_ops_cancel_time_out_ );
    ops_cancel_timer_->async_wait(std::bind(&TcpConnSync::ops_cancel_timeout_call, shared_from_this(),
                                            std::placeholders::_1));
    log_debug("register ops_cancel_time_out %d sec", setting.client_ops_cancel_time_out_);
}

void TcpConnSync::revoke_ops_cancel_timeout() {

    std::lock_guard<std::mutex> lock(ops_cancel_mutex_);

    boost::system::error_code ignore_ec;
    if (ops_cancel_timer_) {
        ops_cancel_timer_->cancel(ignore_ec);
        ops_cancel_timer_.reset();
    }
}

void TcpConnSync::ops_cancel_timeout_call(const boost::system::error_code& ec) {

    if (ec == 0){
        log_info("client_ops_cancel_timeout_call called with timeout: %d", setting.client_ops_cancel_time_out_ );
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    } else if ( ec == boost::asio::error::operation_aborted) {
        // normal cancel
    } else {
        log_err("unknown and won't handle error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
}



} // end tzrpc