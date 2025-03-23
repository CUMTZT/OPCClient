//
// Created by cumtzt on 25-1-21.
//

#ifndef KAFKAPRODUCER_H
#define KAFKAPRODUCER_H

#include <QObject>
#include <cppkafka/producer.h>
#include "GlobalDefine.h"
class KafkaProducer : public QObject {
    Q_OBJECT

public:
    explicit KafkaProducer(QObject *parent = nullptr);

    void loadConfig(const std::string& configFile);

public slots:

    void onNewDatas(const std::string& topic, const std::string& code, const std::vector<std::pair<std::string,std::string>>& datas);

private:

    std::shared_ptr<cppkafka::Producer> mpProducer = nullptr;

    std::string mStationCode;

};

#endif //KAFKAPRODUCER_H
