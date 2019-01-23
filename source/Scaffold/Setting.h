#ifndef __SCAFFOLD_SETTING_H__
#define __SCAFFOLD_SETTING_H__

#include <string>

struct Settings {

    std::string version_;

    int         log_level_;

    std::string bind_ip_;
    int         bind_port_;

    int         io_thread_pool_size_;

};



bool sys_config_init(const std::string& config_file);

extern Settings setting;

#endif // __SCAFFOLD_SETTING_H__
