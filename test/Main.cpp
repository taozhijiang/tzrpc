#include <iostream>
#include "gtest/gtest.h"

#include <xtra_rhel.h>

#include <syslog.h>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/ConfHelper.h>

using namespace tzrpc;

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);

    // do initialize

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(1);
    }

    std::string cfgFile = "tzrpc.conf";
    if(!ConfHelper::instance().init(cfgFile)){
        return -1;
    }

    auto conf_ptr = ConfHelper::instance().get_conf();
    if(!conf_ptr) {
        log_err("ConfHelper return null conf pointer, maybe your conf file ill!");
        return -1;
    }

    int log_level = 0;
    conf_ptr->lookupValue("log_level", log_level);
    if (log_level <= 0 || log_level > 7) {
        return -1;
    }

    set_checkpoint_log_store_func(syslog);
    if (!log_init(log_level)) {
        std::cerr << "init syslog failed!" << std::endl;
        return -1;
    }

    log_notice("syslog init success, level: %d", log_level);

   return RUN_ALL_TESTS();
}



namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}
} // end boost
