/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <thread>
#include <functional>

#include <boost/algorithm/string.hpp>

#include <Client/LogClient.h>

#include <Client/include/RpcClient.h>
#include <Client/TcpConnSync.h>

namespace tzrpc_client {

using tzrpc::Header;
using tzrpc::kHeaderMagic;
using tzrpc::kHeaderVersion;
using tzrpc::kFixedIoBufferSize;
using tzrpc::ShutdownType;

TcpConnSync::TcpConnSync(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                         boost::asio::io_service& io_service,
                         RpcClientSetting& client_setting):
    NetConn(socket),
    io_service_(io_service),
    client_setting_(client_setting) {

    set_tcp_nodelay(true);
    set_tcp_nonblocking(false);

    set_conn_stat(ConnStat::kWorking);
}

TcpConnSync::~TcpConnSync() {
}


int TcpConnSync::parse_header() {

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
        log_err("message header check error!");
        log_err("dump recv_bound.header_: %s", recv_bound_.header_.dump().c_str());
        return -1;
    }

    if (client_setting_.recv_max_msg_size_ != 0 &&
        recv_bound_.header_.length > client_setting_.recv_max_msg_size_) {
        log_err("recv_max_msg_size %d, but we recv %d",
                static_cast<int>(client_setting_.recv_max_msg_size_), static_cast<int>(recv_bound_.header_.length));
        return -1;
    }

    return 0;
}

int TcpConnSync::parse_msg_body(Message& msg) {

    // need to read again!
    if (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {
        log_err("we expect at least message read: %d, but get %d",
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


bool TcpConnSync::do_read(Message& msg) {

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    while (recv_bound_.buffer_.get_length() < sizeof(Header)) {

        uint32_t bytes_read = recv_bound_.buffer_.get_length();
        boost::system::error_code ec;
        size_t bytes_transferred
                = boost::asio::read(*socket_, boost::asio::buffer(recv_bound_.io_block_, kFixedIoBufferSize),
                                    boost::asio::transfer_at_least(sizeof(Header) - bytes_read),
                                    ec );

        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        std::string str(recv_bound_.io_block_, bytes_transferred);
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

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    while (recv_bound_.buffer_.get_length() < recv_bound_.header_.length) {

        uint32_t to_read = std::min((uint32_t)(recv_bound_.header_.length - recv_bound_.buffer_.get_length()),
                                    (uint32_t)(kFixedIoBufferSize));

        boost::system::error_code ec;
        size_t bytes_transferred
                = boost::asio::read(*socket_, boost::asio::buffer(recv_bound_.io_block_, kFixedIoBufferSize),
                                    boost::asio::transfer_at_least(to_read),
                                    ec);

        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        std::string str(recv_bound_.io_block_, bytes_transferred);
        recv_bound_.buffer_.append_internal(str);
    }

    if (parse_msg_body(msg) == 0) {
        return true;
    }

    log_err("read_msg error found, shutdown connection...");
    sock_shutdown_and_close(ShutdownType::kBoth);
    return false;
}

bool TcpConnSync::do_write() {

    if (get_conn_stat() != ConnStat::kWorking) {
        log_err("socket status error: %d", get_conn_stat());
        return false;
    }

    // 循环发送缓冲区中的数据
    while (send_bound_.buffer_.get_length() > 0) {

        uint32_t to_write = std::min((uint32_t)(send_bound_.buffer_.get_length()),
                                     (uint32_t)(kFixedIoBufferSize));

        send_bound_.buffer_.consume(send_bound_.io_block_, to_write);

        boost::system::error_code ec;
        size_t bytes_transferred
                = boost::asio::write(*socket_, boost::asio::buffer(send_bound_.io_block_, to_write),
                                     boost::asio::transfer_exactly(to_write),
                                     ec);
        if (ec) {
            handle_socket_ec(ec);
            return false;
        }

        // 传输返回值的检测和处理
        SAFE_ASSERT(bytes_transferred == to_write );
    }

    return true;
}



// http://www.boost.org/doc/libs/1_44_0/doc/html/boost_asio/reference/error__basic_errors.html
bool TcpConnSync::handle_socket_ec(const boost::system::error_code& ec ) {

    boost::system::error_code ignore_ec;
    bool close_socket = false;

    if (ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::timed_out ||
        ec == boost::asio::error::bad_descriptor )
    {
        log_err("error_code: {%d} %s", ec.value(), ec.message().c_str());
        close_socket = true;
    }
    else if (ec == boost::asio::error::eof)
    {
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


    if (close_socket) {
        sock_shutdown_and_close(ShutdownType::kBoth);
    }

    return close_socket;
}

} // end namespace tzrpc_client
