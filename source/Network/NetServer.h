/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __NETWORK_NET_SERVER_H__
#define __NETWORK_NET_SERVER_H__

#include <mutex>

#include <boost/noncopyable.hpp>

#include <xtra_asio.h>


#include <libconfig.h++>

#include <Utils/Log.h>
#include <Utils/ThreadPool.h>

namespace tzrpc {

// forward
class TcpConnAsync;
class NetServer;

class NetConf {

    friend class NetServer;

    int     session_cancel_time_out_;    // session间隔会话时长
    int     ops_cancel_time_out_;        // ops操作超时时长

    bool    service_enabled_;   // 服务开关
    int     service_speed_;
    int     service_token_;

    int     max_msg_size_;       // 如果为0，则不限制

private:
    bool load_conf(std::shared_ptr<libconfig::Config> conf_ptr);
    bool load_conf(const libconfig::Config& conf);

private:
    std::string bind_addr_;
    unsigned short listen_port_;
    std::set<std::string> safe_ip_;

    int backlog_size_;
    int io_thread_number_;

    // 加载、更新配置的时候保护竞争状态
    // 这里保护主要是非atomic操作的string结构
    std::mutex             lock_;


    bool check_safe_ip(const std::string& ip) {
        std::lock_guard<std::mutex> lock(lock_);
        return ( safe_ip_.empty() || (safe_ip_.find(ip) != safe_ip_.cend()) );
    }

    bool get_service_token() {

        // 注意：
        // 如果关闭这个选项，则整个服务都不可用了(包括管理页面)
        // 此时如果需要变更除非重启服务，或者采用非web方式(比如发送命令)来恢复配置

        if (!service_enabled_) {
            log_alert("service not enabled ...");
            return false;
        }

        // 下面就不使用锁来保证严格的一致性了，因为非关键参数，过多的锁会影响性能
        if (service_speed_ == 0) // 没有限流
            return true;

        if (service_token_ <= 0) {
            log_alert("service not speed over ...");
            return false;
        }

        -- service_token_;
        return true;
    }

    void withdraw_service_token() {    // 支持将令牌还回去
        ++ service_token_;
    }

    void feed_service_token(){
        service_token_ = service_speed_;
    }

    std::shared_ptr<steady_timer> timed_feed_token_;
    void timed_feed_token_handler(const boost::system::error_code& ec);

} __attribute__ ((aligned (4)));  // end class NetConf

class NetServer: public boost::noncopyable {

    friend class TcpConnAsync;

public:

    /// Construct the server to listen on the specified TCP address and port
    explicit NetServer(const std::string& instance_name):
        instance_name_(instance_name),
        io_service_(),
        acceptor_(),
        conf_(),
        io_service_threads_() {
    }

    bool init();

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

public:

    int ops_cancel_time_out() const {
        return conf_.ops_cancel_time_out_;
    }

    int session_cancel_time_out() const {
        return conf_.session_cancel_time_out_;
    }

    int max_msg_size() const {
        return conf_.max_msg_size_;
    }

private:

    // accept stuffs
    void do_accept();
    void accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr);

private:


    const std::string instance_name_;

    // 侦听地址信息
    io_service io_service_;
    ip::tcp::endpoint ep_;
    std::unique_ptr<ip::tcp::acceptor> acceptor_;

    NetConf conf_;

private:
    ThreadPool io_service_threads_;
    void io_service_run(ThreadObjPtr ptr);  // main task loop

public:
    int io_service_stop_graceful() {

        log_err("about to stop io_service... ");

        io_service_.stop();
        io_service_threads_.graceful_stop_threads();
        return 0;
    }

    int io_service_join() {
        io_service_threads_.join_threads();
        return 0;
    }

    int update_runtime_conf(const libconfig::Config& conf);
    int module_status(std::string& strModule, std::string& strKey, std::string& strValue);

};


} // end namespace tzrpc


#endif // __NETWORK_NET_SERVER_H__

