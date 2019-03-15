/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __CLIENT_LOG_CLIENT_H__
#define __CLIENT_LOG_CLIENT_H__

#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <cstring>
#include <cstddef>

#include <algorithm>

#include <boost/current_function.hpp>

// LOG_EMERG   0   system is unusable
// LOG_ALERT   1   action must be taken immediately
// LOG_CRIT    2   critical conditions
// LOG_ERR     3   error conditions
// LOG_WARNING 4   warning conditions
// LOG_NOTICE  5   normal, but significant, condition
// LOG_INFO    6   informational message
// LOG_DEBUG   7   debug-level message


// man 3 syslog
#include <syslog.h>


typedef void(* CP_log_store_func_t)(int priority, const char *format, ...);

namespace tzrpc_client {

bool log_init(int log_level);
void log_close();
void log_api(int priority, const char *file, int line, const char *func, const char *msg, ...)
    __attribute__((format(printf, 5, 6)));

#define log_emerg(...)   log_api( LOG_EMERG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_alert(...)   log_api( LOG_ALERT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_crit(...)    log_api( LOG_CRIT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_err(...)     log_api( LOG_ERR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_warning(...) log_api( LOG_WARNING, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_notice(...)  log_api( LOG_NOTICE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_info(...)    log_api( LOG_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_debug(...)   log_api( LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

extern CP_log_store_func_t checkpoint_log_store_func_impl_;
void set_checkpoint_log_store_func(CP_log_store_func_t func);


} // end namespace tzrpc_client



#endif // __CLIENT_LOG_CLIENT_H__
