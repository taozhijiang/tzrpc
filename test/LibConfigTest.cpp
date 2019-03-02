#include <gmock/gmock.h>
#include <string>

using namespace ::testing;

#include <Scaffold/ConfHelper.h>

using namespace tzrpc;

TEST(LibConfigTest, SysConfigInitVefifyTest) {

    std::string cfgFile = "tzrpc.conf";

    bool b_ret = ConfHelper::instance().init(cfgFile);
    ASSERT_TRUE(b_ret);

    auto conf_ptr = ConfHelper::instance().get_conf();
    ASSERT_TRUE(conf_ptr);

    std::string s_value;
    int         i_value;

    conf_ptr->lookupValue("version", s_value);
    ASSERT_THAT(s_value, Eq("1.2.0"));

    conf_ptr->lookupValue("log_level", i_value);
    ASSERT_THAT(i_value, Eq(7));

}
