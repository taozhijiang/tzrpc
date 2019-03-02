/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __RPC_CLIENT_STATUS_H__
#define __RPC_CLIENT_STATUS_H__


#include <cstdint>

namespace tzrpc_client {

// 服务端的错误码应该包含客户端传递过来的错误码，同时
// 还应该包含其他原因导致的服务错误的状态

enum class RpcClientStatus: uint8_t {


    OK = 0,

    /**
     * An error specific to the particular service. The format of the remainder
     * of the message is specific to the particular service.
     * 暂未使用
     */
    SERVICE_SPECIFIC_ERROR = 1,

    INVALID_VERSION = 2,

    INVALID_SERVICE = 3,

    INVALID_REQUEST = 4,

    // 以上部分是和服务端相互兼容的

    // 未发送网络请求，客户端可以考虑重发
    NETWORK_BEFORE_ERROR = 5,
    NETWORK_CONNECT_ERROR = 6,

    // 实际可能操作了网络部分
    NETWORK_SEND_ERROR = 7,
    NETWORK_RECV_ERROR = 8,

    RPC_CALL_TIMEOUT = 9,

    RECV_FORMAT_ERROR = 10,      // 接收的报文解析错误

};



} // end namespace tzrpc_client

#endif // __RPC_CLIENT_STATUS_H__
