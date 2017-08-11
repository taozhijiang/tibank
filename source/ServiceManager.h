#ifndef __TiBANK_SERVICE_MANAGER_H__
#define __TiBANK_SERVICE_MANAGER_H__

#include <string>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "SqlConnPool.h"

class TimerService;
class HttpServer;

class ServiceManager {
public:
	static ServiceManager& instance();

public:
    bool init();

    bool service_joinall();
	bool service_graceful();
	void service_terminate();

private:
	ServiceManager();

    bool initialized_;

public:
	boost::shared_ptr<TimerService> timer_service_ptr_;
	boost::shared_ptr<HttpServer> http_server_ptr_;
    boost::shared_ptr<SqlConnPool> sql_pool_ptr_;
};


#endif //__TiBANK_SERVICE_MANAGER_H__
