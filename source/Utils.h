#ifndef _TiBANK_UTILS_H_
#define _TiBANK_UTILS_H_

#include <memory>
#include <string>

#include <boost/function.hpp>

void backtrace_init();
int set_nonblocking(int fd);


#include <boost/lexical_cast.hpp>

template <typename T>
std::string convert_to_string(const T& arg) {
    try {
        return boost::lexical_cast<std::string>(arg);
    }
    catch(boost::bad_lexical_cast& e) {
        return "";
    }
}

// SQL connection pool
class SqlConn;
typedef std::shared_ptr<SqlConn> sql_conn_ptr;

bool request_scoped_sql_conn(sql_conn_ptr& conn);
sql_conn_ptr request_sql_conn();
sql_conn_ptr try_request_sql_conn(size_t msec);
void free_sql_conn(sql_conn_ptr conn);


// Redis connection pool
class RedisConn;
typedef std::shared_ptr<RedisConn> redis_conn_ptr;

bool request_scoped_redis_conn(redis_conn_ptr& conn);


// Timer Task helper
typedef boost::function<void ()> TimerEventCallable;
int64_t register_timer_task(TimerEventCallable func, int64_t msec, bool persist = true, bool fast = true);
int64_t revoke_timer_task(int64_t index);

#include <libconfig.h++>

bool sys_config_init(const std::string& config_file);
libconfig::Config& get_config_object();

template <typename T>
bool get_config_value(const std::string& key, T& t) {
    return get_config_object().lookupValue(key, t);
}



#endif // _TiBANK_UTILS_H_
