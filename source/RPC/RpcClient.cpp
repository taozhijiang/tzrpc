
#include <RPC/RpcClient.h>


#include <RPC/RpcRequestMessage.h>
#include <RPC/RpcResponseMessage.h>


namespace tzrpc {


static boost::asio::io_service IO_SERVICE;


bool RpcClient::send_rpc_message(const RpcRequestMessage& rpc_request_message) {
    Message net_msg(rpc_request_message.net_str());
    return conn_->send_net_message(net_msg);
}


bool RpcClient::recv_rpc_message(Message& net_message) {
    return conn_->recv_net_message(net_message);
}

RpcClientStatus RpcClient::call_RPC(uint16_t service_id, uint16_t opcode,
                                    const std::string& payload, std::string& respload) {

    if (!conn_) {

        boost::system::error_code ec;
        std::shared_ptr<ip::tcp::socket> socket_ptr = std::make_shared<ip::tcp::socket>(IO_SERVICE);

        socket_ptr->connect(ip::tcp::endpoint(ip::address::from_string(addr_ip_), addr_port_), ec);
        if (ec) {
            log_err("connect to %s:%u failed with {%d} %s.",
                    addr_ip_.c_str(),  addr_port_, ec.value(), ec.message().c_str() );
            return RpcClientStatus::NETWORK_CONNECT_ERROR;

        }

        conn_.reset(new TcpConnSync(socket_ptr, IO_SERVICE));
        if (!conn_) {
            log_err("create socket %s:%u failed.",  addr_ip_.c_str(), addr_port_);
            return RpcClientStatus::NETWORK_BEFORE_ERROR;
        }

        log_debug("create new socket %s:%u ok.",  addr_ip_.c_str(), addr_port_);
    }

    // 构建请求包
    RpcRequestMessage rpc_request_message(service_id, opcode, payload);
    log_debug("rpc_request_message: %s",  rpc_request_message.dump().c_str());

    // 发送请求报文
    if(!send_rpc_message(rpc_request_message)){
        return RpcClientStatus::NETWORK_SEND_ERROR;
    }

    // 接收报文
    Message net_message;
    if(!recv_rpc_message(net_message)){
        return RpcClientStatus::NETWORK_RECV_ERROR;
    }

    // 解析报文
    RpcResponseMessage rpc_response_message;
    if (!RpcResponseMessageParse(net_message.payload_, rpc_response_message)) {
        return RpcClientStatus::RECV_FORMAT_ERROR;
    }

    // 返回参数校验
    if (rpc_response_message.header_.magic != kRpcHeaderMagic ||
         // rpc_response_message.header_.version != kRpcHeaderVersion ||
        rpc_response_message.header_.service_id != service_id ||
        rpc_response_message.header_.opcode != opcode ) {
        log_err("rpc_response_message header check error: %s", rpc_response_message.header_.dump().c_str());
        return RpcClientStatus::RECV_FORMAT_ERROR;
    }

    // Service Status 校验
    if (rpc_response_message.header_.status != RpcResponseStatus::OK) {
        log_err("ServiceSide status: %u", rpc_response_message.header_.status);
        return static_cast<RpcClientStatus>(static_cast<uint8_t>(rpc_response_message.header_.status));
    }

    respload = rpc_response_message.payload_;
    return RpcClientStatus::OK;
}


} // end tzrpc
