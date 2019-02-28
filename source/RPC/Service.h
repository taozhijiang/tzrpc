/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_SERVICE_H__
#define __RPC_SERVICE_H__

#include <libconfig.h++>
#include <boost/noncopyable.hpp>

#include <string>

// real rpc should implement this interface class

namespace tzrpc {

class RpcInstance;


// 简短的结构体，用来从DetailExecutor传递配置信息到Executor
// 因为主机相关的信息是在DetailExecutor中解析的

struct ExecutorConf {
    int exec_thread_number_;
    int exec_thread_number_hard_;  // 允许最大的线程数目
    int exec_thread_step_size_;
};


class Service: public boost::noncopyable {

public:
    Service() {}
    ~Service() {}

    // 根据opCode分发rpc请求的处理
    virtual void handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) = 0;
    virtual std::string instance_name() = 0;

    virtual bool init() = 0;

    virtual ExecutorConf get_executor_conf() = 0;
    virtual int update_runtime_conf(const libconfig::Config& conf) = 0;
    virtual int module_status(std::string& strModule, std::string& strKey, std::string& strValue) = 0;

};

} // end namespace tzrpc

#endif // __RPC_SERVICE_H__
