/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <sys/time.h>

#include <xtra_rhel6.h>

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



struct COUNT_FUNC_PERF: public boost::noncopyable {

    COUNT_FUNC_PERF(const std::string env, const std::string key):
    env_(env), key_(key) {
        ::gettimeofday(&start_, NULL);
    }

    ~COUNT_FUNC_PERF() {
        struct timeval end;
        ::gettimeofday(&end, NULL);

        int64_t time_ms = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) / 1000; // ms
        int64_t time_us = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) % 1000; // us

        display_info(env_, time_ms, time_us);
    }

    void display_info(const std::string& env, int64_t time_ms, int64_t time_us);

private:
    std::string env_;
    std::string key_;
    struct timeval start_;
};

} // end namespace tzrpc

#define PUT_COUNT_FUNC_PERF(T) \
                tzrpc::COUNT_FUNC_PERF PERF_CHECKER_##T( boost::str(boost::format("%s(%ld):%s") % __FILE__%__LINE__%BOOST_CURRENT_FUNCTION), #T ); \
                (void) PERF_CHECKER_##T


#endif // __UTILS_UTILS_H__
