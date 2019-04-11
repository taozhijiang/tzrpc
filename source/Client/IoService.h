/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __IO_SERVICE_H__
#define __IO_SERVICE_H__

#include <boost/asio.hpp>

#include <thread>
#include <mutex>
#include <memory>

#include <Client/LogClient.h>

// 提供定时回调接口服务


namespace tzrpc_client {

class IoService {

public:

    static IoService& instance();

    bool init() {

        if (initialized_) {
            return true;
        }

        std::lock_guard<std::mutex> lock(lock_);

        if (initialized_) {
            return true;
        }

        // 创建io_service工作线程
        io_service_thread_ = std::thread(std::bind(&IoService::io_service_run, this));
        initialized_ = true;

        log_notice("IoService initialized ok.");
        return true;

    }

    boost::asio::io_service& get_io_service() {
        return  io_service_;
    }

    void stop_io_service() {

        io_service_.stop();
        work_guard_.reset();
    }


private:

    IoService():
        lock_(),
        initialized_(false),
        io_service_thread_(),
        io_service_(),
        work_guard_(new boost::asio::io_service::work(io_service_)){
    }

    ~IoService() {
        io_service_.stop();
        work_guard_.reset();
        
        if (io_service_thread_.joinable())
            io_service_thread_.join();
    }

    // 禁止拷贝
    IoService(const IoService&) = delete;
    IoService& operator=(const IoService&) = delete;

    // 确保只初始化一次，double-lock-check
    std::mutex lock_;
    bool initialized_;

    // 再启一个io_service_，主要来处理定时器等常用服务
    std::thread io_service_thread_;
    boost::asio::io_service io_service_;

    // io_service如果没有任务，会直接退出执行，所以需要
    // 一个强制的work来持有之
    std::unique_ptr<boost::asio::io_service::work> work_guard_;

    void io_service_run() {

        log_notice("io_service thread running...");

        // io_service would not have had any work to do,
        // and consequently io_service::run() would have returned immediately.

        boost::system::error_code ec;
        io_service_.run(ec);

        log_notice("io_service thread terminated ...");
        log_notice("error_code: {%d} %s", ec.value(), ec.message().c_str());
    }

};

} // end namespace tzrpc_client


#endif // __IO_SERVICE_H__
