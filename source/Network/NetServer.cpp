/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <xtra_rhel6.h>

#include <Network/NetServer.h>
#include <Network/TcpConnAsync.h>

#include <Scaffold/ConfHelper.h>
#include <Scaffold/Status.h>
#include <Utils/StrUtil.h>

namespace tzrpc {

bool NetConf::load_conf(std::shared_ptr<libconfig::Config> conf_ptr) {
    const auto& conf = *conf_ptr;
    return load_conf(conf);
}

bool NetConf::load_conf(const libconfig::Config& conf) {

    int listen_port = 0;
    ConfUtil::conf_value(conf, "network.bind_addr", bind_addr_);
    ConfUtil::conf_value(conf, "network.listen_port", listen_port);
    if (bind_addr_.empty() || listen_port <=0 ){
        log_err( "invalid network.bind_addr %s & http.listen_port %d",
                  bind_addr_.c_str(), listen_port);
        return false;
    }
    listen_port_ = static_cast<unsigned short>(listen_port);

    std::string ip_list;
    ConfUtil::conf_value(conf, "network.safe_ip", ip_list);
    if (!ip_list.empty()) {
        std::vector<std::string> ip_vec;
        std::set<std::string> ip_set;
        boost::split(ip_vec, ip_list, boost::is_any_of(";"));
        for (std::vector<std::string>::iterator it = ip_vec.begin(); it != ip_vec.cend(); ++it){
            std::string tmp = boost::trim_copy(*it);
            if (tmp.empty())
                continue;

            ip_set.insert(tmp);
        }

        std::swap(ip_set, safe_ip_);
    }
    if (!safe_ip_.empty()) {
        log_alert("safe_ip not empty, totally contain %d items",
                          static_cast<int>(safe_ip_.size()));
    }

    ConfUtil::conf_value(conf, "network.backlog_size", backlog_size_);
    if (backlog_size_ < 0) {
        log_err( "invalid network.backlog_size %d.", backlog_size_);
        return false;
    }

    ConfUtil::conf_value(conf, "network.io_thread_pool_size", io_thread_number_);
    if (io_thread_number_ < 0) {
        log_err( "invalid network.io_thread_number %d", io_thread_number_);
        return false;
    }

    // other http parameters
    int value_i;

    ConfUtil::conf_value(conf, "network.ops_cancel_time_out", value_i);
    if (value_i < 0){
        log_err("invalid network ops_cancel_time_out value.");
        return false;
    }
    ops_cancel_time_out_ = value_i;

    ConfUtil::conf_value(conf, "network.session_cancel_time_out", value_i);
    if (value_i < 0){
        log_err("invalid network session_cancel_time_out value.");
        return false;
    }
    session_cancel_time_out_ = value_i;

    bool value_b;
    ConfUtil::conf_value(conf, "network.service_enable", value_b, true);
    ConfUtil::conf_value(conf, "network.service_speed", value_i);
    if (value_i < 0){
        log_err("invalid network.service_speed value %d.", value_i);
        return false;
    }
    service_enabled_ = value_b;
    service_speed_ = value_i;

    ConfUtil::conf_value(conf, "network.max_msg_size", value_i);
    if (value_i <= 32 /*actual sizeof RpcRequestMessage, RpcResponseMessage*/){
        log_err("invalid network max_msg_size value.");
        return false;
    }
    max_msg_size_ = value_i;

    log_debug("NetConf parse conf OK!");

    return true;
}

void NetConf::timed_feed_token_handler(const boost::system::error_code& ec) {

    if (service_speed_ == 0) {
        log_alert("unlock speed jail, close the timer.");
        timed_feed_token_.reset();
        return;
    }

    // 恢复token
    feed_service_token();

    // 再次启动定时器
    timed_feed_token_->expires_from_now(boost::chrono::seconds(1)); // 1sec
    timed_feed_token_->async_wait(
                std::bind(&NetConf::timed_feed_token_handler, this, std::placeholders::_1));
}


bool NetServer::init() {

    auto conf_ptr = ConfHelper::instance().get_conf();

    // protect cfg race conditon
    std::lock_guard<std::mutex> lock(conf_.lock_);
    if (!conf_.load_conf(conf_ptr)) {
        log_err("Load conf failed!");
        return false;
    }

    ep_ = ip::tcp::endpoint(ip::address::from_string(conf_.bind_addr_), conf_.listen_port_);
    log_alert("create listen endpoint for %s:%d",
                      conf_.bind_addr_.c_str(), conf_.listen_port_);

    log_debug("socket/session conn cancel time_out: %d secs, enabled: %s",
                      conf_.ops_cancel_time_out_.load(),
                      conf_.ops_cancel_time_out_ > 0 ? "true" : "false");

    if (conf_.service_speed_) {
        conf_.timed_feed_token_.reset(new steady_timer (io_service_)); // 1sec
        if (!conf_.timed_feed_token_) {
            log_err("Create timed_feed_token_ failed!");
            return false;
        }

        conf_.timed_feed_token_->expires_from_now(boost::chrono::seconds(1));
        conf_.timed_feed_token_->async_wait(
                    std::bind(&NetConf::timed_feed_token_handler, &conf_, std::placeholders::_1));
    }
    log_debug("network service enabled: %s, speed: %ld tps", conf_.service_enabled_ ? "true" : "false",
              conf_.service_speed_.load());

    if (!io_service_threads_.init_threads(
        std::bind(&NetServer::io_service_run, this, std::placeholders::_1),
        conf_.io_thread_number_)) {
        log_err("NetServer::io_service_run init task failed!");
        return false;
    }

    // 注册配置动态更新的回调函数
    ConfHelper::instance().register_conf_callback(
            std::bind(&NetServer::update_runtime_conf, this,
                      std::placeholders::_1));

    // 系统状态展示相关的初始化
    Status::instance().register_status_callback(
            "net_server",
            std::bind(&NetServer::module_status, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    return true;
}

// accept stuffs
void NetServer::do_accept() {

    SocketPtr sock_ptr(new ip::tcp::socket(io_service_));
    acceptor_->async_accept(*sock_ptr,
                       std::bind(&NetServer::accept_handler, this,
                                   std::placeholders::_1, sock_ptr));
}


void NetServer::accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr) {

    do {

        if (ec) {
            log_err("error during accept with %d, %s", ec.value(), ec.message().c_str());
            break;
        }

        boost::system::error_code ignore_ec;
        auto remote = sock_ptr->remote_endpoint(ignore_ec);
        if (ignore_ec) {
            log_err("get remote info failed:%d, %s", ignore_ec.value(), ignore_ec.message().c_str());
            break;
        }

        std::string remote_ip = remote.address().to_string(ignore_ec);
        log_debug("remote Client Info: %s:%d", remote_ip.c_str(), remote.port());

        TcpConnAsyncPtr new_conn = std::make_shared<TcpConnAsync>(sock_ptr, *this);
        new_conn->start();

    } while (0);

    // 再次启动接收异步请求
    do_accept();
}


void NetServer::io_service_run(ThreadObjPtr ptr) {

    while (true) {

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1*1000*1000);
            continue;
        }

        log_alert("io_service thread %#lx about to loop...", (long)pthread_self());
        boost::system::error_code ec;
        io_service_.run(ec);

        if (ec){
            log_err("io_service stopped...");
            break;
        }
    }

    ptr->status_ = ThreadStatus::kDead;
    log_info("io_service thread %#lx is about to terminate ... ", (long)pthread_self());

    return;
}



int NetServer::module_status(std::string& strModule, std::string& strKey, std::string& strValue) {

    strModule = "tzrpc";
    strKey = "net_server";

    std::stringstream ss;

    ss << "\t" << "instance_name: " << instance_name_ << std::endl;
    ss << "\t" << "service_addr: " << conf_.bind_addr_ << "@" << conf_.listen_port_ << std::endl;
    ss << "\t" << "backlog_size: " << conf_.backlog_size_ << std::endl;
    ss << "\t" << "io_thread_pool_size: " << conf_.io_thread_number_ << std::endl;
    ss << "\t" << "safe_ips: " ;

    {
        // protect cfg race conditon
        std::lock_guard<std::mutex> lock(conf_.lock_);
        for (auto iter = conf_.safe_ip_.begin(); iter != conf_.safe_ip_.end(); ++iter) {
            ss << *iter << ", ";
        }
        ss << std::endl;
    }

    ss << "\t" << std::endl;

    ss << "\t" << "service_enabled: " << (conf_.service_enabled_  ? "true" : "false") << std::endl;
    ss << "\t" << "service_speed(tps): " << conf_.service_speed_ << std::endl;
    ss << "\t" << "session_cancel_time_out: " << conf_.session_cancel_time_out_ << std::endl;
    ss << "\t" << "ops_cancel_time_out: " << conf_.ops_cancel_time_out_ << std::endl;

    strValue = ss.str();
    return 0;
}


int NetServer::update_runtime_conf(const libconfig::Config& cfg) {

    NetConf conf {};
    if (!conf.load_conf(cfg)) {
        log_err("load conf for HttpConf failed.");
        return -1;
    }

    if (conf_.session_cancel_time_out_ != conf.session_cancel_time_out_) {
        log_notice("update session_cancel_time_out from %d to %d",
                   conf_.session_cancel_time_out_.load(), conf.session_cancel_time_out_.load());
        conf_.session_cancel_time_out_ = conf.session_cancel_time_out_.load();
    }

    if (conf_.ops_cancel_time_out_ != conf.ops_cancel_time_out_) {
        log_notice("update ops_cancel_time_out from %d to %d",
                   conf_.ops_cancel_time_out_.load(),  conf.ops_cancel_time_out_.load());
        conf_.ops_cancel_time_out_ = conf.ops_cancel_time_out_.load();
    }


    log_notice("swap safe_ips...");

    {
        // protect cfg race conditon
        std::lock_guard<std::mutex> lock(conf_.lock_);
        conf_.safe_ip_.swap(conf.safe_ip_);
    }

    if (conf_.service_speed_ != conf.service_speed_) {
        log_notice("update http_service_speed from %ld to %ld",
                   conf_.service_speed_.load(), conf.service_speed_.load());
        conf_.service_speed_ = conf.service_speed_.load();

        // 检查定时器是否存在
        if (conf_.service_speed_) {

            // 直接重置定时器，无论有没有
            conf_.timed_feed_token_.reset(new steady_timer(io_service_)); // 1sec
            if (!conf_.timed_feed_token_) {
                log_err("Create timed_feed_token_ failed!");
                return -1;
            }

            conf_.timed_feed_token_->expires_from_now(boost::chrono::seconds(1));
            conf_.timed_feed_token_->async_wait(
                        std::bind(&NetConf::timed_feed_token_handler, &conf_, std::placeholders::_1));
        }
        else // speed == 0
        {
            if (conf_.timed_feed_token_) {
                boost::system::error_code ignore_ec;
                conf_.timed_feed_token_->cancel(ignore_ec);
                conf_.timed_feed_token_.reset();
            }
        }
    }

    log_notice("service enabled: %s, speed: %ld", conf_.service_enabled_ ? "true" : "false",
                conf_.service_speed_.load());

    return 0;
}



} // end namespace tzrpc
