#include <syslog.h>
#include <cstdlib>

#include <memory>
#include <string>
#include <map>

#include <Network/NetServer.h>

#include <Utils/Utils.h>
#include <Utils/Log.h>

#include <Scaffold/Setting.h>
#include <Scaffold/Manager.h>


namespace tzrpc {

// 在主线程中最先初始化，所以不考虑竞争条件问题
Manager& Manager::instance() {
    static Manager service;
    return service;
}

Manager::Manager():
    initialized_(false){
}


bool Manager::init() {

    if (initialized_) {
        log_err("Manager already initlialized...");
        return false;
    }

    net_server_ptr_.reset(new NetServer(setting.io_thread_pool_size_, std::string(program_invocation_short_name)));
    if (!net_server_ptr_ || !net_server_ptr_->init()) {
        log_err("Init NetServer failed!");
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
