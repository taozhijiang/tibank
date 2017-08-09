#ifndef _MSG_POOL_HPP_
#define _MSG_POOL_HPP_

#include "General.hpp"
#include "Netd/Netd.hpp"
#include <vector>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace netask {

// 为了防止内存碎片，这里都是值拷贝
template<typename T>
class msgPool: private boost::noncopyable {
public:
	const size_t MSG_COUNT_;

    explicit msgPool(size_t cnt) :
        MSG_COUNT_(cnt),
        push_idx_(0), pop_idx_(cnt - 1),
        messages_(cnt),
        dummy_item_()
    {
        assert(MSG_COUNT_);
        BOOST_LOG_T(info) << "Allocated msgPool type " << typeid(T).name() << ", count:" << messages_.size();
        assert(isEmpty());
    }

    // 带有初始值的
    msgPool(size_t cnt, const T& item) :
        MSG_COUNT_(cnt),
        push_idx_(0), pop_idx_(cnt - 1),
        messages_(cnt, item),
        dummy_item_(item)
    {
        assert(MSG_COUNT_);
        BOOST_LOG_T(info) << "Allocated msgPool type " << typeid(T).name() << ", count:" << messages_.size();
        assert(isEmpty());
    }

    ~msgPool() {
        BOOST_LOG_T(info) << "Freeing msgPool type " << typeid(T).name() << ", count:" << messages_.size();
    }

    // 当oride == true的时候，如果此时队列已满，会自动将最旧的
    // 条目删除，并将当前记录插入之
    bool pushMsg(const T& item, bool oride = false) {

        boost::unique_lock<boost::mutex> lock(msgMutex_);
        if (oride) {
            if ( ((push_idx_ + 1) % MSG_COUNT_) == pop_idx_ ) {
                pop_idx_ = (++pop_idx_) % MSG_COUNT_;
                BOOST_LOG_T(info) << "!!Force to remove old item: " << pop_idx_;
            }
        }
        else {
            while (((push_idx_ + 1) % MSG_COUNT_) == pop_idx_)
                msgPopNotify_.wait(lock);
        }

        assert(lock.owns_lock());
        assert( ((push_idx_ + 1) % MSG_COUNT_) != pop_idx_ );

        messages_[push_idx_] = item;
        BOOST_LOG_T(debug) << "PUSHED NEW MSG at " << push_idx_;

        push_idx_ = (++push_idx_) % MSG_COUNT_;
        msgPushNotify_.notify_one();

        return true;
    }

    bool try_pushMsg(const T& item) {

        boost::unique_lock<boost::mutex> lock(msgMutex_);
        if ( ((push_idx_+1)%MSG_COUNT_) == pop_idx_) {
            BOOST_LOG_T(info) << "msgQueue is full: " << push_idx_ << " - " << pop_idx_;
            return false;
        }

        assert(lock.owns_lock());
        assert( ((push_idx_ + 1) % MSG_COUNT_) != pop_idx_ );

        messages_[push_idx_] = item;
        BOOST_LOG_T(debug) << "PUSHED NEW MSG at " << push_idx_;

        push_idx_ = (++push_idx_) % MSG_COUNT_;
        msgPushNotify_.notify_one();

        return true;
    }

    bool popMsg(T& store) {

        boost::unique_lock<boost::mutex> lock(msgMutex_);
        while ( ((pop_idx_ + 1) % MSG_COUNT_ ) == push_idx_ )
            msgPushNotify_.wait(lock);

        assert(lock.owns_lock());
        assert( ((pop_idx_ + 1) % MSG_COUNT_) != push_idx_ );

        pop_idx_ = (++pop_idx_) % MSG_COUNT_;

        store = messages_[pop_idx_];
        messages_[pop_idx_] = dummy_item_;
        BOOST_LOG_T(debug) << "POPED MSG at " << pop_idx_ ;

        msgPopNotify_.notify_one();

        return true;
    }

    bool try_popMsg(T& store) {

        boost::unique_lock<boost::mutex> lock(msgMutex_);
        if ( ( (pop_idx_+1)%MSG_COUNT_ ) == push_idx_ )
            return false;

        assert(lock.owns_lock());
        assert( ((pop_idx_ + 1) % MSG_COUNT_) != push_idx_ );

        pop_idx_ = (++pop_idx_) % MSG_COUNT_;

        store = messages_[pop_idx_];
        messages_[pop_idx_] = dummy_item_;
        BOOST_LOG_T(debug) << "POPED MSG at " << pop_idx_ ;

        msgPopNotify_.notify_one();

        return true;
    }

    size_t getMsgCount() {
        boost::unique_lock<boost::mutex> lock(msgMutex_);
        return ((push_idx_ + MSG_COUNT_ ) - pop_idx_ - 1) % MSG_COUNT_;
    }

    size_t getMsgCapacity() {
        boost::unique_lock<boost::mutex> lock(msgMutex_);
        return (MSG_COUNT_ - 1);
    }

    bool isEmpty() {
        return (getMsgCount() == 0);
    }

    bool isFull() {
        return (getMsgCount() == getMsgCapacity() );
    }

private:
    size_t push_idx_; //将要插入的位置
    size_t pop_idx_;  //下个位置是弹出的位置
    std::vector<T> messages_;
    const T dummy_item_;

    boost::mutex msgMutex_;
    boost::condition_variable_any msgPushNotify_;
    boost::condition_variable_any msgPopNotify_;
};

}


#endif //_MSG_POOL_HPP_
