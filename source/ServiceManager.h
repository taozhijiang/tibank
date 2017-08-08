#ifndef __TiBANK_SERVICE_MANAGER_H__
#define __TiBANK_SERVICE_MANAGER_H__

#include <string>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

class TimerService;

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
};


#endif //__TiBANK_SERVICE_MANAGER_H__
