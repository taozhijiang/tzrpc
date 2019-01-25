#include <iostream>
#include "gtest/gtest.h"

#include <xtra_rhel6.h>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/Setting.h>
#include <Scaffold/Manager.h>

using namespace tzrpc;

int main(int argc, char** argv) {

    testing::InitGoogleTest(&argc, argv);

    // do initialize

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(1);
    }

    const std::string cfgFile = "../pbi_bankpay_lookup_service.conf";
    if (!sys_config_init(cfgFile)) {
        std::cout << "handle system configure " << cfgFile <<" failed!" << std::endl;
        return -1;
    }

   return RUN_ALL_TESTS();
}



namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}
} // end boost
