#ifndef __TiBANK_SERVICE_MANAGER_H__
#define __TiBANK_SERVICE_MANAGER_H__

#include <string>
#include <map>
#include <vector>

#include "HttpServer.h"
#include "TCPConnAsync.h"

class HttpServer;

template <typename T, typename Helper>
class ConnPool;

class SqlConn;
class SqlConnPoolHelper;

class RedisConn;
class RedisConnPoolHelper;

class TimerService;

class TransProcessTask;

class Manager {
public:
    static Manager& instance();

public:
    bool init();

    bool service_joinall();
    bool service_graceful();
    void service_terminate();

private:
    Manager();

    bool initialized_;

public:
    std::shared_ptr<TimerService> timer_service_ptr_;
    std::shared_ptr<HttpServer> http_server_ptr_;
    std::shared_ptr<TransProcessTask> trans_process_ptr_;
    std::shared_ptr<ConnPool<SqlConn, SqlConnPoolHelper>> sql_pool_ptr_;
    std::shared_ptr<ConnPool<RedisConn, RedisConnPoolHelper>> redis_pool_ptr_;
};


#endif //__TiBANK_SERVICE_MANAGER_H__
