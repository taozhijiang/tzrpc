#include <signal.h>
void backtrace_init();

#include <ctime>
#include <cstdio>
#include <sstream>

#include <syslog.h>
#include <boost/format.hpp>
#include <boost/atomic/atomic.hpp>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/ConfHelper.h>
#include <Scaffold/Status.h>

#include <Scaffold/Manager.h>

static void interrupted_callback(int signal){
    log_alert("Signal %d received ...", signal);
    switch(signal) {
        case SIGHUP:
            log_notice("SIGHUP recv, do update_run_conf... ");
            tzrpc::ConfHelper::instance().update_runtime_conf();
            break;

        case SIGUSR1:
            log_notice("SIGUSR recv, do module_status ... ");
            {
                std::string output;
                tzrpc::Status::instance().collect_status(output);
                std::cout << output << std::endl;
                log_notice("%s", output.c_str());
            }
            break;

        default:
            log_err("Unhandled signal: %d", signal);
            break;
    }
}

static void init_signal_handle(){

    ::signal(SIGPIPE, SIG_IGN);
    ::signal(SIGUSR1, interrupted_callback);
    ::signal(SIGHUP,  interrupted_callback);

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

    fprintf(stderr, "we using system configure file %s\n", cfgFile);

    set_checkpoint_log_store_func(syslog);
    if (!log_init(7)) {
        std::cerr << "init syslog failed!" << std::endl;
        ::exit(EXIT_FAILURE);
    }
    log_debug("syslog initialized ok!");

    // test boost::atomic
    boost::atomic<int> atomic_int;
    if (atomic_int.is_lock_free()) {
        log_notice(">>> GOOD <<<, your system atomic is lock_free ...");
    } else {
        log_err(">>> BAD <<<, your system atomic is not lock_free, may impact performance ...");
    }


    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(EXIT_FAILURE);
    }


    // daemonize should before any thread creation...
    if (daemonize) {
        log_notice("we will daemonize this service...");

        bool chdir = false; // leave the current working directory in case
                            // the user has specified relative paths for
                            // the config file, etc
        bool close = true;  // close stdin, stdout, stderr
        if (::daemon(!chdir, !close) != 0) {
            log_err("call to daemon() failed: %s.", strerror(errno));
            ::exit(EXIT_FAILURE);
        }
    }

    (void)tzrpc::Manager::instance(); // create object first!

    create_process_pid();
    init_signal_handle();
    backtrace_init();


    {
        PUT_COUNT_FUNC_PERF(Manager_init);
        if(!tzrpc::Manager::instance().init(cfgFile)) {
            log_err("system manager init error!");
            ::exit(EXIT_FAILURE);
        }
    }

    std::time_t now = boost::chrono::system_clock::to_time_t(boost::chrono::system_clock::now());
    char mbstr[32] {};
    std::strftime(mbstr, sizeof(mbstr), "%F %T", std::localtime(&now));
    log_info("service started at %s", mbstr);

    log_notice("whole service initialized ok!");
    tzrpc::Manager::instance().service_joinall();

    Ssl_thread_clean();

    return 0;
}

