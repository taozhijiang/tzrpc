#include <gmock/gmock.h>
#include <string>

using namespace ::testing;

#include <Scaffold/Setting.h>


TEST(LibConfigTest, SysConfigInitVefifyTest) {

    std::string cfgFile = "pbi_bankpay_lookup_service.conf";
    bool b_ret = sys_config_init(cfgFile);
    ASSERT_TRUE(b_ret);

    ASSERT_THAT(setting.version_, Eq("1.0.0"));
    ASSERT_THAT(setting.log_level_, Eq(7));

    ASSERT_THAT(setting.bind_ip_, Eq("0.0.0.0"));
    ASSERT_THAT(setting.bind_port_, Eq(8435));

    ASSERT_THAT(setting.io_thread_pool_size_, Eq(5));
}
