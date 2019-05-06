/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <other/Log.h>

#include <Client/RpcClientImpl.h>
#include <Client/include/RpcClient.h>

#include <system/ConstructException.h>

namespace tzrpc_client {

// empty handler
rpc_handler_t dummy_handler_;

RpcClient::RpcClient():
    initialized_(false),
    client_setting_() {
}

RpcClient::RpcClient(const std::string& addr, uint16_t port, const rpc_handler_t& handler):
    initialized_(false),
    client_setting_() {

    client_setting_.handler_ = handler;
    if(!init(addr, port))
        throw roo::ConstructException("construct RpcClient failed.");
}

RpcClient::RpcClient(const std::string& cfgFile, const rpc_handler_t& handler):
    initialized_(false),
    client_setting_() {

    client_setting_.handler_ = handler;
    if(!init(cfgFile))
        throw roo::ConstructException("construct RpcClient failed.");
}

RpcClient::RpcClient(const libconfig::Setting& setting, const rpc_handler_t& handler):
    initialized_(false),
    client_setting_() {

    client_setting_.handler_ = handler;
    if(!init(setting))
        throw roo::ConstructException("construct RpcClient failed.");
}

RpcClient::~RpcClient() {}

bool RpcClient::init(const std::string& addr, uint16_t port) {

    if (initialized_) {
        log_err("RpcClient already successfully initialized...");
        return true;
    }

    // init log first
    roo::log_init(client_setting_.log_level_);

    client_setting_.serv_addr_ = addr;
    client_setting_.serv_port_ = port;

    impl_.reset(new RpcClientImpl(std::cref(client_setting_)));
    if (!impl_ || !impl_->init()) {
        log_err("create impl failed.");
        return false;
    }

    initialized_ = true;
    return true;
}

bool RpcClient::init(const std::string& cfgFile) {

    libconfig::Config cfg;
    try {
        cfg.readFile(cfgFile.c_str());

        const libconfig::Setting& setting = cfg.lookup("rpc.client");
        return init(setting);

    } catch(libconfig::FileIOException &fioex) {
        log_err("I/O error while reading file.");
        return false;
    } catch(libconfig::ParseException &pex) {
        log_err("Parse error at %d - %s", pex.getLine(), pex.getError());
        return false;
    } catch (...) {
        log_err("process cfg failed.");
    }

    return false;
}

bool RpcClient::init(const libconfig::Setting& setting) {

    if (!setting.lookupValue("serv_addr", client_setting_.serv_addr_) ||
        !setting.lookupValue("serv_port", client_setting_.serv_port_) ||
        client_setting_.serv_addr_.empty() ||
        client_setting_.serv_port_ <= 0)
    {
        log_err("get rpc server addr config failed.");
        return false;
    }

    if (setting.lookupValue("send_max_msg_size", client_setting_.send_max_msg_size_) &&
        client_setting_.send_max_msg_size_ < 0)
    {
        log_err("invalid send_max_msg_size: %d", client_setting_.send_max_msg_size_);
        return false;
    }

    if (setting.lookupValue("recv_max_msg_size", client_setting_.recv_max_msg_size_) &&
        client_setting_.recv_max_msg_size_ < 0)
    {
        log_err("invalid recv_max_msg_size: %d", client_setting_.recv_max_msg_size_);
        return false;
    }

    if (setting.lookupValue("log_level", client_setting_.log_level_) &&
        client_setting_.log_level_ > 7)
    {
        log_err("invalid log_level: %u", client_setting_.log_level_);
        return false;
    }

    return init(client_setting_.serv_addr_,  client_setting_.serv_port_);
}


RpcClientStatus RpcClient::call_RPC(uint16_t service_id, uint16_t opcode,
                                    const std::string& payload, std::string& respload,
                                    uint32_t timeout_sec) {
    if (!initialized_ || !impl_) {
        log_err("RpcClientImpl not initialized, please check.");
        return RpcClientStatus::NETWORK_BEFORE_ERROR;
    }

    return impl_->call_RPC(service_id, opcode, payload, respload, timeout_sec);
}

RpcClientStatus RpcClient::call_RPC(uint16_t service_id, uint16_t opcode,
                                    const std::string& payload,
                                    uint32_t timeout_sec) {
    if (!initialized_ || !impl_) {
        log_err("RpcClientImpl not initialized, please check.");
        return RpcClientStatus::NETWORK_BEFORE_ERROR;
    }

    if (!client_setting_.handler_) {
        log_err("using async interface, but rpc_handler_t not provide.");
        return RpcClientStatus::NETWORK_BEFORE_ERROR;
    }

    return impl_->call_RPC(service_id, opcode, payload, timeout_sec);
}

} // end namespace tzrpc_client
