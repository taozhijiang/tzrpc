#include <xtra_rhel6.h>

#include <RPC/RpcInstance.h>
#include <RPC/Executor.h>

namespace tzrpc {


void Executor::executor_service_run(ThreadObjPtr ptr) {

    log_alert("executor_service thread %#lx about to loop ...", (long)pthread_self());

    while (true) {

        std::shared_ptr<RpcInstance> rpc_instance {};

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1*1000*1000);
            continue;
        }

        if (!rpc_queue_.POP(rpc_instance, 1000 /*1s*/) || !rpc_instance) {
            continue;
        }

        // execute RPC handler
        service_impl_->handle_RPC(rpc_instance);

//        log_notice("Queue Size: %u", rpc_queue_.SIZE());
    }

    ptr->status_ = ThreadStatus::kDead;
    log_info("io_service thread %#lx is about to terminate ... ", (long)pthread_self());

    return;

}


} // end tzrpc
