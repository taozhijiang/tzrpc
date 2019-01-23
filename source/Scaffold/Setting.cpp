#include <cstdio>
#include <libconfig.h++>

#include <Scaffold/Setting.h>

Settings setting {};

libconfig::Config& get_config_object() {
    static libconfig::Config cfg;
    return cfg;
}

template <typename T>
bool get_config_value(const std::string& key, T& t) {
    return get_config_object().lookupValue(key, t);
}

// this initialized before syslog, so just print out err
bool sys_config_init(const std::string& config_file) {

    libconfig::Config& cfg = get_config_object();

    try {
        cfg.readFile(config_file.c_str());
    } catch(libconfig::FileIOException &fioex) {
        fprintf(stderr, "I/O error while reading file.");
        return false;
    } catch(libconfig::ParseException &pex) {
        fprintf(stderr, "Parse error at %d - %s", pex.getLine(), pex.getError());
        return false;
    }

    // load config.properties to Settings object
    get_config_value("version", setting.version_);

    get_config_value("log_level", setting.log_level_);
    if (setting.log_level_ < 0 || setting.log_level_ > 7) {
        fprintf(stderr, "Invalid log_level: %d", setting.log_level_);
        return false;
    }

    get_config_value("network.bind_addr", setting.bind_ip_);
    get_config_value("network.listen_port", setting.bind_port_);
    if (setting.bind_ip_.empty() || setting.bind_port_ < 0) {
        fprintf(stderr, "Invalid server network address: %s:%d", setting.bind_ip_.c_str(),  setting.bind_port_);
        return false;
    }

    get_config_value("network.io_thread_pool_size", setting.io_thread_pool_size_);
    if (setting.io_thread_pool_size_ < 0 || setting.io_thread_pool_size_ > 10) {
        fprintf(stderr, "Invalid io_thread_pool_size: %d, reset to default 4");
        setting.io_thread_pool_size_ = 4;
    }

    return true;
}


