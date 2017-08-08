#include "SqlConn.h"
#include "SqlConnPool.h"

#include <sstream>

#include <boost/scoped_ptr.hpp>

SqlConn::SqlConn(SqlConnPool& pool, int64_t conn_uuid,
				   string host, string user, string passwd, string db):
	is_connected_(false),
    driver_(get_driver_instance()),
    conn_uuid_(conn_uuid),
    result_(),
    prep_stmt_(),
    pool_(pool) {

    try {
		conn_.reset(driver_->connect(host, user, passwd));
        stmt_.reset(conn_->createStatement());
    }
    catch (sql::SQLException &e) {

        std::stringstream output;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());
    }

    stmt_->execute("USE " + db + ";");
	set_connected(true);
    log_info("Create New Connection OK!");
}

SqlConn::~SqlConn() {

    /* reset to fore delete, actually not need */
    conn_.reset();
    stmt_.reset();
    prep_stmt_.reset();

    if (result_) {
        safe_assert(result_.unique());
        result_.reset();
    }
    //BOOST_LOG_T(info) << "Destruct Connection " << conn_uuid_ << " OK!" ;
}


bool SqlConn::execute_command(const string& sql) {

    try {

        if(!conn_->isValid())
            conn_->reconnect();

        return stmt_->execute(sql);

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

bool SqlConn::execute_query(const string& sql) {

	try {

        if(!conn_->isValid())
            conn_->reconnect();

        stmt_->execute(sql);
        result_.reset(stmt_->getResultSet());

        // 没有结果以及出错的时候，返回false
        if (result_->rowsCount() == 0)
            return false;

        return true;

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

// return 0 maybe error, we ignore this case
size_t SqlConn::execute_query_count(const string& sql) {

    try {

        if(!conn_->isValid())
            conn_->reconnect();

        stmt_->execute(sql);
        result_.reset(stmt_->getResultSet());
        if (result_->rowsCount() != 1)
            return 0;

        result_->next();
        return (result_->getInt(1));

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return 0;
    }
}

bool SqlConn::execute_check_exist(const string& sql) {

    if (execute_query_count(sql) > 0 )
        return true;

    return false;
}



bool SqlConn::execute_prep_stmt_command() {

    try {

        if(!conn_->isValid())
            conn_->reconnect();

        return prep_stmt_->execute();

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return false;
    }
}

bool SqlConn::execute_prep_stmt_query() {

    try {

        if(!conn_->isValid())
            conn_->reconnect();

        prep_stmt_->execute();
        result_.reset(prep_stmt_->getResultSet());

        // 没有结果以及出错的时候，返回false
        if (result_->rowsCount() == 0)
            return false;

        return true;

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

        return false;
    }
}

