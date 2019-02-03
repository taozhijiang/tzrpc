#include <string>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <cstdlib>


#include <Client/include/RpcClient.h>

#include <Client/include/Common.h>
#include <Client/include/ProtoBuf.h>
#include <Client/include/XtraTask.pb.h>

using namespace tzrpc_client;


//
// 短连接的性能
//

volatile bool start = false;
volatile bool stop  = false;

time_t            start_time = 0;
volatile uint64_t count = 0;

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

void* perf_run(void* x_void_ptr) {
	
    while(!start)
        ::usleep(1);

    RpcClient client(setting.addr_ip_, setting.addr_port_);

    while(!stop) {

        std::string mar_str;
        std::string echo_str(generate_random_str());
        tzrpc::XtraTask::XtraReadOps::Request request;

        request.mutable_echo()->set_msg(echo_str);
        if(!tzrpc::ProtoBuf::marshalling_to_string(request, &mar_str)) {
            std::cerr << "marshalling message failed." << std::endl;
            stop = true;
            continue;
        }

        std::string resp_str;
        auto status = client.call_RPC(tzrpc::ServiceID::XTRA_TASK_SERVICE,
                                      tzrpc::XtraTask::OpCode::CMD_READ,
                                      mar_str, resp_str);
        if(status != RpcClientStatus::OK) {
            std::cerr << "call failed, return code [" << static_cast<uint8_t>(status) << "]" << std::endl;
            stop = true;
            continue;
        }

        tzrpc::XtraTask::XtraReadOps::Response response;
        if(!tzrpc::ProtoBuf::unmarshalling_from_string(resp_str, &response)) {
            std::cerr << "unmarshalling message failed." << std::endl;
            stop = true;
            continue;
        }

        if(!response.has_code() || response.code() != 0 ) {
            std::cerr << "response code check error" << std::endl;
            stop = true;
            continue;
        }

        std::string echo_expect = "echo:" + echo_str;
        std::string echo_back_str = response.echo().msg();
        if (echo_expect != echo_back_str) {
            std::cerr << "content check failed, expect: " << echo_expect << ", but recv: " << echo_back_str << std::endl;
            stop = true;
            continue;
        }

        // increment success case
        ++ count;
    }
}

int main(int argc, char* argv[]) {
	
    int thread_num = 0;
    if (argc < 2 || (thread_num = ::atoi(argv[1])) <= 0) {
        usage();
        return 0;
    }

    setting.addr_ip_ = "127.0.0.1";
    setting.addr_port_ = 8435;

    std::vector<pthread_t> tids( thread_num,  0);
    for(size_t i=0; i<tids.size(); ++i) {
		pthread_create(&tids[i], NULL, perf_run, NULL);
        std::cerr << "starting thread with id: " << tids[i] << std::endl;
	}

    ::sleep(3);
    std::cerr << "begin to test, press any to stop." << std::endl;
    start_time = ::time(NULL);
    start = true;

    int ch = getchar();
    stop = true;
    time_t stop_time = ::time(NULL);

    uint64_t count_per_sec = count / ( stop_time - start_time);
    fprintf(stderr, "total count %ld, time: %ld, perf: %ld tps\n", count, stop_time - start_time, count_per_sec);

    for(size_t i=0; i<tids.size(); ++i) {
		pthread_join(tids[i], NULL);
        std::cerr<< "joining " << tids[i] << std::endl;
	}

    std::cerr << "done" << std::endl;

    return 0;
}
