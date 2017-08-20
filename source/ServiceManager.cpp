#include <cstdlib>

#include <string>
#include <map>

#include "ServiceManager.h"
#include "SignHelper.h"

#include "TimerService.h"
#include "HttpServer.h"
#include "TransProcessTask.h"

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
        log_error("ServiceManager already initlialized...");
        return false;
    }

	if (!SignHelper::instance().init()) {
		log_error("Init SignHelper failed!");
        return false;
	}

    sql_pool_ptr_.reset(new SqlConnPool(20, "127.0.0.1", "root", "1234", "paybank"));
	if (!sql_pool_ptr_) {
		log_error("Init SqlConnPool failed!");
		return false;
	}

	timer_service_ptr_.reset(new TimerService());
	if (!timer_service_ptr_ || !timer_service_ptr_->init()) {
		log_error("Init TimerService failed!");
		return false;
	}

	http_server_ptr_.reset(new HttpServer("0.0.0.0", 8899, 8, "/var/www/html/"));
	if (!http_server_ptr_ || !http_server_ptr_->init()) {
		log_error("Init HttpServer failed!");
		return false;
	}

    trans_process_ptr_.reset(new TransProcessTask(4));
    if (!trans_process_ptr_ || !trans_process_ptr_->init()){
		log_error("Init TransProcessTask failed!");
		return false;
    }

	// start work
	timer_service_ptr_->start_timer();
    http_server_ptr_->io_service_threads_.start_threads();
	http_server_ptr_->net_conn_remove_threads_.start_threads();

	// business attached
	// trans_process_ptr_->trans_process_task_.start_threads();

	log_trace("ServiceManager all initialized...");
    initialized_ = true;

	return true;
}


bool ServiceManager::service_graceful() {

	http_server_ptr_->io_service_stop_graceful();
	http_server_ptr_->net_conn_remove_stop_graceful();
	timer_service_ptr_->stop_graceful();
	log_trace("timer_service_ graceful finished!");

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
