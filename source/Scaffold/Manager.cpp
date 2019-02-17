#include <syslog.h>
#include <cstdlib>

#include <memory>
#include <string>
#include <map>

#include <Network/NetServer.h>

#include <Utils/StrUtil.h>
#include <Utils/Utils.h>
#include <Utils/Log.h>

#include <Scaffold/ConfHelper.h>
#include <Scaffold/Manager.h>

#include <RPC/Dispatcher.h>
#include <Protocol/Common.h>
#include <Protocol/ServiceImpl/XtraTaskService.h>

namespace tzrpc {

// 在主线程中最先初始化，所以不考虑竞争条件问题
Manager& Manager::instance() {
    static Manager service;
    return service;
}

Manager::Manager():
    initialized_(false){
}


bool Manager::init(const std::string& cfgFile) {

    if (initialized_) {
        log_err("Manager already initlialized...");
        return false;
    }

    if(!ConfHelper::instance().init(cfgFile)) {
        fprintf(stderr, "init ConfHelper (%s) failed, critical !!!!\n", cfgFile.c_str());
        return false;
    }

    auto conf_ptr = ConfHelper::instance().get_conf();
    if(!conf_ptr) {
        log_err("ConfHelper return null conf pointer, maybe your conf file ill!");
        return false;
    }

    int log_level = 0;
    ConfUtil::conf_value(*conf_ptr, "log_level", log_level);
    if (log_level <= 0 || log_level > 7) {
        log_notice("invalid log_level value, reset to default 7.");
        log_level = 7;
    }

    log_init(log_level);
    log_notice("initialized log with level: %d", log_level);

    net_server_ptr_.reset(new NetServer("tzrpc_server"));
    if (!net_server_ptr_ || !net_server_ptr_->init()) {
        log_err("init NetServer failed!");
        return false;
    }

    // 先注册具体的服务
    // 这里的初始化是调用服务实现侧的初始化函数，意味着要完成配置读取等操作
    std::shared_ptr<Service> xtra_task_service = std::make_shared<XtraTaskService>("XtraTaskService");
    if (!xtra_task_service || !xtra_task_service->init()) {
        log_err("create XtraTaskService failed.");
        return false;
    }
    Dispatcher::instance().register_service(ServiceID::XTRA_TASK_SERVICE, xtra_task_service);

    // 再进行整体服务的初始化
    if (!Dispatcher::instance().init()) {
        log_err("Init Dispatcher failed.");
        return false;
    }

    // do real service
    net_server_ptr_->service();

    log_info("Manager all initialized...");
    initialized_ = true;

    return true;
}


bool Manager::service_graceful() {

    net_server_ptr_->io_service_stop_graceful();
    return true;
}

void Manager::service_terminate() {
    ::sleep(1);
    ::_exit(0);
}

bool Manager::service_joinall() {

    net_server_ptr_->io_service_join();
    return true;
}


} // end tzrpc
