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

TEST(XtraTaskRequestCheckTest, InvalidRequestTest) {

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

    std::string msg_str("nicol, taoz from test rpc");

    std::string ping_str("<<<ping>>>");
    std::string mar_str;
    std::string str;
    XtraTask::XtraReadOps::Request request;
    request.mutable_ping()->set_msg(ping_str);
    ASSERT_TRUE(roo::ProtoBuf::marshalling_to_string(request, &mar_str));
    auto status = client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_READ, mar_str, str);
    ASSERT_THAT(static_cast<uint8_t>(status), Eq(0));

    XtraTask::XtraReadOps::Response response;
    ASSERT_TRUE(roo::ProtoBuf::unmarshalling_from_string(str, &response));
    ASSERT_THAT(response.IsInitialized(), Eq(true));

    ASSERT_TRUE(response.has_code() && (response.code() == 0) );
    std::string pong_str = response.ping().msg();

    std::cout << "pong: " << pong_str << std::endl;
}


