#include <signal.h>

#include <ctime>
#include <cstdio>

#include <syslog.h>
#include <boost/format.hpp>
#include <linux/limits.h>

#include <Utils/Utils.h>
#include <Utils/Log.h>
#include <Utils/SslSetup.h>

#include <Scaffold/ConfHelper.h>
#include <Scaffold/Status.h>

// API for main

void init_signal_handle();
void usage();
void show_vcs_info();
int create_process_pid();


static void interrupted_callback(int signal){
    tzrpc::log_alert("Signal %d received ...", signal);
    switch(signal) {
        case SIGHUP:
            tzrpc::log_notice("SIGHUP recv, do update_run_conf... ");
            tzrpc::ConfHelper::instance().update_runtime_conf();
            break;

        case SIGUSR1:
            tzrpc::log_notice("SIGUSR recv, do module_status ... ");
            {
                std::string output;
                tzrpc::Status::instance().collect_status(output);
                std::cout << output << std::endl;
                tzrpc::log_notice("%s", output.c_str());
            }
            break;

        default:
            tzrpc::log_err("Unhandled signal: %d", signal);
            break;
    }
}

void init_signal_handle(){

    ::signal(SIGPIPE, SIG_IGN);
    ::signal(SIGUSR1, interrupted_callback);
    ::signal(SIGHUP,  interrupted_callback);

    return;
}

extern char * program_invocation_short_name;
void usage() {
    std::stringstream ss;

    ss << program_invocation_short_name << ":" << std::endl;
    ss << "\t -c cfgFile  specify config file, default " << program_invocation_short_name << ".conf. " << std::endl;
    ss << "\t -d          daemonize service." << std::endl;
    ss << "\t -v          print version info." << std::endl;
    ss << std::endl;

    std::cout << ss.str();
}

void show_vcs_info () {

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
int create_process_pid() {

    char pid_msg[24];
    char pid_file[PATH_MAX];

    snprintf(pid_file, PATH_MAX, "./%s.pid", program_invocation_short_name);
    FILE* fp = fopen(pid_file, "w+");

    if (!fp) {
        tzrpc::log_err("Create pid file %s failed!", pid_file);
        return -1;
    }

    pid_t pid = ::getpid();
    snprintf(pid_msg, sizeof(pid_msg), "%d\n", pid);
    fwrite(pid_msg, sizeof(char), strlen(pid_msg), fp);

    fclose(fp);
    return 0;
}
