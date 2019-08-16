/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __NETWORK_NET_SERVER_H__
#define __NETWORK_NET_SERVER_H__

#include <xtra_rhel.h>

#include <boost/asio.hpp>

#include <scaffold/Status.h>
#include <scaffold/Setting.h>
#include <concurrency/ThreadPool.h>
#include <other/Log.h>

#include "NetConf.h"

namespace tzrpc {

// forward
class TcpConnAsync;

typedef std::shared_ptr<boost::asio::ip::tcp::socket>    SocketPtr;

class NetServer {

    friend class TcpConnAsync;

    __noncopyable__(NetServer)

public:

    /// Construct the server to listen on the specified TCP address and port
    explicit NetServer(const std::string& instance_name) :
        instance_name_(instance_name),
        io_service_(),
        acceptor_(),
        conf_(),
        io_service_threads_() {
    }
    ~NetServer() = default;

    bool init();

    void service() {

        // 线程池开始工作
        io_service_threads_.start_threads();

        acceptor_.reset(new boost::asio::ip::tcp::acceptor(io_service_));
        acceptor_->open(ep_.protocol());

        acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_->bind(ep_);
        acceptor_->listen(boost::asio::socket_base::max_connections);

        do_accept();

    }

public:

    int ops_cancel_time_out() const {
        return conf_.ops_cancel_time_out_;
    }

    int session_cancel_time_out() const {
        return conf_.session_cancel_time_out_;
    }

    int send_max_msg_size() const {
        return conf_.send_max_msg_size_;
    }

    int recv_max_msg_size() const {
        return conf_.recv_max_msg_size_;
    }
private:

    // accept stuffs
    void do_accept();
    void accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr);

private:


    const std::string instance_name_;

    // 侦听地址信息
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::endpoint ep_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;

    NetConf conf_;

private:
    roo::ThreadPool io_service_threads_;
    void io_service_run(roo::ThreadObjPtr ptr);  // main task loop

public:
    int io_service_stop_graceful() {

        roo::log_warning("About to stop io_service... ");

        io_service_.stop();
        io_service_threads_.graceful_stop_threads();
        return 0;
    }

    int io_service_join() {
        io_service_threads_.join_threads();
        return 0;
    }

    int module_runtime(const libconfig::Config& conf);
    int module_status(std::string& module, std::string& name, std::string& val);

};


} // end namespace tzrpc


#endif // __NETWORK_NET_SERVER_H__

