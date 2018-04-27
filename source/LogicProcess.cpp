#include "Log.h"
#include "TransProcess.h"

// 自定义成功，失败规则

void process_logic_1( int& status, int& err_code,
                      int amount, const std::string& remarks, int l_status) {

    if (remarks == "succ") {
        status = TransStatus::kTransSuccess;
        err_code = TransErrInfo::kTransNoErr;
    } else if(remarks == "fail") {
        status = TransStatus::kTransFail;
        err_code = TransErrInfo::kTransSystemErr;
    } else {
        if ( amount <= 300 ){
            status = TransStatus::kTransSuccess;
            err_code = TransErrInfo::kTransNoErr;
        } else {
            status = TransStatus::kTransFail;
            err_code = TransErrInfo::kTransAmountErr;
        }
    }
}


void process_logic_2( int& status, int& err_code,
                      int amount, const std::string& remarks, int l_status) {

    if (amount >= 100*10000*100) { // 单笔不超过100W
        log_err("F_amount error: %ld", amount);
        status = TransStatus::kTransFail;
        err_code = TransErrInfo::kTransAmountErr;
        return;
    }

    if ((amount % 5) && (amount < 3000) ){
        status = TransStatus::kTransSuccess;
        err_code = TransErrInfo::kTransNoErr;
    } else {
        status = TransStatus::kTransFail;
        err_code = TransErrInfo::kTransAmountErr;
    }

#if 0
    if ((amount % 11) == 0 ) {
        log_info("Deleting order with amount: %d", F_amount);
        conn->sqlconn_execute_update(
                                va_format(" DELETE FROM %s.t_trans_order_%02d "
                                           " WHERE F_merch_id='%s' AND F_trans_id='%s' AND F_account_type=%d ",
                                            TiBANK_DATABASE_PREFIX, get_db_table_index(qd->trans_id_),
                                            qd->merch_id_.c_str(), qd->trans_id_.c_str(), qd->account_type_));
    }
#endif

    return;
}

void process_logic_3( int& status, int& err_code,
                      int amount, const std::string& remarks, int l_status) {


    if (remarks == "6100000002") {
        if ( amount <= 300 ){
            status = TransStatus::kTransSuccess;
            err_code = TransErrInfo::kTransNoErr;
        } else {
            status = TransStatus::kTransFail;
            err_code = TransErrInfo::kTransAmountErr;
        }

        return;
    }

    if ((amount % 2) && (amount < 1000000) ){
        status = TransStatus::kTransSuccess;
        err_code = TransErrInfo::kTransNoErr;
    } else {
        status = TransStatus::kTransFail;
        err_code = TransErrInfo::kTransAmountErr;
    }

    return;
}
