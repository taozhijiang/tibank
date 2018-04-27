#ifndef _TiBANK_BUCKET_SET_H__
#define _TiBANK_BUCKET_SET_H__

// 用Bucket存放多个set，降低空间浪费和lock contention

#include <boost/function.hpp>

#include "Log.h"


template <typename T>
class SetItem {

    typedef std::set<T> Container;

public:
    // 返回true表示实际插入了
    bool do_insert(const T& t) {
        boost::unique_lock<boost::mutex> lock(lock_);
        auto ret = item_.insert(t);
        return ret.second;
    }

    // 返回实际被删除的元素个数
    size_t do_erase(const T& t) {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.erase(t);
    }

    size_t do_exist(const T& t) const {
        boost::unique_lock<boost::mutex> lock(lock_);
        return (item_.find(t) != item_.end());
    }

    size_t do_size() const {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.size();
    }

    bool do_empty() const {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.empty();
    }

    void do_clear() {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.clear();
    }

private:
    Container item_;
    boost::mutex lock_;
};


template<typename T>
class BucketSet {
public:
    explicit BucketSet(size_t bucket_size, boost::function<size_t(const T&)> call):
        bucket_size_(round_to_power(bucket_size)),
        hash_index_call_(call),
        items_(NULL) {
        log_debug("real bucket size: %x", bucket_size_);
    }

    bool init() {
        items_ = new (std::nothrow) SetItem<T>[bucket_size_];
        if (!items_) {
            log_err("Init BucketSet pool failed!");
            return false;
        }

        return true;
    }

    virtual ~BucketSet() {
        delete [] items_;
    }

public:
    bool INSERT(const T& t) const {
        size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_insert(t);
    }

    size_t ERASE(const T& t) const {
        size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_erase(t);
    }

    size_t EXIST(const T& t) const {
        size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_exist(t);
    }

    size_t SIZE() const {
        size_t total_size = 0;
        for (size_t i=0; i<bucket_size_; ++i)
            total_size += items_[i].do_size();

        return total_size;
    }

    bool EMPTY() const {
        for (size_t i=0; i<bucket_size_; ++i)
            if (!items_[i].do_empty())
                return false;

        return true;
    }

    void CLEAR() const {
        for (size_t i=0; i<bucket_size_; ++i)
            items_[i].do_clear();
    }

private:

    size_t round_to_power(size_t size) const {
        size_t ret = 0x1;
        while (ret < size) { ret = (ret << 1) + 1; }

        return ret;
    }

    size_t bucket_size_;
    boost::function<size_t(const T&)> hash_index_call_;

    // STL容器需要拷贝，但是boost::mutex是不可拷贝的，此处只能动态申请数组
    SetItem<T>* items_;
};



#endif // _TiBANK_BUCKET_SET_H__

