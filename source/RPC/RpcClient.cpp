
#include <RPC/RpcClient.h>

namespace tzrpc {


static boost::asio::io_service IO_SERVICE;


bool RpcClient::send_rpc_message(const RpcRequestMessage& rpc_request_message) {
    Message net_msg(rpc_request_message.net_str());
    return conn_->send_net_message(net_msg);
}


bool RpcClient::recv_rpc_message(RpcResponseMessage& rpc_response_message) {

    Message net_msg;
    if (!conn_->recv_net_message(net_msg)) {
        return false;
    }

    // parse
    return RpcResponseMessageParse(net_msg.payload_, rpc_response_message);
}

int RpcClient::call_RPC(uint16_t service_id, uint16_t opcode, const std::string& playload, std::string& respload) {

    if (!conn_) {

        std::shared_ptr<ip::tcp::socket> socket_ptr = std::make_shared<ip::tcp::socket>(IO_SERVICE);
        socket_ptr->connect(ip::tcp::endpoint(ip::address::from_string(addr_ip_), addr_port_));

        conn_.reset(new TcpConnSync(socket_ptr, IO_SERVICE));
        if (!conn_) {
            log_err("create socket %s:%u failed.",  addr_ip_.c_str(), addr_port_);
            return -1;
        }

        log_debug("create new socket %s:%u ok.",  addr_ip_.c_str(), addr_port_);
    }

    service_id_ = service_id;
    opcode_ = opcode;

    RpcRequestMessage rpc_request_message(service_id, opcode, playload);
    log_debug("rpc_request_message: %s",  rpc_request_message.dump().c_str());
    send_rpc_message(rpc_request_message);

    // 接收报文
    RpcResponseMessage rpc_response_message;
    recv_rpc_message(rpc_response_message);


    respload = rpc_response_message.payload_;

    return 0;
}


} // end tzrpc
