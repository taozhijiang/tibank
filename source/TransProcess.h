#ifndef _TiBANK_TRANS_PROCESS_H_
#define _TiBANK_TRANS_PROCESS_H_

#include <string>
#include <vector>

#include "Log.h"

// 传输信息
enum TransResponseCode {
    kTransResponseOK = 0,
    kTransResponseFail = 1,
};

// 订单状态信息
enum TransStatus {
    kTransSubmitFail = 1,
    kTransSuccess = 2,
    kTransFail = 3,
    kTransInProcess = 4,
    kTransNotFound = 5,
    kTransNonsense = 6,
};

std::string get_trans_status_str(int code);


// 额外的错误信息，提交、回盘时候都可以提供
enum TransErrInfo {
    kTransNoErr = 0,
    kTransDupReqErr = 1,
    kTransParamErr = 2,
    kTransBalanceErr = 3,
    kTransAmountErr = 4,
    kTransSystemErr = 5,
    kTransUnknownErr = 6,
};

std::string get_trans_error_str(int code);

//////////////////////////////
//
// 单笔接口
//
/////////////////////////////

struct trans_submit_request {
    std::string merch_id;
    std::string merch_name;

    std::string trans_id;
    std::string account_no;
    std::string account_name;
    int64_t      amount;
    int32_t      account_type; // 1对私
    std::string branch_name;
    std::string branch_no;
    std::string remarks;
};

struct trans_submit_response {
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    std::string trans_id;
    std::string account_no;
    std::string account_name;

    mutable int32_t      trans_response;
    int32_t      trans_status;
    mutable std::string trans_status_str;
    int32_t      trans_err_code;
    mutable std::string trans_err_str;
};



struct trans_query_request {
    std::string merch_id;
    std::string merch_name;

    std::string trans_id;
    std::string account_no;
    std::string account_name;
    int32_t      account_type; // 1对私
};

struct trans_query_response {
    std::string merch_id;                // batch miss it!
    std::string merch_name;              // batch miss it!
    std::string message_type;            // batch miss it!
    std::string trans_id;
    std::string account_no;
    std::string account_name;
    int64_t      amount;

    mutable int32_t      trans_response;  // batch miss it!

    int32_t      trans_status;
    mutable std::string trans_status_str;
    int32_t      trans_err_code;
    mutable std::string trans_err_str;
};

//////////////////////////////
//
// 批量接口
//
/////////////////////////////

struct trans_batch_submit_request {
    std::string merch_id;
    std::string merch_name;
    int64_t      total_count;
    int64_t      total_amount;

    std::vector<trans_submit_request> orders;
};

struct trans_batch_submit_response {
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    int64_t      total_count;
    int64_t      total_amount;

    mutable int32_t      trans_response;

    int32_t      trans_status;
    mutable std::string trans_status_str;
    int32_t      trans_err_code;
    mutable std::string trans_err_str;
};

struct trans_batch_query_request {
    std::string merch_id;
    std::string merch_name;
    int64_t      total_count;
    int64_t      total_amount;

    std::vector<trans_query_request> orders;
};

struct trans_batch_query_response {
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    int64_t      total_count;
    int64_t      total_amount;

    mutable int32_t      trans_response;

    std::vector<trans_query_response> orders;
};


int process_trans_submit(const struct trans_submit_request& req, struct trans_submit_response& ret);
int generate_trans_submit_ret(const trans_submit_response& ret, std::string& post_body);

int process_trans_query(const struct trans_query_request& req, struct trans_query_response& ret);
int generate_trans_query_ret(const trans_query_response& ret, std::string& post_body);

int generate_trans_batch_submit_ret(const trans_batch_submit_response& ret, std::string& post_body);
int generate_trans_batch_query_ret(const trans_batch_query_response& ret, std::string& post_body);

#define CHECK_CHAR(x) ( x >= '0' && x <='9' )

inline void get_db_table_index(std::string trans_id, int& tb1, int& tb2) {

    if (trans_id.size() >= 2 && CHECK_CHAR(trans_id[trans_id.size()-1]) && CHECK_CHAR(trans_id[trans_id.size()-2])) {
        tb1 = trans_id[trans_id.size()-2] - '0';
        tb2 = trans_id[trans_id.size()-1] - '0';
    } else {
        log_err("Invalid strPartnerOrderId %s ", trans_id.c_str());
        tb1 = 0; tb2 = 0;
    }
}

inline int get_db_table_index(std::string trans_id) {

    if (trans_id.size() >= 2 && CHECK_CHAR(trans_id[trans_id.size()-1]) && CHECK_CHAR(trans_id[trans_id.size()-2])) {
        return 0;
    }

    return (trans_id[trans_id.size()-2] - '0')*10 + (trans_id[trans_id.size()-1] - '0');
}
#undef CHECK_CHAR

#endif //_TiBANK_TRANS_PROCESS_H_
