#include "TiGeneral.h"
#include "Log.h"

#include <algorithm>
#include <vector>
#include <boost/algorithm/string.hpp>

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

	// 如果消息中夹杂着换行，在rsyslog中处理(尤其是转发的时候)会比较麻烦
	// 如果原本发送，则接收端是#开头的编码字符，如果转移，根据协议换行意味着消息的结束，消息会丢失
	// 这种情况下，将消息拆分然后分别发送
	if (likely(std::find(buf, buf + n, '\n') == (buf + n))) {
		n = (ret >= MAX_LOG_BUF_SIZE - n) ?  MAX_LOG_BUF_SIZE : (n + ret);
		buf[n++] = '\n';
		buf[n] = 0;

		::syslog(priority, buf);
		return;
	}

	// 拆分消息
	n = (ret >= MAX_LOG_BUF_SIZE - n) ?  MAX_LOG_BUF_SIZE : (n + ret);
	buf[n] = 0;

	std::vector<std::string> messages;
	boost::split(messages, buf, boost::is_any_of("\r\n"));
	for (std::vector<string>::iterator it = messages.begin(); it != messages.end(); ++it){
		if (!it->empty()) {
			std::string message = (*it) + "\n";
			::syslog(priority, message.c_str());
		}
	}
}
