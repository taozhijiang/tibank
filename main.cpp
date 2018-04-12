#include <signal.h>
void backtrace_init();

#include "config.h"

#include "General.h"
#include "Utils.h"
#include "Log.h"
#include "SrvManager.h"

#include "commonutil/SslSetup.h"
#include "commonutil/HttpUtil.h"

volatile bool TiBANK_SHUTDOWN = false;
const char* TiBANK_DATABASE_PREFIX = "tibank";
struct tm service_start{};

static void interrupted_callback(int signal){
    log_err("Signal %d received ...", signal);

    switch(signal) {

	case SIGQUIT:
            log_info("Graceful stop tibank service ... ");
            SrvManager::instance().service_graceful();
            log_info("Graceful stop tibank done!"); // main join all
            TiBANK_SHUTDOWN = true;
            break;

        case SIGTERM:
            log_info("Immediately shutdown tibank service ... ");
            SrvManager::instance().service_terminate();
            log_info("Immediately shutdown tibank service done! ");
            TiBANK_SHUTDOWN = true;
            ::sleep(1);
            ::exit(0);
            break;

        default:
            log_err("Unhandled signal: %d", signal);
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

    std::cout << " THIS CURRENT RELEASE OF TiBANK " << std::endl;
    std::cout << "      VERSION: "  << tibank_VERSION_MAJOR << "." << tibank_VERSION_MINOR << std::endl;

    if (!sys_config_init()) {
        std::cout << "Handle system configure failed!" << std::endl;
        return -1;
    }

    int log_level = 0;
    if (!get_config_value("log_level", log_level)) {
		log_level = LOG_INFO;
		log_info("Using default log_level LOG_INFO");
	}

    if (!Log::instance().init(log_level)) {
        std::cerr << "Init syslog failed!" << std::endl;
        return -1;
    }

    log_debug("syslog initialized ok!");
    time_t now = ::time(NULL);
    ::localtime_r(&now, &service_start);
    log_info("Service start at %s", ::asctime(&service_start));

    (void)SrvManager::instance(); // create object first!
    HttpUtil::InitHttpEnvironment();

    create_process_pid();
    init_signal_handle();
    backtrace_init();

    if(!SrvManager::instance().init()) {
        log_err("SrvManager init error!");
        ::exit(1);
    }

    // SSL 环境设置
    if (!Ssl_thread_setup()) {
        log_err("SSL env setup error!");
        ::exit(1);
    }

    log_debug( "TiBank system initialized ok!");
    SrvManager::instance().service_joinall();

    Ssl_thread_clean();

    return 0;
}
