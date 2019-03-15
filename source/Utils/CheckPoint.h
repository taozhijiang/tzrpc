/*-
 * Copyright (c) 2018-2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __UTILS_CHECK_POINT__
#define __UTILS_CHECK_POINT__

#include <string>

#include "Log.h"

namespace tzrpc {

// Log Store

typedef void(* CP_log_store_func_t)(int priority, const char *format, ...);
extern CP_log_store_func_t checkpoint_log_store_func_impl_;
void set_checkpoint_log_store_func(CP_log_store_func_t func);


} // end namespace tzrpc


#endif // __UTILS_CHECK_POINT__
