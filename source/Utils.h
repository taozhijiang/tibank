#ifndef _TiBANK_UTILS_H_
#define _TiBANK_UTILS_H_

#include <string>

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

#include "SqlConnPool.h"
#include "ServiceManager.h"

static inline bool request_scoped_sql_conn(sql_conn_ptr& conn) {
    return ServiceManager::instance().sql_pool_ptr_->request_scoped_conn(conn);
}

static inline sql_conn_ptr request_sql_conn() {
    return ServiceManager::instance().sql_pool_ptr_->request_conn();
}

static inline sql_conn_ptr try_request_sql_conn(size_t msec) {
    return ServiceManager::instance().sql_pool_ptr_->try_request_conn(msec);
}

static inline void free_sql_conn(sql_conn_ptr conn) {
    ServiceManager::instance().sql_pool_ptr_->free_conn(conn);
}

template <typename T>
bool get_config_value(const std::string& key, T& t) {
    return ServiceManager::instance().cfg.lookupValue(key, t);
}



#endif // _TiBANK_UTILS_H_
