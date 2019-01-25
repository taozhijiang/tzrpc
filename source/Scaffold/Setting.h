#ifndef __SCAFFOLD_SETTING_H__
#define __SCAFFOLD_SETTING_H__

#include <string>

namespace tzrpc {

struct Settings {

    std::string version_;

    int         log_level_;

    std::string bind_ip_;
    int         bind_port_;

    int         io_thread_pool_size_;
    int         ops_cancel_time_out_;

    int         max_msg_size_;  // 最大消息负载(不包含头部)

};



bool sys_config_init(const std::string& config_file);

extern Settings setting;


} // end tzrpc

#endif // __SCAFFOLD_SETTING_H__
