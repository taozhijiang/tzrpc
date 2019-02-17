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

TEST(XtraTaskRequestCheckTest, InvalidRequestTest) {

    std::string cfgFile = "tzrpc.conf";

    bool b_ret = ConfHelper::instance().init(cfgFile);
    ASSERT_TRUE(b_ret);

    auto conf_ptr = ConfHelper::instance().get_conf();
    ASSERT_TRUE(conf_ptr);

    std::string addr_ip;
    int         bind_port;
    ConfUtil::conf_value(*conf_ptr, "network.bind_addr", addr_ip);
    ConfUtil::conf_value(*conf_ptr, "network.listen_port", bind_port);

    RpcClient client(addr_ip, bind_port);

    std::string msg_str("nicol, taoz from test rpc");

    std::string ping_str("<<<ping>>>");
    std::string mar_str;
    std::string str;
    XtraTask::XtraReadOps::Request request;
    request.mutable_ping()->set_msg(ping_str);
    ASSERT_TRUE(ProtoBuf::marshalling_to_string(request, &mar_str));
    auto status = client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_READ, mar_str, str);
    ASSERT_THAT(static_cast<uint8_t>(status), Eq(0));

    XtraTask::XtraReadOps::Response response;
    ASSERT_TRUE(ProtoBuf::unmarshalling_from_string(str, &response));
    ASSERT_THAT(response.IsInitialized(), Eq(true));

    ASSERT_TRUE(response.has_code() && (response.code() == 0) );
    std::string pong_str = response.ping().msg();

    std::cout << "pong: " << pong_str << std::endl;
}


