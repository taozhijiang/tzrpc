#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <message/ProtoBuf.h>
#include <scaffold/Setting.h>

#include <Protocol/Common.h>

#include <Client/include/RpcClient.h>
#include <Protocol/gen-cpp/XtraTask.pb.h>

using namespace tzrpc;
using namespace tzrpc_client;

TEST(XtraTaskTimeoutTest, TimeoutTest) {


    std::string cfgFile = "tzrpc.conf";
    auto setting_ptr_ = std::make_shared<roo::Setting>();
    ASSERT_THAT(setting_ptr_ && setting_ptr_->init(cfgFile), Eq(true));

    auto setting_ptr = setting_ptr_->get_setting();
    ASSERT_TRUE(setting_ptr);


    std::string addr_ip;
    int         bind_port;
    setting_ptr->lookupValue("rpc.client.serv_addr", addr_ip);
    setting_ptr->lookupValue("rpc.client.serv_port", bind_port);

    RpcClient client(addr_ip, bind_port);

    time_t start_time = ::time(NULL);
    std::string mar_str;
    std::string str;

    XtraTask::XtraReadOps::Request request;
    request.mutable_timeout()->set_timeout(3);
    ASSERT_TRUE(roo::ProtoBuf::marshalling_to_string(request, &mar_str));
    auto status = client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_READ, mar_str, str, 2);
    std::cout << "callout interval: " << ::time(NULL) - start_time << std::endl;
    ASSERT_THAT((::time(NULL) - start_time), Eq(2));
    ASSERT_THAT(static_cast<uint8_t>(status), Eq(static_cast<uint8_t>(RpcClientStatus::RPC_CALL_TIMEOUT))); // RPC 调用超时


    // 重新建立连接了
    start_time = ::time(NULL);
    status = client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_READ, mar_str, str, 4);
    std::cout << "callout interval: " << ::time(NULL) - start_time << std::endl;
    ASSERT_THAT((::time(NULL) - start_time), Eq(3));
    ASSERT_THAT(static_cast<uint8_t>(status), Eq(0)); // RPC 调用正常
}


