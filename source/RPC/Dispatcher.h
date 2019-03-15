/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_DISPATCHER_H__
#define __RPC_DISPATCHER_H__

#include <cinttypes>
#include <memory>

#include <map>
#include <mutex>


namespace tzrpc {

class RpcInstance;
class Service;
class Executor;

class Dispatcher {

public:
    static Dispatcher& instance();

    bool init();

    void register_service(uint16_t service_id, std::shared_ptr<Service> service);
    void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance);

    std::string instance_name() {
        return "Dispatcher";
    }

    int module_runtime(const libconfig::Config& conf);

private:

    Dispatcher():
        initialized_(false),
        services_({}) {
    }

    ~Dispatcher() {
    }

    // 禁止拷贝
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;

    bool initialized_;

    // 系统在启动的时候进行注册初始化，然后再提供服务，所以
    // 这边就不使用锁结构进行保护了，防止影响性能
    std::map<uint16_t, std::shared_ptr<Service>> services_;


};

} // end namespace tzrpc


#endif // __RPC_SERVICE_DISPATCHER_H__
