#ifndef __SCAFFOLD_MANAGER_H__
#define __SCAFFOLD_MANAGER_H__

#include <string>
#include <map>
#include <vector>


namespace tzrpc {


class NetServer;

class Manager {
public:
    static Manager& instance();

public:
    bool init();

    bool service_joinall();
    bool service_graceful();
    void service_terminate();

private:
    Manager();

    ~Manager() {
        // Singleton should not destoried normally,
        // if happens, just terminate quickly
        ::exit(0);
    }


    bool initialized_;

public:

    std::shared_ptr<NetServer> net_server_ptr_;
};

} // end tzrpc


#endif //__SCAFFOLD_MANAGER_H__
