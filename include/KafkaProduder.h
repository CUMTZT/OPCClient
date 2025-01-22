//
// Created by cumtzt on 25-1-21.
//

#ifndef KAFKAPRODUDER_H
#define KAFKAPRODUDER_H

#include <QObject>
#include <cppkafka/producer.h>
class KafkaProducer : public QObject {

    Q_OBJECT

public:

    static KafkaProducer* getInstance();

    KafkaProducer(KafkaProducer const&) = delete;

    KafkaProducer(KafkaProducer&&) = delete;

    KafkaProducer& operator=(KafkaProducer const&) = delete;

public slots:

    void sendMessage(const QString& message);

private:

    explicit KafkaProducer(QObject *parent = nullptr);

    static KafkaProducer* mpInstance;

    static std::mutex mMutex;

    std::shared_ptr<cppkafka::Producer> mpProducer = nullptr;

    std::string mTopic;

};

#endif //KAFKAPRODUDER_H
