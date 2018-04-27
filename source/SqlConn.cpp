#include <sstream>

#include "SqlConn.h"


SqlConn::SqlConn(ConnPool<SqlConn, SqlConnPoolHelper>& pool):
    driver_(),
    stmt_(),
    pool_(pool) {
}

bool SqlConn::init(int64_t conn_uuid,  const SqlConnPoolHelper& helper) {

    try {

        set_uuid(conn_uuid);
        driver_ = get_driver_instance(); // not thread safe!!!

        std::stringstream output;
        output << "# " << driver_->getName() << ", version ";
        output << driver_->getMajorVersion() << "." << driver_->getMinorVersion();
        output << "." << driver_->getPatchVersion() << endl;
        log_info("Driver info: %s", output.str().c_str());

        sql::ConnectOptionsMap connection_properties;
        connection_properties["hostName"] = helper.host_;
        connection_properties["port"] = helper.port_;
        connection_properties["userName"] = helper.user_;
        connection_properties["password"] = helper.passwd_;
        connection_properties["database"] = helper.db_;
        connection_properties["OPT_RECONNECT"] = true;

        conn_.reset(driver_->connect(connection_properties));
        stmt_.reset(conn_->createStatement());
    }
    catch (sql::SQLException &e) {

        std::stringstream output;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_err("%s", output.str().c_str());
        return false;
    }

    stmt_->execute("USE " + helper.db_ + ";");
    log_info("Create New Connection OK!");
    return true;
}

SqlConn::~SqlConn() {

    /* reset to fore delete, actually not need */
    conn_.reset();
    stmt_.reset();

    log_info("Destruct Connection %ld OK!", get_uuid());
}

bool SqlConn::sqlconn_execute(const string& sql) {

    try {

        if(!conn_->isValid()) {
            log_err("Invalid connect, do re-connect...");
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
        log_err("%s", output.str().c_str());
    }

    return false;
}

sql::ResultSet* SqlConn::sqlconn_execute_query(const string& sql) {

    sql::ResultSet* result = NULL;

    try {

        if(!conn_->isValid()) {
            log_err("Invalid connect, do re-connect...");
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
        log_err("%s", output.str().c_str());

    }

    return result;
}

int SqlConn::sqlconn_execute_update(const string& sql) {

    try {

        if(!conn_->isValid()) {
            log_err("Invalid connect, do re-connect...");
            conn_->reconnect();
        }

        return stmt_->executeUpdate(sql);

    } catch (sql::SQLException &e) {

        std::stringstream output;
        output << " STMT: " << sql << endl;
        output << "# ERR: " << e.what() << endl;
        output << " (MySQL error code: " << e.getErrorCode() << endl;
        output << ", SQLState: " << e.getSQLState() << " )" << endl;
        log_err("%s", output.str().c_str());

    }

    return -1;
}
