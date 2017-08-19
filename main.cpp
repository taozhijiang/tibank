#include <signal.h>
void backtrace_init();

#include "config.h"

#include "TiGeneral.h"
#include "Utils.h"
#include "Log.h"
#include "ServiceManager.h"

#include "commonutil/SslSetup.h"

volatile bool TiBANK_SHUTDOWN = false;
const char* TiBANK_DATABASE_PREFIX = "tibank";

static void interrupted_callback(int signal){
    log_error("Signal %d received ...", signal);

    switch(signal) {

	case SIGQUIT:
            log_trace("Graceful stop tibank service ... ");
            ServiceManager::instance().service_graceful();
            log_trace("Graceful stop tibank done!"); // main join all
            TiBANK_SHUTDOWN = true;
            break;

        case SIGTERM:
            log_trace("Immediately shutdown tibank service ... ");
            ServiceManager::instance().service_terminate();
            log_trace("Immediately shutdown tibank service done! ");
            TiBANK_SHUTDOWN = true;
            ::sleep(1);
            ::exit(0);
            break;

        default:
            log_error("Unhandled signal: %d", signal);
            break;
    }
}

static void init_signal_handle(){
    ::signal(SIGPIPE, SIG_IGN);

    ::signal(SIGQUIT, interrupted_callback);
    ::signal(SIGTERM, interrupted_callback);

    return;
}


// /var/run/[program_invocation_short_name].pid --> root permission
static int create_process_pid() {
    char pid_msg[24];
    char pid_file[PATH_MAX];

    snprintf(pid_file, PATH_MAX, "./%s.pid", program_invocation_short_name);
    FILE* fp = fopen(pid_file, "w+");

    if (!fp) {
        log_error("Create pid file %s failed!", pid_file);
        return -1;
    }

    pid_t pid = ::getpid();
    snprintf(pid_msg, sizeof(pid_msg), "%d\n", pid);
    fwrite(pid_msg, sizeof(char), strlen(pid_msg), fp);

    fclose(fp);
    return 0;
}

int main(int argc, char* argv[]) {

    std::cout << " THIS CURRENT RELEASE OF TiBANK " << std::endl;
    std::cout << "      VERSION: "  << tibank_VERSION_MAJOR << "." << tibank_VERSION_MINOR << std::endl;
    if (!Log::instance().init()) {
        std::cerr << "Init syslog failed!" << std::endl;
        return -1;
    }

    log_debug("syslog initialized ok!");

    (void)ServiceManager::instance(); // create object first!

    create_process_pid();
    init_signal_handle();
    backtrace_init();

    if(!ServiceManager::instance().init()) {
        log_error("ServiceManager init error!");
        ::exit(1);
    }

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_error("SSL env setup error!");
        ::exit(1);
    }

    log_debug( "TiBank system initialized ok!");
    ServiceManager::instance().service_joinall();

    Ssl_thread_clean();

    return 0;
}
