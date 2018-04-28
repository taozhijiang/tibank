#ifndef _TiBANK_GENERAL_HPP_
#define _TiBANK_GENERAL_HPP_

// General GNU
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#if __cplusplus > 201103L
// _built_in_
#else
#define override
#define nullptr (void*)0
#define noexcept
#endif

#include <boost/assert.hpp>

#undef NDEBUG  // needed by handler
#ifdef NP_DEBUG
// default, expand to ::assert
#else
// custom assert print but not abort, defined at Utils.cpp
#define BOOST_ENABLE_ASSERT_HANDLER
#endif
#define SAFE_ASSERT(expr) BOOST_ASSERT(expr)


#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <cstdint>
#include <linux/limits.h>  //PATH_MAX

#include <memory>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/asio.hpp>
using namespace boost::asio;

typedef std::shared_ptr<ip::tcp::socket>    SocketPtr;
typedef std::weak_ptr<ip::tcp::socket>      SocketWeakPtr;

typedef boost::asio::posix::stream_descriptor asio_fd;
typedef std::shared_ptr<boost::asio::posix::stream_descriptor> asio_fd_shared_ptr;

template<class T>
T * get_pointer(std::shared_ptr<T> const& p) {
    return p.get();
}

#endif // _TiBANK_GENERAL_HPP_
