#ifndef _TiBANK_TRANS_PROCESS_H_
#define _TiBANK_TRANS_PROCESS_H_

#include <string>

enum TransStatus {
    kTransSubmitFail = 1,
	kTransInProcess = 2,
    kTransSuccess = 3,
    kTransFail = 4,
};

std::string get_trans_status_str(enum TransStatus stat);

enum TransErrCode {
    kTransErrOK = 0,
    kTransErrDup = 1,
    kTransErrParam = 2,
    kTransErrBalance = 3,
    kTransErrNotFound = 4,
    kTransErrUnknow = 5,
};

std::string get_trans_err_str(enum TransErrCode code);

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

    int32_t      trans_status;
    std::string trans_status_str;
    int32_t      trans_err_code;
    std::string trans_err_str;
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
    std::string merch_id;
    std::string merch_name;
    std::string message_type;
    std::string trans_id;
    std::string account_no;
    std::string account_name;

    int32_t      trans_status;
    std::string trans_status_str;
    int32_t      trans_err_code;
    std::string trans_err_str;
};


int process_trans_submit(const struct trans_submit_request& req, struct trans_submit_response& ret);
int generate_trans_submit_ret(const trans_submit_response& ret, std::string& post_body);

int process_trans_query(const struct trans_query_request& req, struct trans_query_response& ret);
int generate_trans_query_ret(const trans_query_response& ret, std::string& post_body);

#endif //_TiBANK_TRANS_PROCESS_H_
