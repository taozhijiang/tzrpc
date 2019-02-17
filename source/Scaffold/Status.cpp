/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <Scaffold/Status.h>

namespace tzrpc {

Status& Status::instance() {
    static Status helper;
    return helper;
}


int Status::register_status_callback(const std::string& name, StatusCallable func) {

    if (name.empty() || !func){
        return -1;
    }

    std::lock_guard<std::mutex> lock(lock_);
    calls_[name] = func;
    return 0;
}

int Status::collect_status(std::string& output) {

    std::lock_guard<std::mutex> lock(lock_);

    std::map<std::string, std::string> results;
    for (auto iter = calls_.begin(); iter != calls_.end(); ++iter) {

        std::string strModule;
        std::string strKey;
        std::string strValue;

        int ret = iter->second(strModule, strKey, strValue);
        if (ret == 0) {
            std::string real = strModule + ":" + strKey;
            results[real] = strValue;
        } else {
            log_err("call collect_status of %s failed with: %d",  iter->first.c_str(), ret);
        }
    }

    std::stringstream ss;

    ss << "  *** SYSTEM RUNTIME STATUS ***  " << std::endl << std::endl;
    for (auto iter = results.begin(); iter != results.end(); ++iter) {
        ss << "[" << iter->first << "]" << std::endl;
        ss << iter->second << std::endl;
        ss << " ------------------------------- " << std::endl;
        ss << std::endl;
    }

    output = ss.str();
    return 0;
}

} // end namespace tzhttpd
