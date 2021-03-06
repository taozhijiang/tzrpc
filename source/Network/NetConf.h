/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __NETWORK_NET_CONF_H__
#define __NETWORK_NET_CONF_H__

#include <xtra_rhel.h>
#include <scaffold/Status.h>
#include <scaffold/Setting.h>

#include <other/Log.h>

#include <concurrency/ThreadPool.h>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
using boost::asio::steady_timer;

namespace tzrpc {

// forward
class TcpConnAsync;
class NetServer;

class NetConf {

    friend class NetServer;

    __noncopyable__(NetConf)

private:

    bool        service_enabled_;   // 服务开关
    int32_t     service_speed_;
    int32_t     service_token_;

    int32_t     service_concurrency_;       // 最大连接并发控制

    int32_t     session_cancel_time_out_;   // session间隔会话时长
    int32_t     ops_cancel_time_out_;       // ops操作超时时长

    int32_t     send_max_msg_size_;         // 如果为0，则不限制
    int32_t     recv_max_msg_size_;         // 如果为0，则不限制

    std::string bind_addr_;
    int32_t     bind_port_;

    // 加载、更新配置的时候保护竞争状态
    // 这里保护主要是非atomic操作的string结构
    // 其他的数据结构都是4字节对其的，intel确保能够原子读取和更新
    std::mutex  lock_;
    std::set<std::string> safe_ip_;

    int32_t     backlog_size_;
    int32_t     io_thread_number_;

    bool load_conf(std::shared_ptr<libconfig::Config> conf_ptr);
    bool load_conf(const libconfig::Config& conf);

    bool check_safe_ip(const std::string& ip) {
        std::lock_guard<std::mutex> lock(lock_);
        return (safe_ip_.empty() || (safe_ip_.find(ip) != safe_ip_.cend()));
    }

    bool get_service_token() {

        // 注意：
        // 如果关闭这个选项，则整个服务都不可用了(包括管理页面)
        // 此时如果需要变更除非重启服务，或者采用非web方式(比如发送命令)来恢复配置

        if (!service_enabled_) {
            roo::log_info("service not enabled ...");
            return false;
        }

        // 下面就不使用锁来保证严格的一致性了，因为非关键参数，过多的锁会影响性能
        if (service_speed_ == 0) // 没有限流
            return true;

        if (service_token_ <= 0) {
            roo::log_info("service should not speed over ...");
            return false;
        }

        service_token_ = service_token_ - 1;
        return true;
    }

    void withdraw_service_token() {    // 支持将令牌还回去
        ++service_token_;
    }

    // 喂狗
    void feed_service_token() {
        service_token_ = service_speed_;
    }

    std::shared_ptr<steady_timer> timed_feed_token_;
    void timed_feed_token_handler(const boost::system::error_code& ec);

    // 行为良好的默认初始化值

    NetConf() :
        service_enabled_(true),
        service_speed_(0),
        service_token_(0),
        service_concurrency_(0),
        session_cancel_time_out_(0),
        ops_cancel_time_out_(0),
        send_max_msg_size_(0),
        recv_max_msg_size_(0),
        bind_addr_(),
        bind_port_(0),
        lock_(),
        safe_ip_(),
        backlog_size_(10),
        io_thread_number_(1) {
    }

} __attribute__((aligned(4)));  // end class NetConf



} // end namespace tzrpc


#endif // __NETWORK_NET_CONF_H__

