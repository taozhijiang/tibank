#include "SqlConn.h"
#include "SqlConnPool.h"

#include <sstream>

#include <boost/scoped_ptr.hpp>

SqlConn::SqlConn(SqlConnPool& pool):
    driver_(),
    conn_uuid_(0),
    stmt_(),
    pool_(pool) {
}

bool SqlConn::init(int64_t conn_uuid,
                     string host, string user, string passwd, string db) {

    try {

        conn_uuid_ = conn_uuid;
        driver_ = get_driver_instance(); // not thread safe!!!

        std::stringstream output;
        output << "# " << driver_->getName() << ", version ";
		output << driver_->getMajorVersion() << "." << driver_->getMinorVersion();
        output << "." << driver_->getPatchVersion() << endl;
        log_trace("Driver info: %s", output.str().c_str());

		conn_.reset(driver_->connect(host, user, passwd));
        stmt_.reset(conn_->createStatement());
    }
    catch (sql::SQLException &e) {

        std::stringstream output;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());
        return false;
    }

    stmt_->execute("USE " + db + ";");
    log_info("Create New Connection OK!");
    return true;
}

SqlConn::~SqlConn() {

    /* reset to fore delete, actually not need */
    conn_.reset();
    stmt_.reset();

    log_trace("Destruct Connection %ld OK!", conn_uuid_);
}

bool SqlConn::sqlconn_execute(const string& sql) {

    try {

        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        stmt_->execute(sql);
        return true;

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());
    }

    return false;
}

sql::ResultSet* SqlConn::sqlconn_execute_query(const string& sql) {

    sql::ResultSet* result = NULL;

	try {

        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        stmt_->execute(sql);
        result = stmt_->getResultSet();

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

    }

    return result;
}

int SqlConn::sqlconn_execute_update(const string& sql) {

    try {

        if(!conn_->isValid()) {
            log_error("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        return stmt_->executeUpdate(sql);

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_error("%s", output.str().c_str());

    }

    return -1;
}
