/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __UTILS_STR_UTIL_H__
#define __UTILS_STR_UTIL_H__

#include <libconfig.h++>


#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <Utils/Log.h>

// 类静态函数可以直接将函数定义丢在头文件中

namespace tzrpc {

struct StrUtil {

    static size_t trim_whitespace(std::string& str) {

        size_t index = 0;
        size_t orig = str.size();

        // trim left whitespace
        for (index = 0; index < str.size() && isspace(str[index]); ++index)
            /* do nothing*/;
        str.erase(0, index);

        // trim right whitespace
        for (index = str.size(); index > 0 && isspace(str[index - 1]); --index)
            /* do nothing*/;
        str.erase(index);

        return orig - str.size();
    }


    template <typename T>
    static std::string convert_to_string(const T& arg) {
        try {
            return boost::lexical_cast<std::string>(arg);
        }
        catch(boost::bad_lexical_cast& e) {
            return "";
        }
    }


    static std::string pure_uri_path(std::string uri) {  // copy
        uri = boost::algorithm::trim_copy(boost::to_lower_copy(uri));
        while (uri[uri.size()-1] == '/' && uri.size() > 1)  // 全部的小写字母，去除尾部
            uri = uri.substr(0, uri.size()-1);

        return uri;
    }

    static std::string trim_lowcase(std::string str) {  // copy
        return boost::algorithm::trim_copy(boost::to_lower_copy(str));
    }

    // 删除host尾部的端口号
    static std::string drop_host_port(std::string host) {  // copy
        host = boost::algorithm::trim_copy(boost::to_lower_copy(host));
        auto pos = host.find(':');
        if (pos != std::string::npos) {
            host.erase(pos);
        }
        return host;
    }
};



struct ConfUtil {

    template<typename T>
    static bool conf_value(const libconfig::Config& conf, const std::string& key, T& value) {
        T t{};  // default value
        if (!conf.lookupValue(key, value)) {
            log_err("conf %s not found, using construct default value.", key.c_str());
            value = t;
            return false;
        }
        return true;
    }

    template<typename T>
    static bool conf_value(const libconfig::Config& conf, const std::string& key, T& value, const T& def_value) {
        if (!conf.lookupValue(key, value)) {
            log_err("conf %s not found, using provided default value.", key.c_str());
            value = def_value;
            return false;
        }
        return true;
    }

    template<typename T>
    static bool conf_value(const libconfig::Setting& conf, const std::string& key, T& value) {
        T t{};  // default value
        if (!conf.lookupValue(key, value)) {
            log_err("conf %s not found, using construct default value.", key.c_str());
            value = t;
            return false;
        }
        return true;
    }

    template<typename T>
    static bool conf_value(const libconfig::Setting& conf, const std::string& key, T& value, const T& def_value) {
        if (!conf.lookupValue(key, value)) {
            log_err("conf %s not found, using provided default value.", key.c_str());
            value = def_value;
            return false;
        }
        return true;
    }

};



class UriRegex: public boost::regex {
public:
    explicit UriRegex(const std::string& regexStr) :
        boost::regex(regexStr), str_(regexStr) {
    }

    std::string str() const {
        return str_;
    }

private:
    std::string str_;
};


} // end namespace tzrpc

#endif // __UTILS_STR_UTIL_H__
