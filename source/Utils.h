#ifndef _TiBANK_UTILS_H_
#define _TiBANK_UTILS_H_

#include <sys/time.h>

#include <memory>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
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


#include <libconfig.h++>

bool sys_config_init(const std::string& config_file);
libconfig::Config& get_config_object();

template <typename T>
bool get_config_value(const std::string& key, T& t) {
    return get_config_object().lookupValue(key, t);
}

//////////

// SQL connection pool
class SqlConn;
typedef std::shared_ptr<SqlConn> sql_conn_ptr;

// Redis connection pool
class RedisConn;
typedef std::shared_ptr<RedisConn> redis_conn_ptr;

// Timer Task helper
typedef boost::function<void ()> TimerEventCallable;
class TimerService;



//
// free functions, in helper namespace

namespace helper {


bool request_scoped_sql_conn(sql_conn_ptr& conn);
sql_conn_ptr request_sql_conn();
sql_conn_ptr try_request_sql_conn(size_t msec);
void free_sql_conn(sql_conn_ptr conn);


bool request_scoped_redis_conn(redis_conn_ptr& conn);


std::shared_ptr<TimerService> request_timer_service();
int64_t register_timer_task(TimerEventCallable func, int64_t msec, bool persist = true, bool fast = true);
int64_t revoke_timer_task(int64_t index);

const std::string& request_http_docu_root();
const std::vector<std::string>& request_http_docu_index();


struct COUNT_FUNC_PERF: public boost::noncopyable {

    COUNT_FUNC_PERF(const std::string env, const std::string key):
    env_(env), key_(key) {
        ::gettimeofday(&start_, NULL);
    }

    ~COUNT_FUNC_PERF() {
        struct timeval end;
        ::gettimeofday(&end, NULL);

        int64_t time_ms = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) / 1000; // ms
        int64_t time_us = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) % 1000; // us

        display_info(env_, time_ms, time_us);
    }

    void display_info(const std::string& env, int64_t time_ms, int64_t time_us);

private:
    std::string env_;
    std::string key_;
    struct timeval start_;
};


} // namespace


#include <boost/assert.hpp>
#include <boost/format.hpp>
#ifdef NP_DEBUG
#define PUT_COUNT_FUNC_PERF(T)  helper::COUNT_FUNC_PERF PERF_CHECKER_##T( boost::str(boost::format("%s(%ld):%s") % __FILE__%__LINE__%BOOST_CURRENT_FUNCTION), #T ); \
                (void) PERF_CHECKER_##T
#else
#define PUT_COUNT_FUNC_PERF(T) ((void)0)
#endif



#endif // _TiBANK_UTILS_H_
