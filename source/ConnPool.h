#ifndef _TiBANK_CONN_POOL_H_
#define _TiBANK_CONN_POOL_H_

#include "General.h"

#include <set>
#include <deque>
#include <numeric>

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>

//#include <boost/atomic.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "TimerService.h"
#include "Log.h"

template <typename T>
struct conn_ptr_compare {
public:
	typedef std::shared_ptr<T> ConnPtr;

    bool operator() (const ConnPtr& lhs,
                       const ConnPtr& rhs) const {
        return (lhs.get() < rhs.get());
    }
};

template <typename T, typename Helper>
class ConnPool: public boost::noncopyable,
			      public std::enable_shared_from_this<ConnPool<T, Helper> >
{
public:
	typedef std::shared_ptr<T> ConnPtr;
	typedef std::weak_ptr<T>   ConnWeakPtr;
	typedef std::set<ConnPtr, conn_ptr_compare<T> > ConnContainer;

public:
    explicit ConnPool(std::string pool_name, size_t capacity, Helper helper, time_t linger_sec = 0):
    	pool_name_(pool_name), capacity_(capacity),
		helper_(helper), conns_busy_(), conns_idle_(),
		conn_notify_(), conn_notify_mutex_(),
        acquired_count_(0), acquired_ok_count_(0),
        hold_time_ms_(512),
    	conn_pool_stats_timer_id_(0),
		conn_pool_linger_sec_(linger_sec),
		conn_pool_linger_trim_id_(0) {

		safe_assert(capacity_);
        log_info( "ConnPool Maxium Capacity: %d", capacity_ );
		return;
	}

	bool init() {
		conn_pool_stats_timer_id_ = TimerService::instance().register_timer_task(
				boost::bind(&ConnPool::show_conn_pool_stats, this->shared_from_this()), 60 * 1000/* 60s */, true, true);
		if (conn_pool_stats_timer_id_ == 0) {
			log_err("Register conn_pool_stats_timer failed! ");
			return false;
		}

		if (conn_pool_linger_sec_) {
			log_info("We will try to trim idle connection %lu sec.", conn_pool_linger_sec_);

			conn_pool_linger_trim_id_ = TimerService::instance().register_timer_task(
				boost::bind(&ConnPool::do_conn_pool_linger_trim, this->shared_from_this()), 10 * 1000/* 10s */, true, false);
			if (conn_pool_linger_trim_id_ == 0) {
				log_err("Register do_conn_pool_linger_trim failed! ");
				return false;
			}
		}

		return true;
	}

    // 由于会返回nullptr，所以不能返回引用
    ConnPtr request_conn() {

		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

		while (!do_check_available()) {
			conn_notify_.wait(lock);
		}

		return do_request_conn();
	}

    ConnPtr try_request_conn(size_t msec)  {

		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);
		ConnPtr conn;

		if (!do_check_available() && !msec)
			return conn; // nullptr

		// timed_wait not work with 0
		if(do_check_available() || conn_notify_.timed_wait(lock, boost::posix_time::milliseconds(msec))) {
			typename ConnContainer::iterator it;
			return do_request_conn();
		}

		return conn;
	}

    bool request_scoped_conn(ConnPtr& scope_conn) {

		// reset first, all will stack at reset latter...
		// probably recursive require conn_nofity_mutex problem

		scope_conn.reset();
		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

		while (!do_check_available()) {
			conn_notify_.wait(lock);
		}

		ConnPtr conn = do_request_conn();
		if (conn) {
			scope_conn.reset(conn.get(),
							 boost::bind(&ConnPool::free_conn,
							 this, conn)); // 还是通过智能指针拷贝一份吧
			// log_debug("Request guard connection: %ld", scope_conn->get_uuid());
			return true;
		}

		return false;
	}

    void free_conn(ConnPtr conn) {

		{
			boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);

			conns_idle_.push_back(conn);
			conns_busy_.erase(conn);
			hold_time_ms_.push_back(conn->wrap_hold_ms());

			// log_debug("Freeing %ld conn", conn->get_uuid());
		}

		conn_notify_.notify_all();
		return;
	}


    size_t get_conn_capacity() const { return capacity_; }

    ~ConnPool() {

		TimerService::instance().revoke_timer_task(conn_pool_stats_timer_id_);

	}

private:

    ConnPtr do_request_conn() {

		ConnPtr conn;
		++ acquired_count_;

		if (!conns_idle_.empty()){
			conn = conns_idle_.front();
			conns_idle_.pop_front();
			conns_busy_.insert(conn);
			++ acquired_ok_count_;
			conn->wrap_touch();
			return conn;
		}

		if ( (conns_idle_.size() + conns_busy_.size()) < capacity_) {

			ConnPtr new_conn = std::make_shared<T>(*this);
			if (!new_conn){
				log_err("creating new Conn failed!");
				return new_conn;
			}

			if (!new_conn->init(reinterpret_cast<int64_t>(new_conn.get()), helper_)) {
				log_err("init new Conn failed!");
				new_conn.reset();
				return new_conn;
			}

			conns_busy_.insert(new_conn);
			++ acquired_ok_count_;
			new_conn->wrap_touch();
			return new_conn;
		}

		return conn;
	}

    bool do_check_available() {
        return (!conns_idle_.empty() || (conns_idle_.size() + conns_busy_.size()) < capacity_ );
    }


	std::string pool_name_;
	size_t capacity_;       // 总连接数据限制


	const Helper helper_;

    std::set<ConnPtr, conn_ptr_compare<T> > conns_busy_;
    std::deque<ConnPtr> conns_idle_;

    boost::condition_variable_any conn_notify_;
    boost::mutex conn_notify_mutex_;

	// 状态和统计，下面这些变量都是用上面的mutex保护
	uint64_t acquired_count_;   // 总请求计数
	uint64_t acquired_ok_count_; // 成功获取计数
	boost::circular_buffer<int64_t> hold_time_ms_; // 使用时长ms，1000个采样率

	int64_t conn_pool_stats_timer_id_;
	void show_conn_pool_stats() {

		std::stringstream output;

		output << "[" << pool_name_ << "] PoolStats: " << std::endl;

		{   // hold lock
			boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);

			output << "capacity: " << capacity_ << ", acquired_count: " << acquired_count_ << ", ok_count: " << acquired_ok_count_;

			if (likely(acquired_count_)) {
				output << ", success_ratio: " << 100 * (static_cast<double>(acquired_ok_count_) / acquired_count_) << "%" << std::endl;
			}

			if (likely(!hold_time_ms_.empty())) {
				int64_t sum_out = std::accumulate(hold_time_ms_.begin(), hold_time_ms_.end(), 0);
				output << "average hold time: " << sum_out / hold_time_ms_.size() << " ms, ";
			}

			output << "current busy: " << conns_busy_.size() << ", idle: " << conns_idle_.size() << std::endl;
		}

		log_debug("%s", output.str().c_str());
	}


private:
	time_t conn_pool_linger_sec_;   // 秒为单位，超过时长的连接被剔除
	int64_t conn_pool_linger_trim_id_;
	void do_conn_pool_linger_trim() {

		struct timeval start_time;
		::gettimeofday(&start_time, NULL);

		boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);

		if (conns_idle_.empty()){
			return;
		}

		int count = 0;
		int trim_count = 0;

		for (auto iter = conns_idle_.begin(); iter != conns_idle_.end(); ) {

			++count;

			// gettimeofday的秒和time的秒是一样的
			if ((*iter)->wrap_conn_expired(start_time.tv_sec, conn_pool_linger_sec_)) {
				iter = conns_idle_.erase(iter);
				++ trim_count;
			} else {
				++ iter;
			}

			if ((count % 20) == 0) {  // 不能卡顿太长时间
				struct timeval now;
				::gettimeofday(&now, NULL);
				int64_t elapse_ms = ( 1000000 * ( now.tv_sec - start_time.tv_sec ) + now.tv_usec - start_time.tv_usec ) / 1000;
				if (elapse_ms > 20) {
					log_notice("too long elapse time: %ld ms, break now", elapse_ms);
					break;
				}
			}
		}

		log_info("pool %s, total checked %d conns, trimed %d conns.", pool_name_.c_str(), count, trim_count);

		return;
	}

};


#endif  // _TiBANK_CONN_POOL_H_
