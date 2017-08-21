#ifndef _TiBANK_ALIVE_TIMER_H__
#define _TiBANK_ALIVE_TIMER_H__

#include <set>

#include <boost/unordered_map.hpp> // C++11

#include "Log.h"

template<typename T>
class AliveItem {
public:
	AliveItem(time_t tm, boost::shared_ptr<T> t_ptr):
		time_(tm), raw_ptr_(t_ptr.get()), weak_ptr_(t_ptr) {
	}

	time_t get_expire_time() {
		return time_;
	}

    T* get_raw_ptr() {
        return raw_ptr_;
    }

    boost::weak_ptr<T> get_weak_ptr() {
        return weak_ptr_;
    }

private:
	time_t time_;
    T*     raw_ptr_;
	boost::weak_ptr<T> weak_ptr_;
};

template<typename T>
class AliveTimer {
public:
    typedef boost::shared_ptr<AliveItem<T> > active_item_ptr;
	typedef std::map<time_t, std::set<active_item_ptr> > Container;
	typedef std::map<T*, active_item_ptr >     HashContainer;
    typedef boost::function<int(boost::shared_ptr<T>)> ExpiredHandler;

public:

    explicit AliveTimer(time_t time_out = 10*60, time_t time_linger = 30):
		func_(), lock_(), items_(),
        time_out_(time_out), time_linger_(time_linger) {
        initialized_ = false;
	}

	explicit AliveTimer(ExpiredHandler func, time_t time_out = 10*60, time_t time_linger = 30):
		func_(func), lock_(), items_(),
        time_out_(time_out), time_linger_(time_linger) {
        initialized_ = true;
	}

    bool init(ExpiredHandler func, time_t time_out = 10*60, time_t time_linger = 30) {
        func_ = func;
        time_out_ = time_out;
        time_linger_ = time_linger;
        initialized_ = true;

        return true;
    }

	~AliveTimer(){
	}

    bool touch(boost::shared_ptr<T> ptr) {
        time_t tm = ::time(NULL) + time_out_;
        return touch(ptr, tm);
	}

	bool touch(boost::shared_ptr<T> ptr, time_t tm) {
		boost::unique_lock<boost::mutex> lock(lock_);
		typename HashContainer::iterator iter = hashed_items_.find(ptr.get());
		if (iter == hashed_items_.end()) {
			log_error("touched item not found!");
			return false;
		}

		time_t before = iter->second->get_expire_time();
        if (tm - before < time_linger_){
            log_debug("Linger optimize: %ld, %ld", tm, before);
            return true;
        }

        if (items_.find(before) == items_.end()){
            safe_assert(false);
            log_error("bucket tm: %ld not found, critical error!", before);
            return false;
        }

        if (items_.find(tm) == items_.end()) {
			items_[tm] = std::set<active_item_ptr>();
		}

		items_[tm].insert(iter->second);
        items_[before].erase(iter->second);
        log_debug("touched: %p, %ld -> %ld", ptr.get(), before, tm);
        return true;
	}

    bool insert(boost::shared_ptr<T> ptr) {
        time_t tm = ::time(NULL) + time_out_;
        return insert(ptr, tm);
    }

	bool insert(boost::shared_ptr<T> ptr, time_t tm ) {
		boost::unique_lock<boost::mutex> lock(lock_);
		typename HashContainer::iterator iter = hashed_items_.find(ptr.get());
		if (iter != hashed_items_.end()) {
			log_error("Insert item already exists: @ %ld, %p", iter->second->get_expire_time(),
                            iter->second->get_raw_ptr());
			return false;
		}

		active_item_ptr alive_item = boost::make_shared<AliveItem<T> >(tm, ptr);
        if (!alive_item){
            log_error("Create AliveItem failed!");
            return false;
        }

        hashed_items_[ptr.get()] = alive_item;

        if (items_.find(tm) == items_.end()) {
			items_[tm] = std::set<active_item_ptr>();
		}
		items_[tm].insert(alive_item);

        log_debug("inserted: %p, %ld", ptr.get(), tm);
		return true;
	}

    bool clean_up() {

        // log_debug("clean_up check ... ");
        if (!initialized_) {
            log_error("not initialized, please check ...");
            return false;
        }

        time_t now = ::time(NULL);
		boost::unique_lock<boost::mutex> lock(lock_);
        typename Container::iterator iter = items_.begin();
        typename Container::iterator remove_iter = items_.end();
        for ( ; iter != items_.end(); ){
            if (iter->first < now) {
                typename std::set<active_item_ptr>::iterator it = iter->second.begin();
                for (; it != iter->second.end(); ++it) {
                    T* p = (*it)->get_raw_ptr();
                    if (hashed_items_.find(p) == hashed_items_.end()) {
                        safe_assert(false);
                        log_error("hashed item: %p not found, critical error!", p);
                    }

                    log_debug("hash item remove: %p, %ld", p, (*it)->get_expire_time());
                    hashed_items_.erase(p);
                    boost::weak_ptr<T> weak_item = (*it)->get_weak_ptr();
                    if (boost::shared_ptr<T> ptr = weak_item.lock()) {
                        func_(ptr);
                    } else {
                        log_debug("Item %p may already release before ...", p);
                    }
                }

                // (Old style) References and iterators to the erased elements are invalidated.
                // Other references and iterators are not affected.

                log_debug("expire entry remove: %ld, now:%ld count:%ld", iter->first, now, iter->second.size());
                remove_iter = iter ++;
                items_.erase(remove_iter);
            }
            else {
                break; // all expired clean
            }
        }

        int64_t total_count = 0;
        for (iter = items_.begin() ; iter != items_.end(); ++ iter) {
            total_count += iter->second.size();
        }
        log_debug("current alived hashed count:%ld, timed_count: %ld", hashed_items_.size(), total_count);
    }

private:
	mutable boost::mutex lock_;
	Container items_;
	HashContainer hashed_items_;
    ExpiredHandler func_;
    bool initialized_;

	time_t time_out_;
	time_t time_linger_;
};


#endif // _TiBANK_ALIVE_TIMER_H__

