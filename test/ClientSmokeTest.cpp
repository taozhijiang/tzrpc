#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/Message.h>
#include <Core/Buffer.h>
#include <Scaffold/Setting.h>

#include <Protocol/Common.h>

#include <RPC/RpcClient.h>
#include <Protocol/gen-cpp/XtraTask.pb.h>

#include <xtra_asio.h>
using namespace boost::asio;

using namespace tzrpc;

TEST(ClientSmokeTest, ClientConnTest) {

    std::string msg_str("nicol, taoz from test");

    struct Header header {};
    header.magic = 0x746b;
    header.version = 0x1;
    header.length = msg_str.size();

    struct tzrpc::Message msg {};
    msg.header_ = header;
    msg.payload_ = msg_str;


    // TCP client
    boost::asio::io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(boost::asio::ip::address::from_string(setting.serv_ip_), setting.bind_port_));

#if 0

    // Send
    boost::system::error_code error;
    Buffer buffer(msg);
    std::string to_send;
    buffer.retrive(to_send, 1024);
    boost::asio::write(socket, boost::asio::buffer(to_send), error);

    bool result = error;
    ASSERT_THAT(result, Eq(false)); // OK here

    // Recv
    boost::asio::streambuf receive_buffer;
    boost::asio::read_until(socket, receive_buffer, "\r\n", error);
    result = error && error != boost::asio::error::eof;
    ASSERT_THAT(result, Eq(false)); // OK here

    const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data() + sizeof(Header));
    std::cout << "RECV: " << data << std::endl;
#endif

}


TEST(ClientSmokeTest, RpcClientTest) {

    std::string msg_str("nicol, taoz from test rpc");

    RpcClient client(setting.serv_ip_, setting.bind_port_);
    log_debug("create client with %s:%u", setting.serv_ip_.c_str(), setting.bind_port_);

    std::string rsp_str1;
    client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_GET, msg_str, rsp_str1);
    log_debug("rpc_call_result: %s", rsp_str1.c_str());
    ASSERT_THAT(rsp_str1.size(), Gt(0));

    std::string rsp_str2;
    client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_SET, msg_str, rsp_str2);
    log_debug("rpc_call_result: %s", rsp_str2.c_str());
    ASSERT_THAT(rsp_str2.size(), Gt(0));
}


