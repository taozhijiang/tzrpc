
#include <thread>
#include <functional>

#include <boost/algorithm/string.hpp>

#include <Network/NetServer.h>
#include <Network/TcpConnAsync.h>


namespace tzrpc {

TcpConnAsync::TcpConnAsync(std::shared_ptr<ip::tcp::socket> socket,
                           NetServer& server):
    NetConn(socket),
    was_cancelled_(false),
    ops_cancel_mutex_(),
    ops_cancel_timer_(),
    server_(server),
    strand_(std::make_shared<io_service::strand>(server.io_service_)) {

    set_tcp_nodelay(true);
    set_tcp_nonblocking(true);
}

TcpConnAsync::~TcpConnAsync() {
    log_debug("TcpConnAsync SOCKET RELEASED!!!");
}

void TcpConnAsync::start() override {

    set_conn_stat(ConnStat::kWorking);
    do_read();
}


void TcpConnAsync::stop() {
    set_conn_stat(ConnStat::kPending);
}


int TcpConnAsync::parse_header() {

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
        recv_bound_.header_.message_len > setting.max_msg_size_) {
        log_err("message header check error!");
        return -1;
    }

    return 0;
}


void TcpConnAsync::do_read() override {

    log_debug("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return;
    }

    if (was_ops_cancelled()) {
        log_err("socket operation canceled");
        return;
    }

    uint32_t bytes_read = recv_bound_.buffer_.get_length();
    if (bytes_read < sizeof(Header)) {
        set_ops_cancel_timeout();
        async_read(*socket_, buffer(recv_bound_.io_block_.data(), recv_bound_.io_block_.size()),
                   boost::asio::transfer_at_least(sizeof(Header) - bytes_read),
                                 strand_->wrap(
                                     std::bind(&TcpConnAsync::read_handler,
                                         shared_from_this(),
                                         std::placeholders::_1,
                                         std::placeholders::_2)));
    } else {
        int ret = parse_header();
        if (ret == 0) {
            return do_read_msg();
        } else if(ret > 0) {
            return do_read();
        } else {
            log_err("read error found, shutdown connection...");
            sock_shutdown_and_close(ShutdownType::kBoth);
            return;
        }
    }
}

void TcpConnAsync::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) override {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    SAFE_ASSERT(bytes_transferred > 0);

    std::string str(recv_bound_.io_block_.begin(), recv_bound_.io_block_.begin() + bytes_transferred);
    recv_bound_.buffer_.append(str);

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= sizeof(Header));

    int ret = parse_header();
    if (ret == 0) {
        return do_read_msg();
    } else if(ret > 0) {
        return do_read();
    } else {
        log_err("read_handler error found, shutdown connection...");
        sock_shutdown_and_close(ShutdownType::kBoth);
        return;
    }
}

int TcpConnAsync::parse_msg_body(Message& msg) {

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= recv_bound_.header_.message_len);
    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.message_len) {
        log_err("we expect at least message read: %d, but get %d",
                static_cast<int>(recv_bound_.header_.message_len), static_cast<int>(recv_bound_.buffer_.get_length()));
        return 1;
    }

    std::string msg_str;
    recv_bound_.buffer_.retrive(msg_str, recv_bound_.header_.message_len);

    msg.header_ = recv_bound_.header_;
    msg.playload_ = msg_str;

    return 0;
}

void TcpConnAsync::do_read_msg() {

    log_debug("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return;
    }

    if (was_ops_cancelled()) {
        log_err("socket operation canceled");
        return;
    }

    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.message_len) {
        uint32_t to_read = std::min((uint32_t)(recv_bound_.header_.message_len - recv_bound_.buffer_.get_length()),
                                    (uint32_t)(recv_bound_.io_block_.size()));
        set_ops_cancel_timeout();
        async_read(*socket_, buffer(recv_bound_.io_block_.data(), recv_bound_.io_block_.size()),
                       boost::asio::transfer_at_least(to_read),
                                     strand_->wrap(
                                         std::bind(&TcpConnAsync::read_msg_handler,
                                             shared_from_this(),
                                             std::placeholders::_1,
                                             std::placeholders::_2)));
    } else {
        Message msg;
        int ret = parse_msg_body(msg);
        if (ret == 0) {
            log_debug("good, read msg finished...");
            log_debug("recv: %s", msg.dump().c_str());

            // write back msg:
            Message msg("message from server ...\r\n");
            send_bound_.buffer_.append(msg);
            do_write();

            return do_read(); // read again for future
        } else if(ret > 0) {
            return do_read_msg();
        } else {
            log_err("read_msg error found, shutdown connection...");
            sock_shutdown_and_close(ShutdownType::kBoth);
            return;
        }
    }
}


void TcpConnAsync::read_msg_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    SAFE_ASSERT(bytes_transferred > 0);

    std::string str(recv_bound_.io_block_.begin(), recv_bound_.io_block_.begin() + bytes_transferred);
    recv_bound_.buffer_.append(str);

    Message msg;
    int ret = parse_msg_body(msg);
    if (ret == 0) {
        log_debug("good, read msg finished...");
        log_debug("recv: %s", msg.dump().c_str());

        // write back msg:
        Message msg("message from server ...\r\n");
        send_bound_.buffer_.append(msg);
        do_write();

        return do_read();
    } else if(ret > 0) {
        return do_read_msg();
    } else {
        log_err("read_msg_handler error found, shutdown connection...");
        sock_shutdown_and_close(ShutdownType::kBoth);
        return;
    }
}


void TcpConnAsync::do_write() override {

    log_debug("strand write ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return;
    }

    if (was_ops_cancelled()) {
        log_err("socket operation canceled");
        return;
    }

    if (send_bound_.buffer_.get_length() == 0) {
        return;
    }

    uint32_t to_write = std::min((uint32_t)(send_bound_.buffer_.get_length()),
                                 (uint32_t)(send_bound_.io_block_.size()));

    send_bound_.buffer_.retrive(send_bound_.io_block_, to_write);

    set_ops_cancel_timeout();
    async_write(*socket_, buffer(send_bound_.io_block_.data(), to_write),
                    boost::asio::transfer_at_least(to_write),
                              strand_->wrap(
                                 std::bind(&TcpConnAsync::write_handler,
                                     shared_from_this(),
                                     std::placeholders::_1,
                                     std::placeholders::_2)));
    return;
}


void TcpConnAsync::write_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    // transfer_at_least 应该可以将需要的数据传输完，除非错误发生了
    SAFE_ASSERT(bytes_transferred > 0);

    // 再次触发写，如果为空就直接返回
    do_write();
}


// socket helper function here...

// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool TcpConnAsync::handle_socket_ec(const boost::system::error_code& ec ) {

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
        log_err("Undetected error %d, %s ...", ec.value(), ec.message().c_str());
        close_socket = true;
    }


    if (close_socket || was_ops_cancelled()) {
        revoke_ops_cancel_timeout();
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    }

    return close_socket;
}

// while:; do echo -e "GET / HTTP/1.1\nhost: test.domain\n\n"; sleep 3; done | telnet 127.0.0.1 8899
bool TcpConnAsync::keep_continue() {
    return false;
}

void TcpConnAsync::set_ops_cancel_timeout() {

    std::lock_guard<std::mutex> lock(ops_cancel_mutex_);

    if (setting.ops_cancel_time_out_  == 0) {
        SAFE_ASSERT(!ops_cancel_timer_);
        return;
    }

    ops_cancel_timer_.reset( new boost::asio::deadline_timer (server_.io_service_,
                                      boost::posix_time::seconds(setting.ops_cancel_time_out_ )) );
    SAFE_ASSERT(setting.ops_cancel_time_out_ );
    ops_cancel_timer_->async_wait(std::bind(&TcpConnAsync::ops_cancel_timeout_call, shared_from_this(),
                                           std::placeholders::_1));
    log_debug("register ops_cancel_time_out %d sec", setting.ops_cancel_time_out_);
}

void TcpConnAsync::revoke_ops_cancel_timeout() {

    std::lock_guard<std::mutex> lock(ops_cancel_mutex_);

    boost::system::error_code ignore_ec;
    if (ops_cancel_timer_) {
        ops_cancel_timer_->cancel(ignore_ec);
        ops_cancel_timer_.reset();
    }
}

void TcpConnAsync::ops_cancel_timeout_call(const boost::system::error_code& ec) {

    if (ec == 0){
        log_info("ops_cancel_timeout_call called with timeout: %d", setting.ops_cancel_time_out_ );
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    } else if ( ec == boost::asio::error::operation_aborted) {
        // normal cancel
    } else {
        log_err("unknown and won't handle error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
}



} // end tzrpc
