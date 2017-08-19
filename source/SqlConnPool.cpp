
#include <ctime>
#include <string>

#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>

#include "SqlConnPool.h"

SqlConnPool::SqlConnPool(size_t capacity, string host, string user,
						    string passwd, string db):
    capacity_(capacity), host_(host), user_(user), passwd_(passwd), db_(db),
    sql_conns_work_(), sql_conns_free_() {

    safe_assert(capacity_);

    log_trace( "TOTAL SQL CONN: %d", capacity_ );
    return;
}

sql_conn_ptr SqlConnPool::do_request_conn() {

    sql_conn_ptr conn;

    if (!sql_conns_free_.empty()){
        conn = (* (sql_conns_free_.begin()) );
        sql_conns_work_.insert(conn);
        sql_conns_free_.erase(conn);
        return conn;
    }

    if ( (sql_conns_free_.size() + sql_conns_work_.size()) < capacity_) {

        sql_conn_ptr new_conn = boost::make_shared<SqlConn>(*this);
        if (!new_conn){
            log_error("Creating new SqlConn failed!");
            return new_conn;
        }

        if (!new_conn->init(reinterpret_cast<int64_t>(new_conn.get()), host_, user_, passwd_, db_)){
            log_error("init new SqlConn failed!");
            new_conn.reset();
            return new_conn;
        }

        sql_conns_work_.insert(new_conn);
        return new_conn;
    }

    return conn;
}

sql_conn_ptr SqlConnPool::request_conn() {

    boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

    while (!do_check_available()) {
        conn_notify_.wait(lock);
    }

    return do_request_conn();
}


bool SqlConnPool::request_scoped_conn(sql_conn_ptr& scope_conn) {

    // reset first, all will stack at reset latter...
    // probably recursive require conn_nofity_mutex problem

    scope_conn.reset();
    boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

    while (!do_check_available()) {
        conn_notify_.wait(lock);
    }

    sql_conn_ptr conn = do_request_conn();
    if (conn) {
        scope_conn.reset(conn.get(),
                         boost::bind(&SqlConnPool::free_conn,
                         this, conn)); // 还是通过智能指针拷贝一份吧
        // log_debug("Request guard connection: %ld", scope_conn->get_uuid());
        return true;
    }

    return false;
}

sql_conn_ptr SqlConnPool::try_request_conn(size_t msec) {

    boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);
    sql_conn_ptr conn;

    if (!do_check_available() && !msec)
        return conn; // nullptr

    // timed_wait not work with 0
    if(do_check_available() || conn_notify_.timed_wait(lock, boost::posix_time::milliseconds(msec))) {
        SqlConnContainer::iterator it;
        return do_request_conn();
    }

    return conn;
}



// f (a < b) is false and (b < a) is false, then (a == b).
// This is how STL's find() works.
void SqlConnPool::free_conn(sql_conn_ptr conn) {

    {
        boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);

        sql_conns_free_.insert(conn);
        sql_conns_work_.erase(conn);

        // log_debug("Freeing %ld conn", conn->get_uuid());
    }

    conn_notify_.notify_all();
    return;
}

