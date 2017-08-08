#include <cstdlib>

#include <string>
#include <map>

#include "ServiceManager.h"
#include "SignHelper.h"
#include "TimerService.h"


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

	timer_service_ptr_.reset(new TimerService());
	if (!timer_service_ptr_ || !timer_service_ptr_->init()) {
		log_error("Init TimerService failed!");
		return false;
	}

	// start work
	timer_service_ptr_->start_timer();

	log_trace("ServiceManager all initialized...");
    initialized_ = true;

	return true;
}


bool ServiceManager::service_graceful() {

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
