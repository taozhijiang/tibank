#include <cstdlib>

#include <string>
#include <map>

#include "SignHelper.h"
#include "Utils.h"
#include "TimerService.h"
#include "HttpServer.h"
#include "RedisData.h"
#include "TransProcessTask.h"

#include "ServiceManager.h"

// 在主线程中最先初始化，所以不考虑竞争条件问题
ServiceManager& ServiceManager::instance() {
	static ServiceManager service;
	return service;
}

ServiceManager::ServiceManager():
    initialized_(false){
}


bool ServiceManager::init() {

    if (initialized_) {
        log_err("ServiceManager already initlialized...");
        return false;
    }

    if (!SignHelper::instance().init()) {
		log_err("Init SignHelper failed!");
        return false;
	}

	RedisData::instance();

	timer_service_ptr_.reset(new TimerService());
	if (!timer_service_ptr_ || !timer_service_ptr_->init()) {
		log_err("Init TimerService failed!");
		return false;
	}

	std::string mysql_hostname;
	int mysql_port;
	std::string mysql_username;
	std::string mysql_passwd;
	std::string mysql_database;
	if (!get_config_value("mysql.host_addr", mysql_hostname) || !get_config_value("mysql.host_port", mysql_port) ||
		!get_config_value("mysql.username", mysql_username) || !get_config_value("mysql.passwd", mysql_passwd) ||
        !get_config_value("mysql.database", mysql_database)){
        log_err("Error, get mysql config value error");
        return false;
    }

	int conn_pool_size = 0;
	if (!get_config_value("mysql.conn_pool_size", conn_pool_size)) {
		conn_pool_size = 20;
		log_info("Using default conn_pool size: 20");
	}

	SqlConnPoolHelper helper(mysql_hostname, mysql_port,
						     mysql_username, mysql_passwd, mysql_database);
    sql_pool_ptr_.reset(new ConnPool<SqlConn, SqlConnPoolHelper>("MySQLPool", conn_pool_size, helper));
	if (!sql_pool_ptr_ || !sql_pool_ptr_->init()) {
		log_err("Init SqlConnPool failed!");
		return false;
	}

	std::string redis_hostname;
	int redis_port;
	std::string redis_passwd;
	if (!get_config_value("redis.host_addr", redis_hostname) || !get_config_value("redis.host_port", redis_port)){
        log_err("Error, get redis config value error");
        return false;
    }
	get_config_value("redis.passwd", redis_passwd);
	int redis_pool_size;
	if (!get_config_value("redis.conn_pool_size", redis_pool_size)) {
		redis_pool_size = 20;
		log_info("Using default conn_pool size: 20");
	}

	RedisConnPoolHelper redis_helper(redis_hostname, redis_port, redis_passwd);
    redis_pool_ptr_.reset(new ConnPool<RedisConn, RedisConnPoolHelper>("RedisPool", redis_pool_size, redis_helper));
	if (!redis_pool_ptr_ || !redis_pool_ptr_->init()) {
		log_err("Init RedisConnPool failed!");
		return false;
	}

	RedisData::instance().init();

    std::string listen_addr;
    int listen_port = 0;
    std::string doc_root;
    if (!get_config_value("http.listen_addr", listen_addr) || !get_config_value("http.listen_port", listen_port) ||
        !get_config_value("http.doc_root", doc_root)){
        log_err("Error, get value error");
        return false;
    }

	int thread_pool_size = 0;
	if (!get_config_value("http.thread_pool_size", thread_pool_size)) {
		thread_pool_size = 8;
		log_info("Using default thread_pool size: 8");
	}
	log_info("listen at: %s:%d, doc:%s, thread_pool: %d", listen_addr.c_str(), listen_port,
														   doc_root.c_str(), thread_pool_size);
    http_server_ptr_.reset(new HttpServer(listen_addr, listen_port, thread_pool_size, doc_root));
	if (!http_server_ptr_ || !http_server_ptr_->init()) {
		log_err("Init HttpServer failed!");
		return false;
	}

    trans_process_ptr_.reset(new TransProcessTask(4));
    if (!trans_process_ptr_ || !trans_process_ptr_->init()){
		log_err("Init TransProcessTask failed!");
		return false;
    }

	// start work
	timer_service_ptr_->start_timer();
    http_server_ptr_->io_service_threads_.start_threads();
	http_server_ptr_->net_conn_remove_threads_.start_threads();

	// business attached
	trans_process_ptr_->trans_process_task_.start_threads();

	log_info("ServiceManager all initialized...");
    initialized_ = true;

	return true;
}


bool ServiceManager::service_graceful() {

	http_server_ptr_->io_service_stop_graceful();
	http_server_ptr_->net_conn_remove_stop_graceful();
	timer_service_ptr_->stop_graceful();
	log_info("timer_service_ graceful finished!");

	return true;
}

void ServiceManager::service_terminate() {
	::sleep(1);
	::_exit(0);
}


extern volatile bool TiBANK_SHUTDOWN;

bool ServiceManager::service_joinall() {

    while (!TiBANK_SHUTDOWN){
        ::sleep(5);
    }

    return true;
}
