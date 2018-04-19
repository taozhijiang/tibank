#include "General.h"

#include <boost/lexical_cast.hpp>

#include "Log.h"

#include "RedisConn.h"


bool RedisConn::init(int64_t conn_uuid, const RedisConnPoolHelper& helper) {

    set_uuid(conn_uuid);

    struct timeval tv {};
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	context_.reset(redisConnectWithTimeout(helper.host_.c_str(), helper.port_, tv),  redisFree);
	if (!context_) {
		log_err("create new redis connect error!");
		return false;
	}

	if (helper.passwd_.empty()) // same as == ""
		return true;

	redisReply_ptr r( (redisReply *)redisCommand(context_.get(), "AUTH %s", helper.passwd_.c_str()), freeReplyObject);
	if (!r) {
		log_err("redis conn auth error!");
		return false;
	}

	if (r->type == REDIS_REPLY_ERROR) {
		log_err("redis conn auth failed!");
		return false;
	}

	return true;
}

redisReply_ptr RedisConn::exec(const char *fmt, ...) {

	va_list ap;
    va_start(ap, fmt);
	redisReply_ptr r( (redisReply *)redisvCommand(context_.get(), fmt, ap), freeReplyObject);
    va_end(ap);

	if (!r && context_->err) {
		log_err("exec failed with: %s", context_->errstr);
	}

	return r;
}

redisReply_ptr RedisConn::exec(int argc, const char **argv) {

	redisReply_ptr r( (redisReply *)redisCommandArgv(context_.get(), argc ,argv, 0 ), freeReplyObject);

	if (!r && context_->err) {
		log_err("exec failed with: %s", context_->errstr);
	}

	return r;
}


redisReply_ptr RedisConn::eval(const std::string& script,
							const std::vector<std::string>& keys,
							const std::vector<std::string>& args) {

	redisReply_ptr r;

	if(script.empty()) {
		return r; // empty
	}

	size_t arg_num = 1 + 1 + 1 + keys.size() + args.size();  // cmd + script + keys num + keys + args
	std::vector<const char*> argvs;

	argvs.reserve(arg_num);
	const char *cmd = "EVAL";
	argvs.push_back(cmd);
	argvs.push_back(script.c_str());
	std::string str_keysnum = boost::lexical_cast<std::string>(keys.size());
	argvs.push_back(str_keysnum.c_str());

	size_t index = 0;
	for(index = 0; index < keys.size(); index ++) {  // The keys
		argvs.push_back(keys[index].c_str());
	}

	for(index = 0; index < args.size(); index ++) {
		argvs.push_back(args[index].c_str());
	}

	r.reset( (redisReply*)redisCommandArgv(context_.get(), static_cast<int>(argvs.size()), &argvs[0], NULL), freeReplyObject);
	return r;
}


RedisConn::operator bool() {
	return !!context_;
}
