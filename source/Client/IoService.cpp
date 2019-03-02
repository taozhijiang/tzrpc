/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#include <Client/IoService.h>

namespace tzrpc_client {

IoService& IoService::instance() {
    static IoService helper {};

    return helper;
}


} // end namespace tzrpc_client
