/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */


#ifndef __SCAFFOLD_STATUS__
#define __SCAFFOLD_STATUS__

// 各个模块可以上报自己的状态，以json的格式输出来，然后这个模块进行数据汇总
// 内部模块采用 internal/status进行状态的展示

#include <mutex>
#include <map>
#include <functional>

#include <string>
#include <sstream>

#include <Utils/Log.h>


namespace tzrpc {

// 配置动态更新回调函数接口类型
// 注意keyOut的构造，输出的时候按照这个排序

typedef std::function<int (std::string& moduleName, std::string& keyName, std::string& valueOut)> StatusCallable;


class Status {

public:
    static Status& instance();

    int register_status_callback(const std::string& name, StatusCallable func);
    int collect_status(std::string& output);

private:
    Status():
        lock_(),
        calls_() {
    }

    ~Status()
        {
    }

private:
    std::mutex lock_;
    std::map<std::string, StatusCallable> calls_;
};



} // end namespace tzrpc

#endif // __SCAFFOLD_STATUS__
