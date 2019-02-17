/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <RPC/Dispatcher.h>

namespace tzrpc {

Dispatcher& Dispatcher::instance() {
    static Dispatcher dispatcher;
    return dispatcher;
}



int Dispatcher::update_runtime_conf(const libconfig::Config& conf) {

    int ret_sum = 0;
    int ret = 0;

    for (auto iter = services_.begin(); iter != services_.end(); ++iter) {

        auto executor = iter->second;
        ret = executor->update_runtime_conf(conf);
        log_notice("update_runtime_conf for host %s return: %d",
                   executor->instance_name().c_str(), ret);
        ret_sum += ret;
    }

    return ret_sum;
}

} // end namespace tzrpc

