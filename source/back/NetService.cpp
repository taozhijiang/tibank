#include "Netd/Netd.hpp"
#include "msgNetd.hpp"
#include "Message.hpp"

namespace Netd {

msgNetd* g_msgNetd = NULL;
bool SockSendBuf_SendError(P_MSG pMsg, const char *strError);
bool SockSendBuf(P_MSG pMsg);

msgNetd* startMsgNetd( const std::string& addr, unsigned short port,
                       size_t worker_sz)
{
    msgNetd *srv = new msgNetd(addr, port, worker_sz);
    g_msgNetd = srv;
    netask::all_threads_groups.create_thread(
                                    boost::bind(&msgNetd::run, srv) );

    srv->startWorker();


    return srv;
}

bool SockSendBuf_FillError(P_MSG pMsg, const char *strError) {
	unsigned int len;

	len = 3;
	sprintf(&pMsg->data_buf[len],
                "<status>error:%s</status>", strError);
	len += strlen(&pMsg->data_buf[len]);

    pMsg->data_count  = len;
    pMsg->data_buf[2] = len & 0xff;        //低位
	pMsg->data_buf[1] = (len >> 8) & 0xff; //高位

	return true;
}

bool SockSendBuf_SendError(P_MSG pMsg, const char *strError) {
	unsigned int len;

	len = 3;
	sprintf(&pMsg->data_buf[len],
                "<status>error:%s</status>", strError);
	len += strlen(&pMsg->data_buf[len]);

    pMsg->data_count  = len;
    pMsg->data_buf[2] = len & 0xff;        //低位
	pMsg->data_buf[1] = (len >> 8) & 0xff; //高位

	return SockSendBuf(pMsg);
}

bool SockSendBuf(const P_MSG pMsg)
{
    assert( (unsigned char)pMsg->data_buf[0] == 'C' );
    assert( (unsigned char)pMsg->data_buf[2] == (pMsg->data_count & 0xff) );
    assert( (unsigned char)pMsg->data_buf[1] == (pMsg->data_count >> 8) & 0xff );

    return g_msgNetd->pushToDeliver(*pMsg); //引用而非指针
}

bool SockSendBuf(const MSG& Msg)
{
    assert( (unsigned char)Msg.data_buf[0] == 'C' );
    assert( (unsigned char)Msg.data_buf[2] == (Msg.data_count & 0xff) );
    assert( (unsigned char)Msg.data_buf[1] == (Msg.data_count >> 8) & 0xff );

    return g_msgNetd->pushToDeliver(Msg); //引用而非指针
}


}

boost::shared_ptr<Netd::Connect> dummy_conn_ptr(nullptr);

_MSG::_MSG(Netd::conn_weakptr c):
    conn(c) {
    memset(data_buf, 0 , sizeof(data_buf));
    data_count = 0;
}

_MSG::_MSG():
    _MSG(dummy_conn_ptr) {
}

_MSG::_MSG(Netd::conn_weakptr c, const char* dat, size_t len):
    _MSG(c) {
    memcpy(data_buf, dat, len);
    data_count = len;
}
