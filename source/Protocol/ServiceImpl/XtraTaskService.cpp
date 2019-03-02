#include <Utils/Log.h>

#include <Core/ProtoBuf.h>

#include "XtraTaskService.h"

namespace tzrpc {

bool XtraTaskService::init() {

    auto conf_ptr = ConfHelper::instance().get_conf();
    if(!conf_ptr) {
        log_err("ConfHelper not initialized? return conf_ptr empty!!!");
        return false;
    }

    bool init_success = false;

    try {

        const libconfig::Setting &rpc_services = conf_ptr->lookup("rpc.services");

        for(int i = 0; i < rpc_services.getLength(); ++i) {

            const libconfig::Setting& service = rpc_services[i];

            // 跳过没有配置instance_name的配置
            std::string instance_name;
            service.lookupValue("instance_name", instance_name);
            if (instance_name.empty()) {
                log_err("check service conf, required instance_name not found, skip this one.");
                continue;
            }

            log_debug("detected instance_name: %s", instance_name.c_str());

            // 发现是匹配的，则找到对应虚拟主机的配置文件了
            if (instance_name == instance_name_) {
                if (!handle_rpc_service_conf(service)) {
                    log_err("handle detail conf for instnace %s failed.", instance_name.c_str());
                    return false;
                }

                // OK, we will return
                log_debug("handle detail conf for instance %s success!", instance_name.c_str());
                init_success = true;
                break;
            }
        }

    } catch (const libconfig::SettingNotFoundException &nfex) {
        log_err("rpc.services not found!");
    } catch (std::exception& e) {
        log_err("execptions catched for %s",  e.what());
    }

    if(!init_success) {
        log_err("instance %s init failed, may not configure for it?", instance_name_.c_str());
    }

    return init_success;
}

// 系统启动时候初始化，持有整个锁进行
bool XtraTaskService::handle_rpc_service_conf(const libconfig::Setting& setting) {

    std::unique_lock<std::mutex> lock(conf_lock_);

    if (!conf_ptr_) {
        conf_ptr_.reset(new DetailExecutorConf());
        if (!conf_ptr_) {
            log_err("create DetailExecutorConf instance failed.");
            return false;
        }
    }

    setting.lookupValue("exec_thread_pool_size", conf_ptr_->executor_conf_.exec_thread_number_);
    setting.lookupValue("exec_thread_pool_size_hard", conf_ptr_->executor_conf_.exec_thread_number_hard_);
    setting.lookupValue("exec_thread_pool_step_size", conf_ptr_->executor_conf_.exec_thread_step_size_);

    // 检查ExecutorConf参数合法性
    if (conf_ptr_->executor_conf_.exec_thread_number_hard_ < conf_ptr_->executor_conf_.exec_thread_number_) {
        conf_ptr_->executor_conf_.exec_thread_number_hard_ = conf_ptr_->executor_conf_.exec_thread_number_;
    }

    if (conf_ptr_->executor_conf_.exec_thread_number_ <= 0 ||
        conf_ptr_->executor_conf_.exec_thread_number_ > 100 ||
        conf_ptr_->executor_conf_.exec_thread_number_hard_ > 100 ||
        conf_ptr_->executor_conf_.exec_thread_number_hard_ < conf_ptr_->executor_conf_.exec_thread_number_ )
    {
        log_err("invalid exec_thread setting: %d, %d",
                conf_ptr_->executor_conf_.exec_thread_number_,
                conf_ptr_->executor_conf_.exec_thread_number_hard_);
        return false;
    }

    // 可以为0，表示不进行动态更新
    if (conf_ptr_->executor_conf_.exec_thread_step_size_ < 0) {
        log_err("invalid exec_thread_step_size setting: %d",
                conf_ptr_->executor_conf_.exec_thread_step_size_);
        return false;
    }

    // other conf handle may add code here...

    return true;
}



ExecutorConf XtraTaskService::get_executor_conf() override {
    SAFE_ASSERT(conf_ptr_);
    return conf_ptr_->executor_conf_;
}

int XtraTaskService::update_runtime_conf(const libconfig::Config& conf) override {

    try {

        const libconfig::Setting &rpc_services = conf.lookup("rpc_services");

        for(int i = 0; i < rpc_services.getLength(); ++i) {

            const libconfig::Setting& service = rpc_services[i];
            std::string instance_name;
            service.lookupValue("instance_name", instance_name);

            // 发现是匹配的，则找到对应虚拟主机的配置文件了
            if (instance_name == instance_name_) {
                log_notice("about to handle_rpc_service_runtime_conf update for %s", instance_name_.c_str());
                return handle_rpc_service_runtime_conf(service);
            }
        }

    } catch (const libconfig::SettingNotFoundException &nfex) {
        log_err("rpc_services not found!");
    } catch (std::exception& e) {
        log_err("execptions catched for %s",  e.what());
    }

    log_err("conf for instance %s not found!!!!", instance_name_.c_str());
    return -1;
}

// 做一些可选的配置动态更新
bool XtraTaskService::handle_rpc_service_runtime_conf(const libconfig::Setting& setting) {

    std::shared_ptr<DetailExecutorConf> conf_ptr = std::make_shared<DetailExecutorConf>();
    if (!conf_ptr) {
        log_err("create DetailExecutorConf instance failed.");
        return -1;
    }

    setting.lookupValue("exec_thread_pool_size", conf_ptr->executor_conf_.exec_thread_number_);
    setting.lookupValue("exec_thread_pool_size_hard", conf_ptr->executor_conf_.exec_thread_number_hard_);
    setting.lookupValue("exec_thread_pool_step_size", conf_ptr->executor_conf_.exec_thread_step_size_);

    // 检查ExecutorConf参数合法性
    if (conf_ptr->executor_conf_.exec_thread_number_hard_ < conf_ptr->executor_conf_.exec_thread_number_) {
        conf_ptr->executor_conf_.exec_thread_number_hard_ = conf_ptr->executor_conf_.exec_thread_number_;
    }

    if (conf_ptr->executor_conf_.exec_thread_number_ <= 0 ||
        conf_ptr->executor_conf_.exec_thread_number_ > 100 ||
        conf_ptr->executor_conf_.exec_thread_number_hard_ > 100 ||
        conf_ptr->executor_conf_.exec_thread_number_hard_ < conf_ptr->executor_conf_.exec_thread_number_ )
    {
        log_err("invalid exec_thread_pool_size setting: %d, %d",
                conf_ptr->executor_conf_.exec_thread_number_, conf_ptr->executor_conf_.exec_thread_number_hard_);
        return -1;
    }

    if (conf_ptr->executor_conf_.exec_thread_step_size_ < 0) {
        log_err("invalid exec_thread_step_size setting: %d",
                conf_ptr->executor_conf_.exec_thread_step_size_);
        return -1;
    }

    {
        // do swap here
        std::unique_lock<std::mutex> lock(conf_lock_);
        conf_ptr_.swap(conf_ptr);
    }

    return 0;
}

int XtraTaskService::module_status(std::string& strModule, std::string& strKey, std::string& strValue) override {

    // empty status ...

    return 0;
}


void XtraTaskService::handle_RPC(std::shared_ptr<RpcInstance> rpc_instance) override {

    using XtraTask::OpCode;

    // Call the appropriate RPC handler based on the request's opCode.
    switch (rpc_instance->get_opcode()) {
        case OpCode::CMD_READ:
            read_ops_impl(rpc_instance);
            break;
        case OpCode::CMD_WRITE:
            write_ops_impl(rpc_instance);
            break;

        default:
            log_err("Received RPC request with unknown opcode %u: "
                    "rejecting it as invalid request",
                    rpc_instance->get_opcode());
            rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
    }
}


void XtraTaskService::read_ops_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    // 再做一次opcode校验
    RpcRequestMessage& rpc_request_message = rpc_instance->get_rpc_request_message();
    if (rpc_request_message.header_.opcode != XtraTask::OpCode::CMD_READ) {
        log_err("invalid opcode %u in service XtraTask.", rpc_request_message.header_.opcode);
        rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
        return;
    }

    XtraTask::XtraReadOps::Response response;
    response.set_code(0);
    response.set_msg("OK");

    do {

        // 消息体的unmarshal
        XtraTask::XtraReadOps::Request request;
        if (!ProtoBuf::unmarshalling_from_string(rpc_request_message.payload_, &request)) {
            log_err("unmarshal request failed.");
            response.set_code(-1);
            response.set_msg("参数错误");
            break;
        }

        // 相同类目下的子RPC分发
        if (request.has_ping()) {
            log_debug("XtraTask::XtraReadOps::ping -> %s", request.ping().msg().c_str());
            response.mutable_ping()->set_msg("[[[pong]]]");
            break;
        } else if (request.has_gets()) {
            log_debug("XtraTask::XtraReadOps::get -> %s", request.gets().key().c_str());
            response.mutable_gets()->set_value("[[[pong]]]");
            break;
        } else if (request.has_echo()) {
            std::string real_msg = request.echo().msg();
            log_debug("XtraTask::XtraReadOps::echo -> %s", real_msg.c_str());
            response.mutable_echo()->set_msg("echo:" + real_msg);
        } else if (request.has_test_timeout()) {
            int32_t timeout = request.test_timeout().timeout();
            log_debug("this thread will sleep for %d sec.", timeout);
            ::sleep(timeout);
            response.mutable_test_timeout()->set_timeout("you should not see this.");
        } else {
            log_err("undetected specified service call.");
            rpc_instance->reject(RpcResponseStatus::INVALID_REQUEST);
            return;
        }

    } while (0);

    std::string response_str;
    ProtoBuf::marshalling_to_string(response, &response_str);
    rpc_instance->reply_rpc_message(response_str);
}

void XtraTaskService::write_ops_impl(std::shared_ptr<RpcInstance> rpc_instance) {

    #if 0
    PRELUDE(XtraWriteCmd);


    rpc.reply(response);
    #endif

    const RpcRequestMessage& msg = rpc_instance->get_rpc_request_message();
    log_debug(" write ops recv: %s", msg.dump().c_str());

    rpc_instance->reply_rpc_message("nicol_write_reply");
}


} // namespace tzrpc
