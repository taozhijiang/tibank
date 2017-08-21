#ifndef _TiBANK_LOG_H_
#define _TiBANK_LOG_H_

// man 3 syslog
#include <syslog.h>
#include <stdarg.h>

// LOG_EMERG      system is unusable
// LOG_ALERT      action must be taken immediately
// LOG_CRIT       critical conditions
// LOG_ERR        error conditions
// LOG_WARNING    warning conditions
// LOG_NOTICE     normal, but significant, condition
// LOG_INFO       informational message
// LOG_DEBUG      debug-level message

static const size_t MAX_LOG_BUF_SIZE = 16*1024;

class Log {
public:
	static Log& instance();
	bool init(int log_level);

	~Log() {
        closelog();
	}

private:
    int get_time_prefix(char *buf, int size);

public:
    void log_api(int priority, const char *file, int line, const char *func, const char *msg, ...);
};

#define log_emerg(...)  Log::instance().log_api( LOG_EMERG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_alert(...)  Log::instance().log_api( LOG_ALERT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_crit(...)   Log::instance().log_api( LOG_CRIT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_err(...)  Log::instance().log_api( LOG_ERR, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_warning(...)  Log::instance().log_api( LOG_WARNING, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_notice(...)  Log::instance().log_api( LOG_NOTICE, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_info(...)  Log::instance().log_api( LOG_INFO, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define log_debug(...)  Log::instance().log_api( LOG_DEBUG, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#endif // _TiBANK_LOG_H_
