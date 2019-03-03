#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/ProtoBuf.h>
#include <Scaffold/ConfHelper.h>

#include <Protocol/Common.h>

#include <Client/include/RpcClient.h>
#include <Protocol/gen-cpp/XtraTask.pb.h>

using namespace tzrpc;
using namespace tzrpc_client;

TEST(XtraTaskTimeoutTest, TimeoutTest) {

    std::string cfgFile = "tzrpc.conf";

    bool b_ret = ConfHelper::instance().init(cfgFile);
    ASSERT_TRUE(b_ret);

    auto conf_ptr = ConfHelper::instance().get_conf();
    ASSERT_TRUE(conf_ptr);

    std::string addr_ip;
    int         bind_port;
    conf_ptr->lookupValue("rpc.client.serv_addr", addr_ip);
    conf_ptr->lookupValue("rpc.client.serv_port", bind_port);

    RpcClient client(addr_ip, bind_port);

    time_t start_time = ::time(NULL);
    std::string mar_str;
    std::string str;

    XtraTask::XtraReadOps::Request request;
    request.mutable_timeout()->set_timeout(3);
    ASSERT_TRUE(ProtoBuf::marshalling_to_string(request, &mar_str));
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


