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

#include <other/Log.h>
using roo::log_api;

#include <Client/include/RpcClient.h>
#include <Client/TcpConnAsync.h>


namespace tzrpc_client {

using tzrpc::Header;
using tzrpc::kHeaderMagic;
using tzrpc::kHeaderVersion;
using tzrpc::kFixedIoBufferSize;
using tzrpc::ShutdownType;

TcpConnAsync::TcpConnAsync(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                           boost::asio::io_service& io_service,
                           RpcClientSetting& client_setting,
                           const rpc_wrapper_t& handler) :
    NetConn(socket),
    client_setting_(client_setting),
    wrapper_handler_(handler),
    io_service_(io_service),
    strand_(std::make_shared<boost::asio::io_service::strand>(io_service)) {

    set_tcp_nodelay(true);
    set_tcp_nonblocking(true);

    set_conn_stat(ConnStat::kWorking);
}

TcpConnAsync::~TcpConnAsync() {
    log_debug("TcpConnAsync SOCKET RELEASED!!!");
}


bool TcpConnAsync::do_write() {

    log_debug("strand write ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    if (send_bound_.buffer_.get_length() == 0) {
        return true;
    }

    uint32_t to_write = std::min((uint32_t)(send_bound_.buffer_.get_length()),
                                 (uint32_t)(kFixedIoBufferSize));

    send_bound_.buffer_.consume(send_bound_.io_block_, to_write);

    async_write(*socket_, boost::asio::buffer(send_bound_.io_block_, to_write),
                boost::asio::transfer_exactly(to_write),
                strand_->wrap(
                    std::bind(&TcpConnAsync::write_handler,
                              shared_from_this(),
                              std::placeholders::_1,
                              std::placeholders::_2)));
    return true;
}


void TcpConnAsync::write_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    // transfer_at_least 应该可以保证将需要的数据传输完，除非错误发生了
    SAFE_ASSERT(bytes_transferred > 0);

    // 再次触发写，如果为空就直接返回
    // 函数中会检查，如果内容为空，就直接返回不执行写操作
    do_write();
}


int TcpConnAsync::parse_header() {

    if (recv_bound_.buffer_.get_length() < sizeof(Header)) {
        log_err("we expect at least header read: %d, but get %d",
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
        log_err("async message header check error %x:%x - %x:%x!",
                recv_bound_.header_.magic, recv_bound_.header_.version,
                kHeaderMagic, kHeaderVersion);
        return -1;
    }

    if (client_setting_.recv_max_msg_size_ != 0 &&
        recv_bound_.header_.length > client_setting_.recv_max_msg_size_) {
        log_err("send_max_msg_size %d, but we will recv %d",
                static_cast<int>(client_setting_.recv_max_msg_size_), static_cast<int>(recv_bound_.header_.length));
        return -1;
    }

    return 0;
}


bool TcpConnAsync::do_read() {

    log_debug("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    uint32_t bytes_read = recv_bound_.buffer_.get_length();
    if (bytes_read < sizeof(Header)) {
        async_read(*socket_, boost::asio::buffer(recv_bound_.io_block_, kFixedIoBufferSize),
                   boost::asio::transfer_at_least(sizeof(Header) - bytes_read),
                   strand_->wrap(
                       std::bind(&TcpConnAsync::read_handler,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2)));
    } else {
        int ret = parse_header();
        if (ret == 0) {
            do_read_msg();
            return true;
        } else if (ret > 0) {
            do_read();
            return true;
        } else {
            log_err("read error found, shutdown connection...");
            sock_shutdown_and_close(ShutdownType::kBoth);
            return false;
        }
    }

    return true;
}

void TcpConnAsync::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) {

    if (ec) {
        handle_socket_ec(ec);
        return;
    }

    SAFE_ASSERT(bytes_transferred > 0);

    std::string str(recv_bound_.io_block_, bytes_transferred);
    recv_bound_.buffer_.append_internal(str);

    if (recv_bound_.buffer_.get_length() < sizeof(Header)) {
        log_notice("unexpect read again!");
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
    } else {
        log_err("read_handler error found, shutdown connection...");
        sock_shutdown_and_close(ShutdownType::kBoth);
        return;
    }
}

int TcpConnAsync::parse_msg_body(Message& msg) {

    // need to read again!
    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        log_debug("we expect at least message read: %d, but get %d",
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

    log_debug("strand read ... in thread %#lx", (long)pthread_self());

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return;
    }


    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        uint32_t to_read = std::min((uint32_t)(recv_bound_.header_.length - recv_bound_.buffer_.get_length()),
                                    (uint32_t)(kFixedIoBufferSize));
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
            log_debug("read_message: %s", msg.dump().c_str());
            log_debug("read message finished, dispatch for RPC process.");

            if (wrapper_handler_) {
                wrapper_handler_(msg);
            }

            do_read(); // read again for future
            return;

        } else if (ret > 0) {

            do_read_msg();
            return;

        } else {

            log_err("read_msg error found, shutdown connection...");
            sock_shutdown_and_close(ShutdownType::kBoth);
            return;

        }
    }
}


void TcpConnAsync::read_msg_handler(const boost::system::error_code& ec, size_t bytes_transferred) {

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
        log_debug("read_message: %s", msg.dump().c_str());
        log_debug("read message finished, dispatch for RPC process.");

        if (wrapper_handler_) {
            wrapper_handler_(msg);
        }

        do_read();
        return;

    } else if (ret > 0) {

        return do_read_msg();

    } else {

        log_err("read_msg_handler error found, shutdown connection...");
        sock_shutdown_and_close(ShutdownType::kBoth);
        return;

    }
}


// socket helper function here...

// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool TcpConnAsync::handle_socket_ec(const boost::system::error_code& ec) {

    boost::system::error_code ignore_ec;
    bool close_socket = false;

    if (ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::timed_out ||
        ec == boost::asio::error::bad_descriptor) {
        log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
        close_socket = true;
    } else if (ec == boost::asio::error::eof) {
        // 正常的，数据传输完毕
        close_socket = true;
    } else if (ec == boost::asio::error::operation_aborted) {
        // like itimeout trigger
        log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
    } else {
        log_err("Undetected error %d, %s ...", ec.value(), ec.message().c_str());
        close_socket = true;
    }


    if (close_socket) {
        sock_shutdown_and_close(ShutdownType::kBoth);
    }

    return close_socket;
}

} // end namespace tzrpc_client
