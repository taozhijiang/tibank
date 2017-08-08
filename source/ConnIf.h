#ifndef _TiBANK_CONN_IF_HPP_
#define _TiBANK_CONN_IF_HPP_

class ConnIf {

public:
    virtual void do_read() = 0;
    virtual void do_write() = 0;

    virtual void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) = 0;
    virtual void write_handler(const boost::system::error_code &ec, std::size_t bytes_transferred) = 0;

    virtual ~ConnIf();
};


#endif //_TiBANK_CONN_IF_HPP_
