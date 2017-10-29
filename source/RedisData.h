#ifndef __TiBANK_REDIS_DATA_H__
#define __TiBANK_REDIS_DATA_H__

#include <string>

#include <boost/noncopyable.hpp>

#include "RedisConn.h"
#include "TimerService.h"


/**
 * 主要用于统计使用，比如MQ消费每个消息的时间，每个RPC调用返回的时间
 * 格式：bankpay_[MQ/RPC]_[key]_20170708120023
 * 统计事件是以秒为单位的， 值以ms为单位
 * */

class RedisData: public boost::noncopyable {
public:
	static RedisData& instance();

public:
	bool init();

	int calc_item_last_min(const std::string class_key, const std::string item_key,
						   int64_t& count, int64_t& average) {
		return calc_item(class_key, item_key, last_min(), count, average);
	}

	int get_item_last_min(const std::string class_key, const std::string item_key,
						  int64_t& count) {
		return get_item(class_key, item_key, last_min(), count);
	}

private:

	std::string last_min() {
		time_t tt = ::time(NULL) - 60; // 上一分钟
		tm* t= localtime(&tt);
		char time_buf[128] = {0,};
		sprintf(time_buf, "%d%2d%2d%2d%2d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
		return time_buf;
	}

	int calc_item(const std::string& class_key, const std::string& item_key, const std::string& stime,
				 int64_t& count, int64_t& average);
	int get_item(const std::string& class_key, const std::string& item_key, const std::string& stime,
				 int64_t& value);

private:
	RedisData(){}
	~RedisData(){}
};

#endif // __PBI_REDIS_DATA_H__

