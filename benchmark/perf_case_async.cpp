#include <unistd.h>

#include <string>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <cstdlib>


#include <Client/include/RpcClient.h>

#include <Client/Common.h>
#include <message/ProtoBuf.h>
#include <Client/XtraTask.pb.h>

using namespace tzrpc_client;


//
// 异步客户端请求的性能
//

volatile bool start = false;
volatile bool stop  = false;

time_t            start_time = 0;
time_t            stop_time  = 0;
volatile uint64_t count = 0;
volatile uint64_t h_count = 0;

struct RpcClientSetting setting {};

extern char * program_invocation_short_name;
static void usage() {
    std::stringstream ss;

    ss << program_invocation_short_name << " [thread_num] " << std::endl;
    ss << std::endl;

    std::cerr << ss.str();
}

static std::string generate_random_str() {

    std::stringstream ss;
    ss << "message with random [" << ::random() << "] include";
    return ss.str();
}

int async_handler(RpcClientStatus status, const std::string& rsp) {

    if(status != RpcClientStatus::OK) {
        std::cout << "async_handler process failed." << std::endl;
        stop = true;
    }

    ++ h_count;
    return 0;
}

void* perf_run(void* x_void_ptr) {

    while(!start)
        ::usleep(1);

    RpcClient client(setting.serv_addr_, setting.serv_port_, async_handler);

    while(!stop) {

        std::string mar_str;
        std::string echo_str(generate_random_str());
        tzrpc::XtraTask::XtraReadOps::Request request;

        request.mutable_echo()->set_msg(echo_str);
        if(!roo::ProtoBuf::marshalling_to_string(request, &mar_str)) {
            std::cerr << "marshalling message failed." << std::endl;
            stop = true;
            continue;
        }

        std::string resp_str;
        auto status = client.call_RPC(tzrpc::ServiceID::XTRA_TASK_SERVICE,
                                      tzrpc::XtraTask::OpCode::CMD_READ,
                                      mar_str);
        if(status != RpcClientStatus::OK) {
            std::cerr << "call failed, return code [" << static_cast<uint8_t>(status) << "]" << std::endl;
            stop = true;
        }

        // increment success case
        ++ count;

        // 异步的请求性能太高，这边达到30000就不再发了
        if (count >= 30000) {
            stop_time = ::time(NULL);
            ::sleep(10);  // 等待服务端全部处理完
            std::cout << "break..." << std::endl;
            break;
        }
    }

    return NULL;
}

int main(int argc, char* argv[]) {

    int thread_num = 0;
    if (argc < 2 || (thread_num = ::atoi(argv[1])) <= 0) {
        usage();
        return 0;
    }

    setting.serv_addr_ = "127.0.0.1";
    setting.serv_port_ = 8434;

    std::vector<pthread_t> tids( thread_num,  0);
    for(size_t i=0; i<tids.size(); ++i) {
        pthread_create(&tids[i], NULL, perf_run, NULL);
        std::cerr << "starting thread with id: " << tids[i] << std::endl;
    }

    ::sleep(2);
    std::cerr << "begin to test, when see break, press any to stop." << std::endl;
    start_time = ::time(NULL);
    start = true;

    int ch = getchar();
    stop = true;
    if (stop_time == 0)
        stop_time = ::time(NULL);

    uint64_t count_per_sec = count / ( stop_time - start_time);
    fprintf(stderr, "total count %ld, time: %ld, perf: %ld tps\n", count, stop_time - start_time, count_per_sec);
    fprintf(stderr, "handle count %ld\n", h_count);

    for(size_t i=0; i<tids.size(); ++i) {
        pthread_join(tids[i], NULL);
        std::cerr<< "joining " << tids[i] << std::endl;
    }

    std::cerr << "done" << std::endl;

    return 0;
}
