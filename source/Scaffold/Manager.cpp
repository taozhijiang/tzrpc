#include <syslog.h>
#include <cstdlib>

#include <memory>
#include <string>
#include <map>

#include <Utils/Utils.h>
#include <Utils/Log.h>

#include <Scaffold/Setting.h>
#include <Scaffold/Manager.h>


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

    log_info("Manager all initialized...");
    initialized_ = true;

    return true;
}


bool Manager::service_graceful() {
    return true;
}

void Manager::service_terminate() {
    ::sleep(1);
    ::_exit(0);
}

bool Manager::service_joinall() {
    return true;
}
