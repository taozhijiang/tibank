#ifndef _TiBANK_CONN_POOL_H_
#define _TiBANK_CONN_POOL_H_

#include "TiGeneral.h"

#include <set>
#include <deque>

#include <boost/noncopyable.hpp>

//#include <boost/atomic.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "ServiceManager.h"
#include "TimerService.h"
#include "Log.h"

template <typename T>
struct conn_ptr_compare {
public:
	typedef boost::shared_ptr<T> conn_ptr;

    bool operator() (const conn_ptr& lhs,
                     const conn_ptr& rhs) const {
        return (lhs.get() < rhs.get());
    }
};

template <typename T, typename Helper>
class ConnPool: public boost::noncopyable,
			    public boost::enable_shared_from_this<ConnPool<T, Helper> >
{
public:
	typedef boost::shared_ptr<T> conn_ptr;
	typedef boost::weak_ptr<T>   conn_weak_ptr;
	typedef std::set<conn_ptr, conn_ptr_compare<T> > ConnContainer;

public:
    explicit ConnPool(std::string pool_name, size_t capacity, Helper helper):
    	pool_name_(pool_name), capacity_(capacity),
        acquired_count_(0), acquired_ok_count_(0), helper_(helper),
        conns_busy_(), conns_idle_(),
    	conn_pool_stats_timer_id_(0) {

		safe_assert(capacity_);
		log_info( "TOTAL CONN Capacity: %d", capacity_ );
		return;
	}

	bool init() {
		conn_pool_stats_timer_id_ = ServiceManager::instance().timer_service_ptr_->register_timer_task(
				boost::bind(&ConnPool::show_conn_pool_stats, this->shared_from_this()), 60 * 1000/* 60s */, true, true);
		if (conn_pool_stats_timer_id_ == 0) {
			log_err("Register conn_pool_stats_timer failed! ");
			return false;
		}

		return true;
	}

    // 由于会返回nullptr，所以不能返回引用
    conn_ptr request_conn() {

		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

		while (!do_check_available()) {
			conn_notify_.wait(lock);
		}

		return do_request_conn();
	}

    conn_ptr try_request_conn(size_t msec)  {

		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);
		conn_ptr conn;

		if (!do_check_available() && !msec)
			return conn; // nullptr

		// timed_wait not work with 0
		if(do_check_available() || conn_notify_.timed_wait(lock, boost::posix_time::milliseconds(msec))) {
			typename ConnContainer::iterator it;
			return do_request_conn();
		}

		return conn;
	}

    bool request_scoped_conn(conn_ptr& scope_conn) {

		// reset first, all will stack at reset latter...
		// probably recursive require conn_nofity_mutex problem

		scope_conn.reset();
		boost::unique_lock<boost::mutex> lock(conn_notify_mutex_);

		while (!do_check_available()) {
			conn_notify_.wait(lock);
		}

		conn_ptr conn = do_request_conn();
		if (conn) {
			scope_conn.reset(conn.get(),
							 boost::bind(&ConnPool::free_conn,
							 this, conn)); // 还是通过智能指针拷贝一份吧
			// log_debug("Request guard connection: %ld", scope_conn->get_uuid());
			return true;
		}

		return false;
	}

    void free_conn(conn_ptr conn) {

		{
			boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);

			conns_idle_.push_back(conn);
			conns_busy_.erase(conn);

			// log_debug("Freeing %ld conn", conn->get_uuid());
		}

		conn_notify_.notify_all();
		return;
	}


    size_t get_conn_capacity() const { return capacity_; }

    ~ConnPool() {
		ServiceManager::instance().timer_service_ptr_->revoke_timer_task(conn_pool_stats_timer_id_);
	}

private:

    conn_ptr do_request_conn() {

		conn_ptr conn;
		++ acquired_count_;

		if (!conns_idle_.empty()){
			conn = conns_idle_.front();
			conns_idle_.pop_front();
			conns_busy_.insert(conn);
			++ acquired_ok_count_;
			return conn;
		}

		if ( (conns_idle_.size() + conns_busy_.size()) < capacity_) {

			conn_ptr new_conn = boost::make_shared<T>(*this);
			if (!new_conn){
				log_err("Creating new Conn failed!");
				return new_conn;
			}

			if (!new_conn->init(reinterpret_cast<int64_t>(new_conn.get()), helper_)) {
				log_err("init new Conn failed!");
				new_conn.reset();
				return new_conn;
			}

			conns_busy_.insert(new_conn);
			++ acquired_ok_count_;
			return new_conn;
		}

		return conn;
	}

    bool do_check_available() {
        return (!conns_idle_.empty() || (conns_idle_.size() + conns_busy_.size()) < capacity_ );
    }

    boost::condition_variable_any conn_notify_;
    boost::mutex conn_notify_mutex_;

	std::string pool_name_;
	size_t capacity_;       // 总连接数据限制

	// boost::atomic<int64_t> acquired_count_;   // 总请求计数
	// boost::atomic<int64_t> acquired_ok_count; // 成功获取计数

	uint64_t acquired_count_;   // 总请求计数
	uint64_t acquired_ok_count_; // 成功获取计数

	const Helper helper_;

    std::set<conn_ptr, conn_ptr_compare<T> > conns_busy_;
    std::deque<conn_ptr> conns_idle_;

	int64_t conn_pool_stats_timer_id_;
	void show_conn_pool_stats() {

		std::stringstream output;

		output << "[" << pool_name_ << "] PoolStats: " << std::endl;
		output << "capacity: " << capacity_ << ", acquired_count: " << acquired_count_ << ", ok_count: " << acquired_ok_count_;

		if (likely(acquired_count_)) {
			output << ", success_ratio: " << 100 * (static_cast<double>(acquired_ok_count_) / acquired_count_) << "%" << std::endl;
		}

		{
			boost::lock_guard<boost::mutex> lock(conn_notify_mutex_);
			output << "current busy: " << conns_busy_.size() << ", idle: " << conns_idle_.size() << std::endl;
		}

		std::cerr << output.str() << std::endl;
		log_debug("%s", output.str().c_str());
	}
};


#endif  // _TiBANK_CONN_POOL_H_
