/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __SCAFFOLD_CONF_HELPER__
#define __SCAFFOLD_CONF_HELPER__

#include <memory>
#include <mutex>
#include <vector>
#include <functional>

#include <libconfig.h++>

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include <Utils/StrUtil.h>
#include <Utils/Log.h>


// 值拷贝

namespace tzrpc {

// 配置动态更新回调函数接口类型
typedef std::function<int (const libconfig::Config& cfg)> ConfUpdateCallable;

class ConfHelper: public boost::noncopyable {

public:
    static ConfHelper& instance();

    // conf file path
    bool init(std::string cfgfile);

    // 配置更新的调用入口函数
    int  update_runtime_conf();
    int  register_conf_callback(ConfUpdateCallable func);

    std::shared_ptr<libconfig::Config> get_conf() {

        // try update new conf first
        load_conf_file();

        std::lock_guard<std::mutex> lock(lock_);
        return conf_ptr_;
    }

    // 模板函数，方便快速简洁获取配置
    // 这边保证conf_ptr_始终是可用的，否则整个系统初始化失败
    template <typename T>
    bool get_conf_value(const std::string& key, T& t) {

        if (conf_update_time_ < ::time(NULL) - 10*60 ) { // 超过10min，重新读取配置文件

            log_debug("reloading config file, last update interval was %ld secs",
                              ::time(NULL) - conf_update_time_);

            auto conf = load_conf_file();
            if (!conf) {
                log_err("load config file %s failed.", cfgfile_.c_str());
                log_err("try return old staged value.");
            } else {
                std::lock_guard<std::mutex> lock(lock_);
                std::swap(conf, conf_ptr_);
                conf_update_time_ = ::time(NULL);
            }
        }

        std::lock_guard<std::mutex> lock(lock_);
        return ConfUtil::conf_value(*conf_ptr_, key, t);
    }


private:

    std::shared_ptr<libconfig::Config> load_conf_file() {

        std::shared_ptr<libconfig::Config> conf = std::make_shared<libconfig::Config>();
        if (!conf) {
            log_err("create libconfig::Config instance failed!");
            return conf; // nullptr
        }

        try {
            conf->readFile(cfgfile_.c_str());
        } catch(libconfig::FileIOException &fioex) {
            log_err("I/O error while reading file: %s.", cfgfile_.c_str());
            conf.reset();
        } catch(libconfig::ParseException &pex) {
            log_err("Parse error at %d - %s", pex.getLine(), pex.getError());
            conf.reset();
        }

        return conf;
    }

    ConfHelper():
        cfgfile_(), 
        conf_ptr_(), 
        in_process_(false) {
    }

    ~ConfHelper(){}

private:
    std::string cfgfile_;
    std::shared_ptr<libconfig::Config> conf_ptr_;
    time_t conf_update_time_;

    bool in_process_;
    std::mutex lock_;
    std::vector<ConfUpdateCallable> calls_;
};

} // end namespace tzrpc

#endif // __SCAFFOLD_CONF_HELPER__
