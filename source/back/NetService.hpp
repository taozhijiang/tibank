#ifndef _NETD_HPP_
#define _NETD_HPP_

#include "General.hpp"


#include <boost/bind.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

namespace Netd {

class Connect;
typedef boost::shared_ptr<Connect> conn_ptr;
typedef boost::weak_ptr<Connect>   conn_weakptr;


class msgNetd;
msgNetd* startMsgNetd( const std::string& addr, unsigned short port,
                       size_t worker_sz);
}



#endif // _NETD_HPP_
