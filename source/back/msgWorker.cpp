#include "msgWorker.hpp"
#include "msgNetd.hpp"

namespace Netd {

extern msgNetd* g_msgNetd;
bool SockSendBuf_SendError(P_MSG pMsg, const char *strError);
bool SockSendBuf(const P_MSG pMsg);
bool SockSendBuf(const MSG& msg);


size_t msgWorker::WORKER_ID = 1;

void msgWorker::run() {

    BOOST_LOG_T(info) << "msgWorker Thread started with id:" << id_ << "!";

    MSG Msg;
    P_MSG pMsg(&Msg);

    while (true) {
        if (tasks_.popMsg(Msg)) {
            BOOST_LOG_T(debug) << "msgWorker[" << id_ <<"] RECV:" << Msg.data_buf;

            // 数据处理 ...
            // ...

            SockSendBuf_SendError(pMsg, "This is the server response!");
        }
    }
}

} // END NAMESPACE

