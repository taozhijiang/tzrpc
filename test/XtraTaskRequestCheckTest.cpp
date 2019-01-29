#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/ProtoBuf.h>
#include <Scaffold/Setting.h>

#include <Protocol/Common.h>

#include <RPC/RpcClient.h>
#include <Protocol/gen-cpp/XtraTask.pb.h>

using namespace boost::asio;

using namespace tzrpc;

TEST(XtraTaskRequestCheckTest, InvalidRequestTest) {

    std::string msg_str("nicol, taoz from test rpc");

    RpcClient client(setting.serv_ip_, setting.bind_port_);

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


