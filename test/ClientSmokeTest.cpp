#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/Message.h>
#include <Core/Buffer.h>

#include <scaffold/Setting.h>
#include <other/Log.h>
using roo::log_api;

#include <Protocol/Common.h>

#include <Client/include/RpcClient.h>
#include <Protocol/gen-cpp/XtraTask.pb.h>

#include <boost/asio.hpp>
using namespace boost::asio;

using namespace tzrpc;
using namespace tzrpc_client;

TEST(ClientSmokeTest, ClientConnTest) {


    std::string cfgFile = "tzrpc.conf";
    auto setting_ptr_ = std::make_shared<roo::Setting>();
    ASSERT_THAT(setting_ptr_ && setting_ptr_->init(cfgFile), Eq(true));

    auto setting_ptr = setting_ptr_->get_setting();
    ASSERT_TRUE(setting_ptr);

    std::string addr_ip;
    int         bind_port;
    setting_ptr->lookupValue("rpc.client.serv_addr", addr_ip);
    setting_ptr->lookupValue("rpc.client.serv_port", bind_port);

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
    socket.connect(ip::tcp::endpoint(boost::asio::ip::address::from_string(addr_ip), bind_port));

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

    std::string cfgFile = "tzrpc.conf";
    auto setting_ptr_ = std::make_shared<roo::Setting>();
    ASSERT_THAT(setting_ptr_ && setting_ptr_->init(cfgFile), Eq(true));

    auto setting_ptr = setting_ptr_->get_setting();
    ASSERT_TRUE(setting_ptr);

    std::string addr_ip;
    int         bind_port;
    setting_ptr->lookupValue("rpc.client.serv_addr", addr_ip);
    setting_ptr->lookupValue("rpc.client.serv_port", bind_port);


    std::string msg_str("nicol, taoz from test rpc");

    RpcClient client(addr_ip, bind_port);
    roo::log_debug("create client with %s:%u", addr_ip.c_str(), bind_port);

    std::string rsp_str1;
    client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_READ, msg_str, rsp_str1);
    roo::log_debug("rpc_call_result: %s", rsp_str1.c_str());
    ASSERT_THAT(rsp_str1.size(), Gt(0));

    std::string rsp_str2;
    client.call_RPC(ServiceID::XTRA_TASK_SERVICE, XtraTask::OpCode::CMD_WRITE, msg_str, rsp_str2);
    roo::log_debug("rpc_call_result: %s", rsp_str2.c_str());
    ASSERT_THAT(rsp_str2.size(), Gt(0));
}


