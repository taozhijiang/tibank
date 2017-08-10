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
	void do_insert(const T& t) { // todo bool return
		boost::unique_lock<boost::mutex> lock(lock_);
		item_.insert(t);
	}

	size_t do_erase(const T& t) {
		boost::unique_lock<boost::mutex> lock(lock_);
		return item_.erase(t);
	}

    size_t do_exist(const T& t) {
		boost::unique_lock<boost::mutex> lock(lock_);
		return (item_.find(t) != item_.end());
	}

    size_t do_size() {
        boost::unique_lock<boost::mutex> lock(lock_);
		return item_.size();
    }

    bool do_empty() {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.empty();
    }

    void do_clear() {
        boost::unique_lock<boost::mutex> lock(lock_);
        return item_.clear();
    }

private:
	std::set<T> item_;
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
			log_error("Init BucketSet pool failed!");
			return false;
		}

		return true;
	}

	virtual ~BucketSet() {
		delete [] items_;
	}

public:
    void INSERT(const T& t){
		size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_insert(t);
	}

	size_t ERASE(const T& t) {
		size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_erase(t);
	}

    size_t EXIST(const T& t) {
		size_t index = hash_index_call_(t) & bucket_size_;
        return items_[index].do_exist(t);
	}

    size_t SIZE() {
        size_t total_size = 0;
        for (size_t i=0; i<bucket_size_; ++i)
            total_size += items_[i].do_size();

        return total_size;
    }

    bool EMPTY() {
        for (size_t i=0; i<bucket_size_; ++i)
            if (!items_[i].do_empty())
                return false;

        return true;
    }

    void CLEAR() {
        for (size_t i=0; i<bucket_size_; ++i)
            items_[i].do_clear();
    }

private:

    size_t round_to_power(size_t size) {
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
