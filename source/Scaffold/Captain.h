/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#ifndef __SCAFFOLD_CAPTAIN_H__
#define __SCAFFOLD_CAPTAIN_H__

#include <string>
#include <map>
#include <vector>


namespace tzrpc {


class NetServer;

class Captain {
public:
    static Captain& instance();

public:
    bool init(const std::string& cfgFile);

    bool service_joinall();
    bool service_graceful();
    void service_terminate();

private:
    Captain();

    ~Captain() {
        // Singleton should not destoried normally,
        // if happens, just terminate quickly
        ::exit(0);
    }


    bool initialized_;

public:

    std::shared_ptr<NetServer> net_server_ptr_;
};

} // end namespace tzrpc


#endif //__SCAFFOLD_CAPTAIN_H__
