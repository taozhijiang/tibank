#ifndef _TiBANK_EQUEUE_HPP_
#define _TiBANK_EQUEUE_HPP_

#include <deque>

#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template<typename T>
class EQueue {
public:
    EQueue() {}
    virtual ~EQueue() {}

    void PUSH(const T& t){
        boost::unique_lock<boost::mutex> lock(lock_);
        items_.push_back(t);
        item_notify_.notify_one();
    }

    void POP(T& t) {
        t = POP();
    }

    T POP() {
        boost::unique_lock<boost::mutex> lock(lock_);
        while (items_.empty()) {
            item_notify_.wait(lock);
        }

        T t = items_.front();
        items_.pop_front();
        return t;
    }

    bool POP(T& t, uint64_t timed_out_msec) {
        boost::unique_lock<boost::mutex> lock(lock_);
        const boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(timed_out_msec);

        // if(!item_notify_.timed_wait(lock, timeout, boost::bind(&EQueue::EMPTY, this))){
        while (items_.empty()) {
            if (!item_notify_.timed_wait(lock, timeout)){
                goto check;
            }
        }

check:
        if (items_.empty()) {
            return false;
        }

        t = items_.front();
        items_.pop_front();
        return true;
    }

    // 只有t不存在的时候才添加
    bool UNIQUE_PUSH(const T& t) {
        boost::unique_lock<boost::mutex> lock(lock_);
        if (std::find(items_.begin(), items_.end(), t) == items_.end()) {
            items_.push_back(t);
            item_notify_.notify_one();
            return true;
        }
        return false;
    }

    size_t SIZE() {
        boost::unique_lock<boost::mutex> lock(lock_);
        return items_.size();
    }

    bool EMPTY() {
        boost::unique_lock<boost::mutex> lock(lock_);
        return items_.empty();
    }

private:
    boost::mutex lock_;
    boost::condition_variable item_notify_;

    std::deque<T> items_;
};


#endif // _TiBANK_EQUEUE_HPP_

