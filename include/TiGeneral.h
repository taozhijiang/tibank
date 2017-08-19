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
#define nullptr NULL
#endif

#ifdef NP_DEBUG
#define safe_assert(x) assert(x)
#else
#define safe_assert(x)
#endif


#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>
using std::string;

#include <cstdint>
#include <linux/limits.h>  //PATH_MAX

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/thread.hpp>
#include <boost/bind.hpp>


#include <boost/asio.hpp>
using namespace boost::asio;

typedef boost::shared_ptr<ip::tcp::socket> 	socket_shared_ptr;
typedef boost::weak_ptr<ip::tcp::socket>   	socket_weak_ptr;

typedef boost::asio::posix::stream_descriptor asio_fd;
typedef boost::shared_ptr<boost::asio::posix::stream_descriptor> asio_fd_shared_ptr;



#endif // _TiBANK_GENERAL_HPP_
