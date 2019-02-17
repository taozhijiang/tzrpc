#include <xtra_rhel6.h>

#include <RPC/RpcInstance.h>
#include <RPC/Executor.h>

#include <RPC/Dispatcher.h>
#include <Scaffold/Status.h>

namespace tzrpc {


bool Executor::init() override {

    conf_ = service_impl_->get_executor_conf();

    SAFE_ASSERT(conf_.exec_thread_number_ > 0);
    if (!executor_threads_.init_threads(
            std::bind(&Executor::executor_service_run, this, std::placeholders::_1), conf_.exec_thread_number_)) {
        log_err("executor_service_run init task failed!");
        return false;
    }

    if (conf_.exec_thread_number_hard_ > conf_.exec_thread_number_ &&
        conf_.exec_thread_step_queue_size_ > 0)
    {
        threads_adjust_timer_.reset(new steady_timer (Dispatcher::instance().get_io_service()));
        if (!threads_adjust_timer_) {
            log_err("create thread adjust timer failed.");
            return false;
        }

        log_debug("we will support thread adjust for %s, with param %d:%d",
                  instance_name().c_str(),
                  conf_.exec_thread_number_hard_, conf_.exec_thread_step_queue_size_);
        threads_adjust_timer_->expires_from_now(boost::chrono::seconds(1));
        threads_adjust_timer_->async_wait(
                    std::bind(&Executor::executor_threads_adjust, this));
    }

    Status::instance().register_status_callback(
                "executor_" + instance_name(),
                std::bind(&Executor::module_status, shared_from_this(),
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


    return true;
}

void Executor::executor_threads_adjust() {

    ExecutorConf conf {};

    {
        std::lock_guard<std::mutex> lock(conf_lock_);
        conf = conf_;
    }

    SAFE_ASSERT(conf.exec_thread_step_queue_size_ > 0);

    // 进行检查，看是否需要伸缩线程池
    int expect_thread = conf.exec_thread_number_;

    int queueSize = rpc_queue_.SIZE();
    if (queueSize > conf.exec_thread_step_queue_size_) {
        expect_thread = queueSize / conf.exec_thread_step_queue_size_;
    }
    if (expect_thread > conf.exec_thread_number_hard_) {
        expect_thread = conf.exec_thread_number_hard_;
    }

    executor_threads_.resize_threads(expect_thread);


    threads_adjust_timer_->expires_from_now(boost::chrono::seconds(1));
    threads_adjust_timer_->async_wait(
            std::bind(&Executor::executor_threads_adjust, shared_from_this()));

}


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
    }

    ptr->status_ = ThreadStatus::kDead;
    log_info("io_service thread %#lx is about to terminate ... ", (long)pthread_self());

    return;

}


int Executor::module_status(std::string& strModule, std::string& strKey, std::string& strValue) {

    strModule = "tzrpc";
    strKey = "executor_" + instance_name();

    std::stringstream ss;

    ss << "\t" << "instance_name: " << instance_name() << std::endl;
    ss << "\t" << "exec_thread_number: " << conf_.exec_thread_number_ << std::endl;
    ss << "\t" << "exec_thread_number_hard(maxium): " << conf_.exec_thread_number_hard_ << std::endl;
    ss << "\t" << "exec_thread_step_queue_size: " << conf_.exec_thread_step_queue_size_ << std::endl;

    ss << "\t" << std::endl;

    ss << "\t" << "current_thread_number: " << executor_threads_.get_pool_size() << std::endl;
    ss << "\t" << "current_queue_size: " << rpc_queue_.SIZE() << std::endl;

    std::string nullModule;
    std::string subKey;
    std::string subValue;
    service_impl_->module_status(nullModule, subKey, subValue);

    // collect
    strValue = ss.str() + subValue;

    return 0;
}


int Executor::update_runtime_conf(const libconfig::Config& conf) {

    int ret = service_impl_->update_runtime_conf(conf);

    // 如果返回0，表示配置文件已经正确解析了，同时ExecutorConf也重新初始化了
    if (ret == 0) {
        log_notice("update ExecutorConf for host %s", instance_name().c_str());

        std::lock_guard<std::mutex> lock(conf_lock_);
        conf_ = service_impl_->get_executor_conf();
    }

    return ret;
}

} // end tzrpc
