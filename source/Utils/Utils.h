/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <sys/time.h>

#include <xtra_rhel.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>


namespace tzrpc {

void backtrace_init();

template <typename T>
std::string convert_to_string(const T& arg) {
    try {
        return boost::lexical_cast<std::string>(arg);
    }
    catch(boost::bad_lexical_cast& e) {
        return "";
    }
}



struct RAII_PERF_COUNTER {

    RAII_PERF_COUNTER(const std::string env, const std::string key):
        env_(env),
        key_(key) {
        ::gettimeofday(&start_, NULL);
    }

    ~RAII_PERF_COUNTER() {
        struct timeval end {};
        ::gettimeofday(&end, NULL);

        int64_t time_ms = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) / 1000; // ms
        int64_t time_us = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) % 1000; // us

        display_info(env_, time_ms, time_us);
    }

    // 禁止拷贝
    RAII_PERF_COUNTER(const RAII_PERF_COUNTER&) = delete;
    RAII_PERF_COUNTER& operator=(const RAII_PERF_COUNTER&) = delete;

    void display_info(const std::string& env, int64_t time_ms, int64_t time_us);

private:
    std::string env_;
    std::string key_;
    struct timeval start_;
};

} // end namespace tzrpc

#define PUT_RAII_PERF_COUNTER(T) \
    tzrpc::RAII_PERF_COUNTER __RAII_PERF_CHECKER_##T( boost::str(boost::format("%s(%ld):%s") % __FILE__%__LINE__%BOOST_CURRENT_FUNCTION), #T ); \
    (void) __RAII_PERF_CHECKER_##T


#endif // __UTILS_UTILS_H__
