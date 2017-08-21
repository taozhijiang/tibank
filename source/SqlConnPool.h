#ifndef _TiBANK_SQL_CONN_POOL_H_
#define _TiBANK_SQL_CONN_POOL_H_

#include <set>

#include <boost/noncopyable.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "SqlConn.h"

struct sql_conn_ptr_compare {
    bool operator() (const sql_conn_ptr& lhs,
                     const sql_conn_ptr& rhs) const {
        return (lhs.get() < rhs.get());
    }
};

class SqlConnPool: public boost::noncopyable
{
public:
    SqlConnPool(size_t capacity, string host, int port, string user,
                 string passwd, string db);

    // 由于会返回nullptr，所以不能返回引用
    sql_conn_ptr request_conn();
    sql_conn_ptr try_request_conn(size_t msec);
    bool request_scoped_conn(sql_conn_ptr& scope_conn);
    void free_conn(sql_conn_ptr conn);

    size_t get_conn_capacity() const { return capacity_; }

    ~SqlConnPool(){}

private:

    sql_conn_ptr do_request_conn();
    bool do_check_available() {
        return (!sql_conns_free_.empty() || (sql_conns_free_.size() + sql_conns_work_.size()) < capacity_ );
    }

    boost::condition_variable_any conn_notify_;
    boost::mutex conn_notify_mutex_;

    size_t aquired_time_; 	// 使用计数

    const size_t capacity_; // total capacity
    const string host_;
    const int port_;
    const string user_;
    const string passwd_;
    const string db_;

    std::set<sql_conn_ptr, sql_conn_ptr_compare> sql_conns_work_;
    std::set<sql_conn_ptr, sql_conn_ptr_compare> sql_conns_free_;
};

typedef std::set<sql_conn_ptr, sql_conn_ptr_compare> SqlConnContainer;

#endif  // _TiBANK_SQL_CONN_POOL_H_
