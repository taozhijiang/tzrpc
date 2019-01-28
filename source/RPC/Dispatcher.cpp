#include <RPC/Dispatcher.h>

namespace tzrpc {

Dispatcher& Dispatcher::instance() {
    static Dispatcher dispatcher;
    return dispatcher;
}

} // tzrpc

