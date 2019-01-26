#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/Message.h>
#include <Core/Buffer.h>
#include <Scaffold/Setting.h>

#include <xtra_asio.h>
using namespace boost::asio;

using namespace tzrpc;

TEST(ClientSmokeTest, ClientConnTest) {

    std::string msg_str("nicol, taoz");

    struct Header header {};
    header.magic = 0x746b;
    header.version = 0x1;
    header.message_len = msg_str.size();

    struct tzrpc::Message msg {};
    msg.header_ = header;
    msg.playload_ = msg_str;


    // TCP client
    boost::asio::io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), setting.bind_port_));

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
}


