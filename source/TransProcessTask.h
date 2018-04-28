#ifndef __TiBANK_PROCESS_TASK_H__
#define __TiBANK_PROCESS_TASK_H__

#include "ThreadPool.h"


// ProcessTask utils
#if 0
CREATE TABLE `t_equeue_data` (
  `F_id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `F_task_type` tinyint(5) NOT NULL COMMENT '任务类型',
  `F_task_status` tinyint(5) NOT NULL COMMENT '任务状态',
  `F_next_handle_time`  datetime DEFAULT NULL COMMENT '状态锁持续时间',
  `F_merch_id` varchar(20) NOT NULL COMMENT '商户号',
  `F_trans_id` varchar(20) NOT NULL COMMENT '付款流水号',
  `F_account_type` tinyint(5) NOT NULL COMMENT '0:对公业务，1:对私业务',
  `F_process_eqtime` bigint(20) NOT NULL COMMENT '下次处理时间，time()返回值',
  `F_process_count` int(10) unsigned DEFAULT '0' COMMENT '尝试处理次数',
  `F_create_time` datetime DEFAULT NULL COMMENT '本条目创建时间',
  `F_update_time` datetime DEFAULT NULL COMMENT '本条目更新时间',
  PRIMARY KEY (`F_id`),
  UNIQUE KEY `unique_batch` (`F_merch_id`,`F_trans_id`,`F_account_type`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

CREATE TABLE `t_equeue_data_finished` (
  `F_id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID',
  `F_task_type` tinyint(5) NOT NULL COMMENT '任务类型',
  `F_task_status` tinyint(5) NOT NULL COMMENT '任务状态',
  `F_merch_id` varchar(20) NOT NULL COMMENT '商户号',
  `F_trans_id` varchar(20) NOT NULL COMMENT '付款流水号',
  `F_account_type` tinyint(5) NOT NULL COMMENT '0:对公业务，1:对私业务',
  `F_process_count` int(10) unsigned DEFAULT '0' COMMENT '该任务尝试处理次数',
  `F_create_time` datetime DEFAULT NULL COMMENT '任务创建时间',
  `F_finish_time` datetime DEFAULT NULL COMMENT '任务完成时间',
  PRIMARY KEY (`F_id`),
  UNIQUE KEY `unique_batch` (`F_merch_id`,`F_trans_id`,`F_account_type`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;

#endif


//任务类型
enum TaskType {
    kTaskTypeUnknown      = 0,    // 未知
    kTaskTypeTransProcess = 1,  // 交易处理
};

//任务状态
enum TaskStatusType {
    kTaskStatusInit        = 0, //初始化
    kTaskStatusWaiting    = 1,  //等待处理
    kTaskStatusInProcess  = 2,  //正在处理
    kTaskStatusFinished   = 3,  //处理完成
    kTaskStatusFailed     = 4,  //处理出错, 但也不再继续处理
};


class EQueueData {
public:
    EQueueData(std::string merch_id, std::string trans_id, int account_type) :
                merch_id_(merch_id), trans_id_(trans_id), account_type_(account_type),
                process_count_(0) {
    }

    ~EQueueData() {}

    std::string merch_id_;
    std::string trans_id_;
    int          account_type_;
    int          process_count_;   //回盘接口调动计数
};

typedef std::shared_ptr<EQueueData> EQueueDataPtr;
typedef std::list<EQueueDataPtr> EQueueList;

int creat_trans_process_task(EQueueDataPtr qd);
int touch_trans_process_task(EQueueDataPtr qd);

int finish_trans_process_task(EQueueDataPtr qd, enum TaskStatusType stat);
int finish_trans_process_task(sql_conn_ptr conn, EQueueDataPtr qd, enum TaskStatusType stat);

int get_unfinished_trans_process_task(EQueueList& qlist, size_t batch_size, size_t hold_time);

int do_process_task(EQueueDataPtr qd);

class TransProcessTask: public std::enable_shared_from_this<TransProcessTask>
{
public:
    explicit TransProcessTask(uint8_t thread_num):
        thread_num_init_(thread_num), trans_process_task_(thread_num) {
    }

    virtual ~TransProcessTask() {
    }

    bool init();

private:
    void trans_process_task_run(ThreadObjPtr ptr);

public:
    // 工作线程组
    uint8_t thread_num_init_;
    ThreadPool trans_process_task_;

    int stop_graceful() {
        trans_process_task_.graceful_stop_threads();
        return 0;
    }

    int join() {
        trans_process_task_.join_threads();
        return 0;
    }

};


#endif // __TiBANK_PROCESS_TASK_H__
