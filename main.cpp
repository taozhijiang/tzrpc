#include <signal.h>
#include <ctime>

#include <syslog.h>
#include <boost/atomic/atomic.hpp>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/Captain.h>

void init_signal_handle();
void usage();
void show_vcs_info();
int create_process_pid();

char cfgFile[PATH_MAX] {};
bool daemonize = false;

int main(int argc, char* argv[]) {

    strncpy(cfgFile, program_invocation_short_name, strlen(program_invocation_short_name));
    strcat(cfgFile, ".conf");
    int opt_g = 0;
    while( (opt_g = getopt(argc, argv, "c:dhv")) != -1 ) {
        switch(opt_g)
        {
            case 'c':
                memset(cfgFile, 0, sizeof(cfgFile));
                strncpy(cfgFile, optarg, PATH_MAX);
                break;
            case 'd':
                daemonize = true;
                break;
            case 'v':
                show_vcs_info();
                ::exit(EXIT_SUCCESS);
            case 'h':
            default:
                usage();
                ::exit(EXIT_SUCCESS);
        }
    }

    std::cout << "current system cfg file [" <<  cfgFile << "]."  << std::endl;
    // display build version info.
    show_vcs_info();


    tzrpc::set_checkpoint_log_store_func(syslog);
    if (!tzrpc::log_init(7)) {
        std::cerr << "init syslog failed!" << std::endl;
        ::exit(EXIT_FAILURE);
    }
    tzrpc::log_debug("syslog initialized ok!");

    // test boost::atomic
    boost::atomic<int> atomic_int;
    if (atomic_int.is_lock_free()) {
        tzrpc::log_notice(">>> GOOD <<<, your system atomic is lock_free ...");
    } else {
        tzrpc::log_err(">>> BAD <<<, your system atomic is not lock_free, may impact performance ...");
    }


    // SSL 环境设置
    if (!tzrpc::Ssl_thread_setup()) {
        tzrpc::log_err("SSL env setup error!");
        ::exit(EXIT_FAILURE);
    }


    // daemonize should before any thread creation...
    if (daemonize) {
        tzrpc::log_notice("we will daemonize this service...");

        bool chdir = false; // leave the current working directory in case
                            // the user has specified relative paths for
                            // the config file, etc
        bool close = true;  // close stdin, stdout, stderr
        if (::daemon(!chdir, !close) != 0) {
            tzrpc::log_err("call to daemon() failed: %s.", strerror(errno));
            ::exit(EXIT_FAILURE);
        }
    }

    (void)tzrpc::Captain::instance(); // create object first!

    create_process_pid();
    init_signal_handle();
    tzrpc::backtrace_init();


    {
        PUT_RAII_PERF_COUNTER(Captain_init);
        if(!tzrpc::Captain::instance().init(cfgFile)) {
            tzrpc::log_err("system manager init error!");
            ::exit(EXIT_FAILURE);
        }
    }

    std::time_t now = boost::chrono::system_clock::to_time_t(boost::chrono::system_clock::now());
    char mbstr[32] {};
    std::strftime(mbstr, sizeof(mbstr), "%F %T", std::localtime(&now));
    tzrpc::log_info("service started at %s", mbstr);

    tzrpc::log_notice("whole service initialized ok!");
    tzrpc::Captain::instance().service_joinall();

    tzrpc::Ssl_thread_clean();

    return 0;
}

