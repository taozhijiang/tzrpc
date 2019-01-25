#ifndef __NETWORK_NET_SERVER_H__
#define __NETWORK_NET_SERVER_H__

#include <mutex>

#include <boost/noncopyable.hpp>
#include <boost/atomic/atomic.hpp>

#include <xtra_asio.h>

#include <Utils/Log.h>
#include <Utils/ThreadPool.h>
#include <Scaffold/Setting.h>

namespace tzrpc {

// forward
class TcpConnAsync;

class NetServer: public boost::noncopyable {

    friend class TcpConnAsync;

public:

    /// Construct the server to listen on the specified TCP address and port
    explicit NetServer(uint32_t pool_size, const std::string& instance_name):
        pool_size_(pool_size),
        instance_name_(instance_name),
        io_service_(),
        acceptor_(),
        io_service_threads_(pool_size) {
    }

    bool init() {
        ep_ = ip::tcp::endpoint(ip::address::from_string(setting.bind_ip_), setting.bind_port_);
        log_alert("create listen endpoint for %s:%d",
                      setting.bind_ip_.c_str(), setting.bind_port_);

        if (!io_service_threads_.init_threads(
            std::bind(&NetServer::io_service_run, this, std::placeholders::_1),
                pool_size_)) {
            log_err("io_service_run init task failed!");
            return false;
        }

        return true;
    }

    void service() {

        // 线程池开始工作
        io_service_threads_.start_threads();

        acceptor_.reset( new ip::tcp::acceptor(io_service_) );
        acceptor_->open(ep_.protocol());

        acceptor_->set_option(ip::tcp::acceptor::reuse_address(true));
        acceptor_->bind(ep_);
        acceptor_->listen(socket_base::max_connections);

        do_accept();

    }


private:

    // accept stuffs
    void do_accept();
    void accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr);

private:


    const uint32_t pool_size_;
    const std::string instance_name_;

    // 侦听地址信息
    io_service io_service_;
    ip::tcp::endpoint ep_;
    std::unique_ptr<ip::tcp::acceptor> acceptor_;

public:
    ThreadPool io_service_threads_;
    void io_service_run(ThreadObjPtr ptr);  // main task loop
    int io_service_stop_graceful();
    int io_service_join();

};


} // end tzrpc


#endif // __NETWORK_NET_SERVER_H__

