#include <sstream>

#include "MqConn.h"


MqConn::MqConn(ConnPool<MqConn, MqConnPoolHelper>& pool):
    t_(-1), mq_(""),
    state_(MqWorkState::kProducer), //default
    pool_(pool) {
}

bool MqConn::init(int64_t conn_uuid,  const MqConnPoolHelper& helper) {

    set_uuid(conn_uuid);
    helper_ = helper;

    if (helper_.connect_urls_.empty()) {
        log_err("empty connect urls ...");
        return false;
    }
    mq_ = AMQP::RabbitMQHelper(helper_.connect_urls_);
    if (mq_.doConnect() < 0) {
        log_err("RabbitMQ connect error!");
        return false;
    }

    std::string version = mq_.brokerVersion();
    if (version.empty()) {
        log_err("RabbitMQ retrive broker version error!");
        return false;
    }

    log_info("RabbitMQ Broker Version: %s", version.c_str());

    int retCode = 0;
    if (state_ == MqWorkState::kProducer) {
        retCode = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_publish, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
    } else if (state_ == MqWorkState::kConsumer) {
        retCode = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_consume, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
    } else if (state_ == MqWorkState::kGetter) {
        retCode = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_get, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
    } else {
        log_err("Unknown work mode here: %d", static_cast<int>(state_));
        return false;
    }

    if (retCode != 0) {
        log_err("setup mq connection failed ... ");
        return false;
    }

    log_info("create new mq connection OK!");
    return true;
}

MqConn::~MqConn() {

    /* reset to fore delete, actually not need */

    log_info("destruct mq connection %ld OK!", get_uuid());
}



bool MqConn::mq_setup_channel_publish(AMQP::RabbitChannelPtr pChannel, void* pArg) {

    if (!pChannel || !pArg) {
        log_err("RabbitChannelPtr or pArg argument nullptr!");
        return false;
    }

    rabbitmq_character_t* p_rabbitmq = static_cast<rabbitmq_character_t *>(pArg);
    if (p_rabbitmq->exchange_name_.empty() || p_rabbitmq->queue_name_.empty() || p_rabbitmq->route_key_.empty()){
       log_err("required connection info missing...");
       return false;
    }

    if (pChannel->declareExchange(p_rabbitmq->exchange_name_, "direct", false/*passive*/, true/*durable*/, false/*auto_delete*/) < 0) {
        log_err("declareExchange %s Error!", p_rabbitmq->exchange_name_.c_str());
        return false;
    }

    uint32_t msg_cnt = 0;
    uint32_t cons_cnt = 0;
    if (pChannel->declareQueue(p_rabbitmq->queue_name_, msg_cnt, cons_cnt, false/*passive*/, true/*durable*/, false/*exclusive*/, false/*auto_delete*/) < 0) {
        log_err("declareQueue %s Failed!", p_rabbitmq->queue_name_.c_str());
        return false;
    }

    log_info("Broker report info: msg_cnt->%d, cons_cnt->%d", msg_cnt, cons_cnt);

    // 路邮键 paybill
    if (pChannel->bindQueue(p_rabbitmq->queue_name_, p_rabbitmq->exchange_name_, p_rabbitmq->route_key_)) {
        log_err("bindExchange Error!");
        return false;
    }

    log_notice("PbiRabbitMQHandler init ok (working in producer/publish mode)!");

    return true;
}




bool MqConn::mq_setup_channel_consume(AMQP::RabbitChannelPtr pChannel, void* pArg) {

    if (!pChannel || !pArg) {
        log_err("RabbitChannelPtr or pArg nullptr!");
        return false;
    }

    rabbitmq_character_t* p_rabbitmq = static_cast<rabbitmq_character_t *>(pArg);
    if (p_rabbitmq->exchange_name_.empty() || p_rabbitmq->queue_name_.empty() || p_rabbitmq->route_key_.empty()){
       log_err("Required connection info missing...");
       return false;
    }

    if (pChannel->declareExchange(p_rabbitmq->exchange_name_, "direct", false/*passive*/, true/*durable*/, false/*auto_delete*/) < 0) {
        log_err("declareExchange %s Error!", p_rabbitmq->exchange_name_.c_str());
        return false;
    }

    uint32_t msg_cnt = 0;
    uint32_t cons_cnt = 0;
    if (pChannel->declareQueue(p_rabbitmq->queue_name_, msg_cnt, cons_cnt, false/*passive*/, true/*durable*/, false/*exclusive*/, false/*auto_delete*/) < 0) {
        log_err("Declare Queue %s Failed!", p_rabbitmq->queue_name_.c_str());
        return false;
    }

    log_info("Broker report info: msg_cnt->%d, cons_cnt->%d", msg_cnt, cons_cnt);

    // 路邮键 paybill
    if (pChannel->bindQueue(p_rabbitmq->queue_name_, p_rabbitmq->exchange_name_, p_rabbitmq->route_key_)) {
        log_err("bindExchange Error!");
        return false;
    }

    if (pChannel->basicQos(1, true) < 0) {
        log_err("basicQos Failed!");
        return false;
    }

    if (pChannel->basicConsume(p_rabbitmq->queue_name_, "*", false/*no_local*/, false/*no_ack*/, false/*exclusive*/) < 0) {
        log_err("BasicConosume queue %s Failed!", p_rabbitmq->queue_name_.c_str());
        return false;
    }

    log_notice("PbiRabbitMQHandler init ok (working in comsume mode)!");
    return true;
}

bool MqConn::mq_setup_channel_get(AMQP::RabbitChannelPtr pChannel, void* pArg) {

    if (!pChannel || !pArg) {
        log_err("RabbitChannelPtr or pArg nullptr!");
        return false;
    }

    rabbitmq_character_t* p_rabbitmq = static_cast<rabbitmq_character_t *>(pArg);
    if (p_rabbitmq->exchange_name_.empty() || p_rabbitmq->queue_name_.empty() || p_rabbitmq->route_key_.empty()){
       log_err("Required connection info missing...");
       return false;
    }

    if (pChannel->declareExchange(p_rabbitmq->exchange_name_, "direct", false/*passive*/, true/*durable*/, false/*auto_delete*/) < 0) {
        log_err("declareExchange %s Error!", p_rabbitmq->exchange_name_.c_str());
        return false;
    }

    uint32_t msg_cnt = 0;
    uint32_t cons_cnt = 0;
    if (pChannel->declareQueue(p_rabbitmq->queue_name_, msg_cnt, cons_cnt, false/*passive*/, true/*durable*/, false/*exclusive*/, false/*auto_delete*/) < 0) {
        log_err("Declare Queue %s Failed!", p_rabbitmq->queue_name_.c_str());
        return false;
    }

    log_info("Broker report info: msg_cnt->%d, cons_cnt->%d", msg_cnt, cons_cnt);

    // 路邮键
    if (pChannel->bindQueue(p_rabbitmq->queue_name_, p_rabbitmq->exchange_name_, p_rabbitmq->route_key_)) {
        log_err("bindExchange Error!");
        return false;
    }

    if (pChannel->basicQos(1, true) < 0) {
        log_err("basicQos Failed!");
        return false;
    }

    log_notice("PbiRabbitMQHandler init ok (working in get mode)!");
    return true;
}


bool MqConn::check_and_set_work_mode(enum MqWorkState state) {

    int boolRet = false;

    if (state_ != state) {

        log_info("transform state from %d to %d", static_cast<int>(state_), static_cast<int>(state));
        state_ = state;

        // force re-setup once
        if (state_ == MqWorkState::kProducer) {
            boolRet = mq_.setupChannel(t_, boost::bind(&MqConn::mq_setup_channel_publish, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
        } else if (state_ == MqWorkState::kConsumer) {
            boolRet = mq_.setupChannel(t_, boost::bind(&MqConn::mq_setup_channel_consume, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
		} else if (state_ == MqWorkState::kGetter) {
            boolRet = mq_.setupChannel(t_, boost::bind(&MqConn::mq_setup_channel_get, this, _1, _2),
                                                                const_cast<rabbitmq_character_t *>(&helper_.properity_));
		} else {
            log_err("nnknown work mode here: %d", static_cast<int>(state_));
            return false;
        }

        if (!boolRet) {
            log_err("Setup channel failed!");
            return false;
        }
	}

    return true;
}

bool MqConn::check_and_repair() {

    int intRet  = 0;

    // check connection self
    int retry_count = helper_.connect_urls_.size() * 2;
    do {
        if (state_ == MqWorkState::kProducer) {
            intRet = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_publish, this, _1, _2),
                                                    const_cast<rabbitmq_character_t *>(&helper_.properity_));
        } else if (state_ == MqWorkState::kConsumer) {
            intRet = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_consume, this, _1, _2),
                                                    const_cast<rabbitmq_character_t *>(&helper_.properity_));
        } else if (state_ == MqWorkState::kGetter) {
            intRet = mq_.checkAndRepairChannel(t_, boost::bind(&MqConn::mq_setup_channel_get, this, _1, _2),
                                                    const_cast<rabbitmq_character_t *>(&helper_.properity_));
		} else {
            log_err("nnknown work mode here: %d", static_cast<int>(state_));
            return false;
        }

        if( intRet == 0)
            return true;

    } while (-- retry_count > 0);

    log_err("retry repair connection failed...");
    return false;
}

int MqConn::publish(const std::string& msg) {

    if (!check_and_set_work_mode(MqWorkState::kProducer)) {
        log_err("check and tranform conn mode failed!");
        return -1;
    }

    return mq_.basicPublish(t_, helper_.properity_.exchange_name_, helper_.properity_.route_key_, true/* mandatory */, false/* immediate */, msg);
}

int MqConn::consume(std::string& msgout, int timeout_sec) {

    if (!check_and_set_work_mode(MqWorkState::kConsumer)) {
        log_err("check and tranform conn mode failed!");
        return -1;
    }

    struct timeval time_out {};
    time_out.tv_usec = 0;
    time_out.tv_sec = timeout_sec;

    AMQP::RabbitMessage RabbitMsg;
    int retCode = mq_.basicConsumeMessage(RabbitMsg, &time_out, 0);
    if (retCode == AMQP::WAIT_MSG_TIMEOUT) {
        // timeout return
        log_info("mq_consume t0 time_out return!");
        return 1;
    }

    if (retCode < 0) {
        log_err("Consume failed with %d", retCode);
        return retCode;
    }

    if (!RabbitMsg.has_content()) {
        log_err("empty message...");
        return 1;
    }

    std::string msg((const char*)(RabbitMsg.content().bytes), RabbitMsg.content().len);

    // 注意，这里必须先消费后确认，否则所有的消息都会堆积在librabbitmq-c库当中
    // 这种情况更加危险
    mq_.basicAck(t_, RabbitMsg.envelope.delivery_tag);  // 无论如何都ACK，不会回退消息
    log_debug(" Rabbit ACK: %ld ", RabbitMsg.envelope.delivery_tag);

    RabbitMsg.safe_clear();

    msgout = msg;
    return 0;
}


int MqConn::get(std::string& msgout) {

    if (!check_and_set_work_mode(MqWorkState::kGetter)) {
        log_err("check and tranform conn mode failed!");
        return -1;
    }

    AMQP::RabbitMessage RabbitMsg;
    int intRet = mq_.basicGet(t_, RabbitMsg, helper_.properity_.queue_name_, false /*no_ack*/);
    if( intRet < 0) {
        log_err("get message failed %d ...", intRet);
        return -2;
    }

    std::string msg((const char*)(RabbitMsg.content().bytes), RabbitMsg.content().len);

    // 无论如何都ACK，不会回退消息给Broker
    mq_.basicAck(t_, RabbitMsg.envelope.delivery_tag);
    log_debug(" Rabbit ACK: %ld ", RabbitMsg.envelope.delivery_tag);

    RabbitMsg.safe_clear();

    msgout = msg;
    return 0;
}
