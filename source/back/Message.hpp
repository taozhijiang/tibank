#ifndef _MESSAGE_HPP_
#define _MESSAGE_HPP_

#include <stdio.h>

#include "General.hpp"

typedef struct _MSG
{
    char 	data_buf[netask::MAX_MSG_SIZE]; //100K
	size_t 	data_count;

    Netd::conn_weakptr conn;

    // Others may needed
    /// ...

    explicit _MSG(Netd::conn_weakptr c);
    _MSG();
    _MSG(Netd::conn_weakptr c, const char* dat, size_t len);

} MSG;

typedef boost::shared_ptr<MSG> P_MSG;

#endif //_MESSAGE_HPP_
