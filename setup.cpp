#include <signal.h>

#include <ctime>
#include <cstdio>
#include <iostream>

#include <boost/format.hpp>
#include <linux/limits.h>

#include <version.h>
#include <other/Log.h>

#include <scaffold/Setting.h>
#include <scaffold/Status.h>

#include <Captain.h>
using tzrpc::Captain;

// API for main

void init_signal_handle();
void usage();
void show_vcs_info();
int create_process_pid();


static void interrupted_callback(int signal){
    roo::log_warning("signal %d received ...", signal);
    switch(signal) {
        case SIGHUP:
            roo::log_warning("signal SIGHUP recv, do update_run_conf... ");
            Captain::instance().setting_ptr()->update_runtime_setting();
            break;

        case SIGUSR1:
            roo::log_warning("signal SIGUSR recv, do module_status ... ");
            {
                std::string output;
                Captain::instance().status_ptr()->collect_status(output);
                std::cout << output << std::endl;
                roo::log_warning("%s", output.c_str());
            }
            break;

        default:
            roo::log_err("Unhandled signal %d received.", signal);
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

    ss << std::endl;
    ss << " * THIS RELEASE OF " << program_invocation_short_name
       << ": ver " << PROGRAM_VERSION << " * " << std::endl;

    ss << std::endl;
    ss << "\t -c cfgFile  specify config file, or using default " << program_invocation_short_name << ".conf. " << std::endl;
    ss << "\t -d          daemonize service." << std::endl;
    ss << "\t -v          print version info." << std::endl;
    ss << std::endl;

    std::cout << ss.str();
}

void show_vcs_info () {

    std::cout << std::endl;
    std::cout << " * THIS RELEASE OF " << program_invocation_short_name
              << ": ver " << PROGRAM_VERSION << " * " << std::endl;

    extern const char *build_commit_version;
    extern const char *build_commit_branch;
    extern const char *build_commit_date;
    extern const char *build_commit_author;
    extern const char *build_time;

    std::cout << std::endl;
    std::cout << "    " << build_commit_version << std::endl;
    std::cout << "    " << build_commit_branch << std::endl;
    std::cout << "    " << build_commit_date << std::endl;
    std::cout << "    " << build_commit_author << std::endl;
    std::cout << "    " << build_time << std::endl;

    return;
}


// /var/run/[program_invocation_short_name].pid --> root permission
int create_process_pid() {

    char pid_msg[24];
    char pid_file[PATH_MAX];

    snprintf(pid_file, PATH_MAX, "./%s.pid", program_invocation_short_name);
    FILE* fp = fopen(pid_file, "w+");

    if (!fp) {
        roo::log_err("Create pid_file at %s failed!", pid_file);
        return -1;
    }

    pid_t pid = ::getpid();
    snprintf(pid_msg, sizeof(pid_msg), "%d\n", pid);
    fwrite(pid_msg, sizeof(char), strlen(pid_msg), fp);

    fclose(fp);
    return 0;
}
