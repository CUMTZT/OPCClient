//
// Created by cumtzt on 25-1-21.
//

#ifndef KAFKAPRODUCER_H
#define KAFKAPRODUCER_H

#include <QObject>
#include <cppkafka/producer.h>
#include "MessageConsumer.h"
class KafkaProducer : public MessageConsumer {

    Q_OBJECT

public:

    static KafkaProducer* getInstance();

    KafkaProducer(KafkaProducer const&) = delete;

    KafkaProducer(KafkaProducer&&) = delete;

    KafkaProducer& operator=(KafkaProducer const&) = delete;

    void loadConfig(const std::string& configFile);

public slots:

    void onNewMessage(const std::string& dist, const std::string& message) override;

private:

    explicit KafkaProducer(QObject *parent = nullptr);

    static KafkaProducer* mpInstance;

    static std::mutex mMutex;

    std::shared_ptr<cppkafka::Producer> mpProducer = nullptr;

};

#define KafkaProducerIns KafkaProducer::getInstance()

#endif //KAFKAPRODUCER_H
