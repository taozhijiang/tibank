#include <string>


#include "Utils.h"
#include "RedisData.h"
#include "SrvManager.h"


static const std::string kRedisPrefix = "bankpay";

// 主要进行健康数据统计使用

RedisData& RedisData::instance() {
    static RedisData handler;
    return handler;
}

bool RedisData::init() {
    safe_assert( SrvManager::instance().redis_pool_ptr_);
    return true;
}

// Implementation

int RedisData::calc_item(const std::string& class_key, const std::string& item_key,
                         const std::string& stime, int64_t& count, int64_t& average) {
    int nResult = 0;

    do {

        std::stringstream realKey;
        realKey << kRedisPrefix << "_" << class_key << "_" << item_key << "_" << stime;

        try {

            redis_conn_ptr conn;
            request_scoped_redis_conn(conn);

            if (!conn) {
                log_err("cannot get redis conn!");
                nResult = -1;
                break;
            }

            redisReply_ptr r = conn->exec("LRANGE %s 0 -1", realKey.str().c_str());
            log_debug("LRANGE %s 0 -1", realKey.str().c_str());

            if (!r) {
                log_err("get redis resp error!");
                nResult = -2;
                break;
            }

            if (r->type == REDIS_REPLY_ERROR){
                log_err("err data:%d ", r->type);
                nResult = -3;
                break;
            }

            if (r->elements == 0) {
                log_debug("info empty");
                nResult = -4;
                break;
            }

            int64_t sum = 0;
            count = r->elements;
            for(size_t i = 0; i < r->elements; i++) {
                std::string strCount(r->element[i]->str, r->element[i]->len);
                sum += ::atoll(strCount.c_str());
            }
            average = sum / r->elements;

            log_debug("for %s, len:%ld, avarge:%ld", realKey.str().c_str(), count, average);


        } catch(...) {
            log_err("Redis error!!!");
            nResult = -5;
        }

    } while (0);

    return nResult;
}


int RedisData::get_item(const std::string& class_key, const std::string& item_key,
                        const std::string& stime, int64_t& value) {

    int nResult = 0;

    do {
        std::stringstream realBucket;
        realBucket << kRedisPrefix << "_" << class_key << "_" << item_key << "_INCRITEM";

        try {

            redis_conn_ptr conn;
            request_scoped_redis_conn(conn);

            if (!conn) {
                log_err("cannot get redis conn!");
                nResult = -1;
                break;
            }

            redisReply_ptr r = conn->exec("hmget %s %s", realBucket.str().c_str(), stime.c_str());
            log_debug("hmget %s %s", realBucket.str().c_str(), stime.c_str());
            if (!r) {
                log_err("Get reply failed!!!");
                nResult = -2;
                break;
            }

            if (r->type == REDIS_REPLY_ERROR || r->elements != 1){
                log_err("err data:%d ", r->type);
                nResult = -3;
                break;
            }

            if (r->element[0]->len == 0) {
                // log_debug("Key empty! set value=0");
                value = 0;
            } else {
                std::string strCount(r->element[0]->str, r->element[0]->len);
                value = ::atoll(strCount.c_str());
            }
        }
        catch(...)
        {
            log_err("error!!!");
            return -4;
        }

    } while (0);

    return nResult;
}
