#include <signal.h>
void backtrace_init();

#include <iostream>
#include <sstream>

#include <syslog.h>
#include <boost/format.hpp>
#include <boost/atomic/atomic.hpp>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/Setting.h>
#include <Scaffold/Manager.h>

static void init_signal_handle(){

    ::signal(SIGPIPE, SIG_IGN);
    ::signal(SIGUSR1, SIG_IGN);
    ::signal(SIGUSR2, SIG_IGN);

    return;
}

extern char * program_invocation_short_name;
static void usage() {
    std::stringstream ss;

    ss << program_invocation_short_name << ":" << std::endl;
    ss << "\t -c cfgFile  specify config file, default " << program_invocation_short_name << ".conf. " << std::endl;
    ss << "\t -d          daemonize service." << std::endl;
    ss << "\t -v          print version info." << std::endl;
    ss << std::endl;

    std::cout << ss.str();
}

char cfgFile[PATH_MAX] {};
bool daemonize = false;

static void show_vcs_info () {

    std::cout << " THIS RELEASE OF " << program_invocation_short_name << std::endl;

    extern const char *build_commit_version;
    extern const char *build_commit_branch;
    extern const char *build_commit_date;
    extern const char *build_commit_author;
    extern const char *build_time;

    std::cout << build_commit_version << std::endl;
    std::cout << build_commit_branch << std::endl;
    std::cout << build_commit_date << std::endl;
    std::cout << build_commit_author << std::endl;
    std::cout << build_time << std::endl;

    return;
}


// /var/run/[program_invocation_short_name].pid --> root permission
static int create_process_pid() {

    char pid_msg[24];
    char pid_file[PATH_MAX];

    snprintf(pid_file, PATH_MAX, "./%s.pid", program_invocation_short_name);
    FILE* fp = fopen(pid_file, "w+");

    if (!fp) {
        log_err("Create pid file %s failed!", pid_file);
        return -1;
    }

    pid_t pid = ::getpid();
    snprintf(pid_msg, sizeof(pid_msg), "%d\n", pid);
    fwrite(pid_msg, sizeof(char), strlen(pid_msg), fp);

    fclose(fp);
    return 0;
}

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


    std::cout << "we using system configure file: " << cfgFile << std::endl;
    if (!tzrpc::sys_config_init(cfgFile)) {
        std::cout << "handle system configure " << cfgFile <<" failed!" << std::endl;
        return -1;
    }

    set_checkpoint_log_store_func(syslog);
    if (!log_init(tzrpc::setting.log_level_)) {
        std::cerr << "init syslog failed!" << std::endl;
        return -1;
    }

    // daemonize should before any thread creation...
    if (daemonize) {
        std::cout << "we will daemonize this service..." << std::endl;
        log_notice("we will daemonize this service...");

        bool chdir = false; // leave the current working directory in case
                            // the user has specified relative paths for
                            // the config file, etc
        bool close = true;  // close stdin, stdout, stderr
        if (::daemon(!chdir, !close) != 0) {
            log_err("call to daemon() failed: %s", strerror(errno));
            std::cout << "call to daemon() failed: " << strerror(errno) << std::endl;
            ::exit(EXIT_FAILURE);
        }
    }

    log_debug("syslog initialized ok!");
    time_t now = ::time(NULL);
    struct tm service_start;
    ::localtime_r(&now, &service_start);
    log_info("service started at %s", ::asctime(&service_start));

    // test boost::atomic
    boost::atomic<int> atomic_int;
    if (atomic_int.is_lock_free()) {
        log_alert(">>> GOOD <<<, your system atomic is lock_free ...");
    } else {
        log_err(">>> BAD <<<, your system atomic is not lock_free, may impact performance ...");
    }

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(1);
    }


    (void)tzrpc::Manager::instance(); // create object first!

    create_process_pid();
    init_signal_handle();
    backtrace_init();

    {
        PUT_COUNT_FUNC_PERF(Manager_init);
        if(!tzrpc::Manager::instance().init()) {
            log_err("Manager init error!");
            ::exit(1);
        }
    }

    log_debug( "service initialized ok!");
    tzrpc::Manager::instance().service_joinall();

    Ssl_thread_clean();

    return 0;
}



namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {
    log_err("BAD!!! expr `%s` assert failed at %s(%ld): %s", expr, file, line, function);
}
} // end boost
