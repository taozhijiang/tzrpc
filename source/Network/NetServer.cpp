#include <xtra_rhel6.h>

#include <Network/NetServer.h>
#include <Network/TcpConnAsync.h>

namespace tzrpc {

// accept stuffs
void NetServer::do_accept() {

    SocketPtr sock_ptr(new ip::tcp::socket(io_service_));
    acceptor_->async_accept(*sock_ptr,
                       std::bind(&NetServer::accept_handler, this,
                                   std::placeholders::_1, sock_ptr));
}


void NetServer::accept_handler(const boost::system::error_code& ec, SocketPtr sock_ptr) {

    do {

        if (ec) {
            log_err("error during accept with %d, %s", ec.value(), ec.message().c_str());
            break;
        }

        boost::system::error_code ignore_ec;
        auto remote = sock_ptr->remote_endpoint(ignore_ec);
        if (ignore_ec) {
            log_err("get remote info failed:%d, %s", ignore_ec.value(), ignore_ec.message().c_str());
            break;
        }

        std::string remote_ip = remote.address().to_string(ignore_ec);
        log_debug("remote Client Info: %s:%d", remote_ip.c_str(), remote.port());

        TcpConnAsyncPtr new_conn = std::make_shared<TcpConnAsync>(sock_ptr, *this);
        new_conn->start();

    } while (0);

    // 再次启动接收异步请求
    do_accept();
}


void NetServer::io_service_run(ThreadObjPtr ptr) {

    while (true) {

        if (unlikely(ptr->status_ == ThreadStatus::kTerminating)) {
            log_err("thread %#lx is about to terminating...", (long)pthread_self());
            break;
        }

        // 线程启动
        if (unlikely(ptr->status_ == ThreadStatus::kSuspend)) {
            ::usleep(1*1000*1000);
            continue;
        }

        log_alert("io_service thread %#lx about to loop...", (long)pthread_self());
        boost::system::error_code ec;
        io_service_.run(ec);

        if (ec){
            log_err("io_service stopped...");
            break;
        }
    }

    ptr->status_ = ThreadStatus::kDead;
    log_info("io_service thread %#lx is about to terminate ... ", (long)pthread_self());

    return;
}


} // end tzrpc
