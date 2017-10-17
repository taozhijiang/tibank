#include "TiGeneral.h"
#include "Log.h"

Log& Log::instance() {
	static Log helper;
	return helper;
}

bool Log::init(int log_level) {
    openlog(program_invocation_short_name, LOG_PID , LOG_LOCAL6);
    setlogmask (LOG_UPTO (log_level));
	return true;
}

void Log::log_api(int priority, const char *file, int line, const char *func, const char *msg, ...) {

    int n = 0;
    char buf[MAX_LOG_BUF_SIZE + 2] = {0,};

    n += sprintf(buf + n,"[%s:%d][%s][%ld]-- ", file, line, func, (long)pthread_self());

    va_list arg_ptr;
    va_start(arg_ptr, msg);
    int ret = vsnprintf(buf + n, MAX_LOG_BUF_SIZE - n, msg, arg_ptr);
    va_end(arg_ptr);

    n = (ret >= MAX_LOG_BUF_SIZE - n) ?  MAX_LOG_BUF_SIZE : (n + ret);
    buf[n++] = '\n';
    buf[n] = 0;

    ::syslog(priority, buf);
}
