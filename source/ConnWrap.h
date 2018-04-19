#ifndef _TiBANK_CONN_WRAP_H_
#define _TiBANK_CONN_WRAP_H_

#include "General.h"
#include "Log.h"

class ConnWrap {
public:
    ConnWrap():
        start_({}), count_(0), conn_uuid_(0)
	{}

    ~ConnWrap() {}

	void wrap_touch() {
		++ count_;
		::gettimeofday(&start_, NULL);
	}

	int64_t wrap_hold_ms(bool update = true) {
		struct timeval end;
        ::gettimeofday(&end, NULL);

        int64_t time_ms = ( 1000000 * ( end.tv_sec - start_.tv_sec ) + end.tv_usec - start_.tv_usec ) / 1000; // ms

		if (update) {
			start_ = end;
		}
		return time_ms;
	}

	bool wrap_conn_expired(time_t now, time_t live_sec) {
		if (now - start_.tv_sec > live_sec) {
            log_info("use count: %ld, now: %lu, last_time: %lu, linger: %dsec", count_, now, start_.tv_sec, live_sec);
			return true;
		}

		return false;
	}


    void set_uuid(int64_t uuid) { conn_uuid_ = uuid; }
    int64_t get_uuid() { return conn_uuid_; }


private:
	struct timeval start_; // 每次请求的时候更新
	int64_t        count_;
    int64_t          conn_uuid_;   // reinterpret_cast
};


#endif // _TiBANK_CONN_WRAP_H_
