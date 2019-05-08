#include <gmock/gmock.h>
#include <string>

using namespace ::testing;


#include <scaffold/Setting.h>

TEST(LibConfigTest, SysConfigInitVefifyTest) {

    std::string cfgFile = "tzrpc.conf";
    auto setting_ptr_ = std::make_shared<roo::Setting>();
    ASSERT_THAT(setting_ptr_ && setting_ptr_->init(cfgFile), Eq(true));

    auto setting_ptr = setting_ptr_->get_setting();
    ASSERT_TRUE(setting_ptr);

    std::string s_value;
    int         i_value;

    setting_ptr->lookupValue("version", s_value);
    ASSERT_THAT(s_value, Eq("1.2.0"));

    setting_ptr->lookupValue("log_level", i_value);
    ASSERT_THAT(i_value, Le(7));

}
