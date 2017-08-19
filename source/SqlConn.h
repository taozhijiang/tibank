#ifndef _TiBANK_SQL_CONN_H_
#define _TiBANK_SQL_CONN_H_

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "Log.h"
#include "TiGeneral.h"


class SqlConnPool;

class SqlConn;
typedef boost::shared_ptr<SqlConn> sql_conn_ptr;
typedef boost::scoped_ptr<sql::ResultSet> scoped_result_ptr;
typedef boost::shared_ptr<sql::ResultSet> shared_result_ptr;

static const int kMaxBuffSize = 8190;
static std::string va_format(const char * fmt, ...) {

    char buff[kMaxBuffSize + 1] = {0, };
    uint32_t n = 0;

    va_list ap;
    va_start(ap, fmt);
    n += vsnprintf(buff, kMaxBuffSize, fmt, ap);
    va_end(ap);
    buff[n] = '\0';

    return std::string(buff, n);
}

template <typename T>
bool cast_raw_value(shared_result_ptr result, const uint32_t idx, T& val) {
    if (typeid(T) == typeid(float) ||
        typeid(T) == typeid(double) ) {
        val = static_cast<T>(result->getDouble(idx));
    }
    else if (typeid(T) == typeid(int) ||
        typeid(T) == typeid(int64_t) ) {
        val = static_cast<T>(result->getInt64(idx));
    }
    else if (typeid(T) == typeid(unsigned int) ||
        typeid(T) == typeid(result) ) {
        val = static_cast<T>(result->getUInt64(idx));
    }
    else {
        log_error("Tell unsupported type: %s", typeid(T).name());
        return false;
    }

    return true;
}

// 特例化如果多次包含连接会重复定义，所以要么static、inline
template <>
inline bool cast_raw_value(shared_result_ptr result, const uint32_t idx, std::string& val) {
    val = static_cast<std::string>(result->getString(static_cast<int32_t>(idx)));
    return true;
}

// 可变模板参数进行查询
template <typename T, typename ... Args>
bool cast_raw_value(shared_result_ptr result, const uint32_t idx, T& val, Args& ... rest) {

	cast_raw_value(result, idx, val);
    return cast_raw_value(result, idx+1, rest ...);
}



class SqlConn: public boost::noncopyable {
public:
    explicit SqlConn(SqlConnPool& pool);
    ~SqlConn();
    bool init(int64_t conn_uuid, string host, string user, string passwd, string db);

    void set_uuid(int64_t uuid) { conn_uuid_ = uuid; }
    int64_t get_uuid() { return conn_uuid_; }


    // Simple SQL API
    bool sqlconn_execute(const string& sql);
    sql::ResultSet* sqlconn_execute_query(const string& sql);
    int sqlconn_execute_update(const string& sql);

    // 常用操作
    template <typename T>
    bool sqlconn_execute_query_value(const string& sql, T& val);
    template<typename ... Args>
    bool sqlconn_execute_query_values(const string& sql, Args& ... rest);
    template <typename T>
    bool sqlconn_execute_query_multi(const string& sql, std::vector<T>& vec);

    bool begin_transaction() { return sqlconn_execute("START TRANSACTION"); }
    bool commit() { return sqlconn_execute("COMMIT"); }
    bool rollback() { return sqlconn_execute("ROLLBACK"); }

private:
    sql::Driver* driver_;   /* no need explicit free */

    int64_t  conn_uuid_;   // reinterpret_cast
    boost::scoped_ptr<sql::Connection> conn_;
    boost::scoped_ptr<sql::Statement> stmt_;

    // may be used in future
    SqlConnPool&    pool_;
};


template <typename T>
bool SqlConn::sqlconn_execute_query_value(const string& sql, T& val) {
    try {

        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        stmt_->execute(sql);
        shared_result_ptr result(stmt_->getResultSet());
        if (!result)
            return false;

        if (result->rowsCount() != 1) {
            log_error( "Error rows count: %d", result->rowsCount());
            return false;
        }

        if (result->next())
            return cast_raw_value(result, 1, val);

        return false;

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return false;
    }
}



template <typename ... Args>
bool SqlConn::sqlconn_execute_query_values(const string& sql, Args& ... rest){

    try {
        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        stmt_->execute(sql);
        shared_result_ptr result(stmt_->getResultSet());
        if (!result)
            return false;

        if (result->rowsCount() != 1) {
            log_error( "Error rows count: %d", result->rowsCount());
            return false;
        }

        if (result->next())
            return cast_raw_value(result, 1, rest ...);

        return false;

    } catch (sql::SQLException &e)  {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return false;
    }
}



template <typename T>
bool SqlConn::sqlconn_execute_query_multi(const string& sql, std::vector<T>& vec) {

    try {

        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        stmt_->execute(sql);
        shared_result_ptr result(stmt_->getResultSet());
        if (!result)
            return false;

        vec.clear();
        T r_val;
        bool bRet = false;
        while (result->next()) {
            if (cast_raw_value(result, 1, r_val)) {
                vec.push_back(r_val);
                bRet = true;
            }
        }
        return bRet;

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return false;
    }
}


#endif  // _TiBANK_SQL_CONN_H_
