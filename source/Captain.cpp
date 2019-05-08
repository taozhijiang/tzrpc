/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <cstdlib>

#include <memory>
#include <string>
#include <map>

#include <other/Log.h>
#include <Network/NetServer.h>

#include <concurrency/Timer.h>

#include <scaffold/Setting.h>
#include <scaffold/Status.h>

#include <RPC/Dispatcher.h>
#include <Protocol/Common.h>
#include <Protocol/ServiceImpl/XtraTaskService.h>


#include "Captain.h"

namespace tzrpc {

// 在主线程中最先初始化，所以不考虑竞争条件问题
Captain& Captain::instance() {
    static Captain service;
    return service;
}

Captain::Captain() :
    initialized_(false) {
}


bool Captain::init(const std::string& cfgFile) {

    if (initialized_) {
        roo::log_err("Captain already initlialized...");
        return false;
    }

    timer_ptr_ = std::make_shared<roo::Timer>();
    if (!timer_ptr_ || !timer_ptr_->init()) {
        roo::log_err("init Timer service failed, critical !!!!");
        return false;
    }

    setting_ptr_ = std::make_shared<roo::Setting>();
    if (!setting_ptr_ || !setting_ptr_->init(cfgFile)) {
        roo::log_err("init Setting (%s) failed, critical !!!!", cfgFile.c_str());
        return false;
    }

    auto setting_ptr = setting_ptr_->get_setting();
    if (!setting_ptr) {
        roo::log_err("Setting return null setting pointer, maybe your conf file ill!");
        return false;
    }

    int log_level = 0;
    setting_ptr->lookupValue("log_level", log_level);
    if (log_level <= 0 || log_level > 7) {
        roo::log_warning("invalid log_level value, reset to default 7.");
        log_level = 7;
    }

    roo::log_init(log_level, "", "./log", LOG_LOCAL6);
    roo::log_warning("initialized log with level: %d", log_level);

    status_ptr_ = std::make_shared<roo::Status>();
    if (!status_ptr_) {
        roo::log_err("init Status failed, critical !!!!");
        return false;
    }

    net_server_ptr_.reset(new NetServer("tzrpc_server"));
    if (!net_server_ptr_ || !net_server_ptr_->init()) {
        roo::log_err("init NetServer failed!");
        return false;
    }

    // 先注册具体的服务
    // 这里的初始化是调用服务实现侧的初始化函数，意味着要完成配置读取等操作
    std::shared_ptr<Service> xtra_task_service = std::make_shared<XtraTaskService>("XtraTaskService");
    if (!xtra_task_service || !xtra_task_service->init()) {
        roo::log_err("create XtraTaskService failed.");
        return false;
    }
    Dispatcher::instance().register_service(ServiceID::XTRA_TASK_SERVICE, xtra_task_service);

    // 再进行整体服务的初始化
    if (!Dispatcher::instance().init()) {
        roo::log_err("Init Dispatcher failed.");
        return false;
    }

    // do real service
    net_server_ptr_->service();

    roo::log_warning("Captain all initialized...");
    initialized_ = true;

    return true;
}


bool Captain::service_graceful() {

    timer_ptr_->threads_join();
    net_server_ptr_->io_service_stop_graceful();
    return true;
}

void Captain::service_terminate() {
    ::sleep(1);
    ::_exit(0);
}

bool Captain::service_joinall() {

    timer_ptr_->threads_join();
    net_server_ptr_->io_service_join();
    return true;
}


} // end namespace tzrpc
