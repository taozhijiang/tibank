#ifndef _TiBANK_REDIS_CONN_H
#define _TiBANK_REDIS_CONN_H


#include <hiredis/hiredis.h>
#include <hiredis/async.h>

#include <boost/thread/mutex.hpp>
#include "ConnPool.h"

class RedisConn;
typedef boost::shared_ptr<RedisConn> redis_conn_ptr;

struct RedisConnPoolHelper {
public:
    RedisConnPoolHelper(string host, int port, string passwd):
			host_(host), port_(port), passwd_(passwd) {
	}

public:
    const string host_;
    const int port_;
    const string passwd_;
};

typedef boost::shared_ptr<redisReply> redisReply_ptr;

class RedisConn: public boost::noncopyable {
public:
    explicit RedisConn(ConnPool<RedisConn, RedisConnPoolHelper>& pool):
		pool_(pool), conn_uuid_(0) {
	}

    ~RedisConn(){
	}

	bool init(int64_t conn_uuid, const RedisConnPoolHelper& helper);

    void set_uuid(int64_t uuid) { conn_uuid_ = uuid; }
    int64_t get_uuid() { return conn_uuid_; }

	redisReply_ptr exec(const char *format, ...);
	redisReply_ptr exec(int argc, const char **argv);
	redisReply_ptr eval(const std::string& script,
						const std::vector<std::string>& keys,
						const std::vector<std::string>& args);

	operator bool();

private:
	int64_t  conn_uuid_;   // reinterpret_cast
	boost::shared_ptr<redisContext> context_;

    // may be used in future
    ConnPool<RedisConn, RedisConnPoolHelper>& pool_;
};


#endif // _TiBANK_REDIS_CONN_H
