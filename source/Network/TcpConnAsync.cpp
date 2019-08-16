/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <xtra_rhel.h>

#include <thread>
#include <functional>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#include <Network/NetServer.h>
#include <Network/TcpConnAsync.h>

#include <RPC/RpcInstance.h>
#include <RPC/Dispatcher.h>

namespace tzrpc {

boost::atomic<int32_t> TcpConnAsync::current_concurrency_(0);

TcpConnAsync::TcpConnAsync(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                           NetServer& server) :
    NetConn(socket),
    was_cancelled_(false),
    ops_cancel_mutex_(),
    ops_cancel_timer_(),
    server_(server),
    strand_(std::make_shared<boost::asio::io_service::strand>(server.io_service_)),
    bound_mutex_(),
    send_status_(SendStatus::kDone) {

    set_tcp_nodelay(true);
    set_tcp_nonblocking(true);

    ++current_concurrency_;
}

TcpConnAsync::~TcpConnAsync() {

    --current_concurrency_;
    roo::log_info("TcpConnAsync SOCKET RELEASED!!!");
}

void TcpConnAsync::start() {

    set_conn_stat(ConnStat::kWorking);
    do_read();
}


void TcpConnAsync::stop() {

    set_conn_stat(ConnStat::kPending);
}


int TcpConnAsync::parse_header() {

    if (recv_bound_.buffer_.get_length() < sizeof(Header)) {
        roo::log_err("Expect recv at least head length: %d, but only get %d.",
                     static_cast<int>(sizeof(Header)), static_cast<int>(recv_bound_.buffer_.get_length()));
        return 1;
    }

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= sizeof(Header));

    // 解析头部
    std::string head_str;
    recv_bound_.buffer_.consume(head_str, sizeof(Header));
    ::memcpy(reinterpret_cast<char*>(&recv_bound_.header_), head_str.c_str(), sizeof(Header));

    recv_bound_.header_.from_net_endian();

    if (recv_bound_.header_.magic != kHeaderMagic ||
        recv_bound_.header_.version != kHeaderVersion) {
        roo::log_err("async message head check failed.");
        roo::log_err("dump recv_bound.header_: %s]", recv_bound_.header_.dump().c_str());
        return -1;
    }

    if (server_.recv_max_msg_size() != 0 &&
        recv_bound_.header_.length > static_cast<uint32_t>(server_.recv_max_msg_size())) {
        roo::log_err("Limit recv_max_msg_size length to %d, but need to recv content length %d.",
                     static_cast<int>(server_.recv_max_msg_size()), static_cast<int>(recv_bound_.header_.length));
        return -1;
    }

    return 0;
}


bool TcpConnAsync::do_read() {

//    roo::log_info("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        roo::log_err("Check socket status error, current status %d.", get_conn_stat());
        return false;
    }

    if (was_ops_cancelled()) {
        roo::log_err("Socket operation was already cancelled.");
        return false;
    }

    uint32_t bytes_read = recv_bound_.buffer_.get_length();
    if (bytes_read < sizeof(Header)) {
        set_ops_cancel_timeout();
        async_read(*socket_, boost::asio::buffer(recv_bound_.io_block_, kFixedIoBufferSize),
                   boost::asio::transfer_at_least(sizeof(Header) - bytes_read),
                   strand_->wrap(
                       std::bind(&TcpConnAsync::read_handler,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2)));
        return true;
    }


    int ret = parse_header();
    if (ret == 0) {
        do_read_msg();
        return true;
    } else if (ret > 0) {
        do_read();
        return true;
    }

    roo::log_err("Recv message head error found, shutdown connection ...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return false;
}

void TcpConnAsync::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    SAFE_ASSERT(bytes_transferred > 0);

    std::string str(recv_bound_.io_block_, bytes_transferred);
    recv_bound_.buffer_.append_internal(str);

    if (recv_bound_.buffer_.get_length() < sizeof(Header)) {
        roo::log_err("Expect recv at least head length: %d, but only get %d. do_read again...",
                     static_cast<int>(sizeof(Header)), static_cast<int>(recv_bound_.buffer_.get_length()));
        do_read();
        return;
    }

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= sizeof(Header));

    int ret = parse_header();
    if (ret == 0) {
        return do_read_msg();
    } else if (ret > 0) {
        do_read();
        return;
    }

    roo::log_err("Recv message head error found, shutdown connection ...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return;
}

int TcpConnAsync::parse_msg_body(Message& msg) {

    // need to read again!
    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        roo::log_info("Expect recv at least body length: %d, but only get %d. do_read again...",
                      static_cast<int>(recv_bound_.header_.length), static_cast<int>(recv_bound_.buffer_.get_length()));
        return 1;
    }

    SAFE_ASSERT(recv_bound_.buffer_.get_length() >= recv_bound_.header_.length);

    std::string msg_str;
    recv_bound_.buffer_.consume(msg_str, recv_bound_.header_.length);

    msg.header_ = recv_bound_.header_;
    msg.payload_ = msg_str;

    return 0;
}

void TcpConnAsync::do_read_msg() {

    // roo::log_info("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        roo::log_err("Check socket status error, current status %d.", get_conn_stat());
        return;
    }

    if (was_ops_cancelled()) {
        roo::log_err("Socket operation was already cancelled.");
        return;
    }

    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        uint32_t to_read = std::min((uint32_t)(recv_bound_.header_.length - recv_bound_.buffer_.get_length()),
                                    (uint32_t)(kFixedIoBufferSize));
        set_ops_cancel_timeout();
        async_read(*socket_, boost::asio::buffer(recv_bound_.io_block_, kFixedIoBufferSize),
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

            // 转发到RPC请求
            roo::log_info("read_message: %s", msg.dump().c_str());
            roo::log_info("read message finished, dispatch for RPC process.");
            auto instance = std::make_shared<RpcInstance>(msg.payload_, shared_from_this(), msg.payload_.size());
            Dispatcher::instance().handle_RPC(instance);

            do_read(); // read again for future
            return;

        } else if (ret > 0) {

            do_read_msg();
            return;

        }

        roo::log_err("Recv message body error found, shutdown connection ...");
        sock_shutdown_and_close(ShutdownType::kBoth);
        return;
    }
}


void TcpConnAsync::read_msg_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    SAFE_ASSERT(bytes_transferred > 0);

    std::string str(recv_bound_.io_block_, bytes_transferred);
    recv_bound_.buffer_.append_internal(str);

    Message msg;
    int ret = parse_msg_body(msg);
    if (ret == 0) {

        // 转发到RPC请求
        roo::log_info("read_message: %s", msg.dump().c_str());
        roo::log_info("read message finished, dispatch for RPC process.");
        auto instance = std::make_shared<RpcInstance>(msg.payload_, shared_from_this(), msg.payload_.size());
        Dispatcher::instance().handle_RPC(instance);

        do_read();
        return;

    } else if (ret > 0) {

        return do_read_msg();

    }

    roo::log_err("read_msg_handler error found, shutdown connection...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return;
}

int TcpConnAsync::async_send_message(const Message& msg) {

    if (server_.send_max_msg_size() != 0 &&
        msg.header_.length > static_cast<uint32_t>(server_.send_max_msg_size())) {
        roo::log_err("Limit send_max_msg_size length to %d, but need to send content length %d.",
                     static_cast<int>(server_.send_max_msg_size()), static_cast<int>(msg.header_.length));
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(bound_mutex_);
        send_bound_.buffer_.append(msg);
    }

    do_write();

    return 0;
}


bool TcpConnAsync::do_write() {

    // 如果之前的发送没有完成，则这次放弃主动发送请求。等待本次发送完成的时候，会自动
    // 触发剩余数据的发送操作
    if (send_status_ != SendStatus::kDone) {
        roo::log_info("SendStatus is not finished, give up this active write trigger.");
        return true;
    }

    // roo::log_info("strand write ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        roo::log_err("Check socket status error, current status %d.", get_conn_stat());
        return false;
    }

    if (was_ops_cancelled()) {
        roo::log_err("Socket operation was already cancelled.");
        return false;
    }

    {

        std::lock_guard<std::mutex> lock(bound_mutex_);

        if (send_bound_.buffer_.get_length() == 0) {
            send_status_ = SendStatus::kDone;
            return true;
        }

        uint32_t to_write = std::min((uint32_t)(send_bound_.buffer_.get_length()),
                                     (uint32_t)(kFixedIoBufferSize));

        send_bound_.buffer_.consume(send_bound_.io_block_, to_write);
        send_status_ = SendStatus::kSend;

        set_ops_cancel_timeout();
        async_write(*socket_, boost::asio::buffer(send_bound_.io_block_, to_write),
                    boost::asio::transfer_exactly(to_write),
                    strand_->wrap(
                        std::bind(&TcpConnAsync::write_handler,
                                  shared_from_this(),
                                  std::placeholders::_1,
                                  std::placeholders::_2)));

    }

    return true;
}


void TcpConnAsync::write_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    revoke_ops_cancel_timeout();

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    // transfer_at_least 应该可以保证将需要的数据传输完，除非错误发生了
    SAFE_ASSERT(bytes_transferred > 0);

    send_status_ = SendStatus::kDone;

    // 再次触发写，如果为空就直接返回
    // 函数中会检查，如果内容为空，就直接返回不执行写操作
    do_write();
}


// socket helper function here...

// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool TcpConnAsync::handle_socket_ec(const boost::system::error_code& ec) {

    boost::system::error_code ignore_ec;
    bool close_socket = false;

    if (ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::timed_out ||
        ec == boost::asio::error::bad_descriptor) {
        roo::log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
        close_socket = true;
    } else if (ec == boost::asio::error::eof) {
        // 正常的，数据传输完毕
        close_socket = true;
    } else if (ec == boost::asio::error::operation_aborted) {
        // like itimeout trigger
        roo::log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
    } else {
        roo::log_err("Undetected error %d, %s ...", ec.value(), ec.message().c_str());
        close_socket = true;
    }


    if (close_socket || was_ops_cancelled()) {
        revoke_ops_cancel_timeout();
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    }

    return close_socket;
}

void TcpConnAsync::set_ops_cancel_timeout() {

    std::lock_guard<std::mutex> lock(ops_cancel_mutex_);

    if (server_.ops_cancel_time_out() == 0) {
        SAFE_ASSERT(!ops_cancel_timer_);
        return;
    }

    // cancel the already timer first if any
    boost::system::error_code ignore_ec;
    if (ops_cancel_timer_) {
        ops_cancel_timer_->cancel(ignore_ec);
    } else {
        ops_cancel_timer_.reset(new steady_timer(server_.io_service_));
    }

    SAFE_ASSERT(server_.ops_cancel_time_out());
    ops_cancel_timer_->expires_from_now(seconds(server_.ops_cancel_time_out()));
    ops_cancel_timer_->async_wait(std::bind(&TcpConnAsync::ops_cancel_timeout_call, shared_from_this(),
                                            std::placeholders::_1));
    return;
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

    if (ec == 0) {
        roo::log_warning("ops_cancel_timeout_call called with timeout: %d", server_.ops_cancel_time_out());
        ops_cancel();
        sock_shutdown_and_close(ShutdownType::kBoth);
    } else if (ec == boost::asio::error::operation_aborted) {
        // normal cancel
    } else {
        roo::log_err("Undetected and won't be handled error_code: {%d} %s", ec.value(), ec.message().c_str());
    }
}


} // end namespace tzrpc
