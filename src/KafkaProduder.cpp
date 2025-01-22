//
// Created by cumtzt on 25-1-21.
//
#include "KafkaProduder.h"
#include <yaml-cpp/yaml.h>
KafkaProducer* KafkaProducer::mpInstance = nullptr;

std::mutex KafkaProducer::mMutex;

KafkaProducer* KafkaProducer::getInstance() {
    if (mpInstance == nullptr) {
        std::lock_guard lock(mMutex);
        if (mpInstance == nullptr) {
            mpInstance = new KafkaProducer();
        }
    }
    return mpInstance;
}

KafkaProducer::KafkaProducer(QObject *parent) : QObject(parent) {
    cppkafka::Configuration config;
    YAML::Node configNode = YAML::LoadFile("./config/config.yml");
    if (!configNode.IsNull()) {
        if (configNode["brokers"]){
            auto brokers = configNode["brokers"].as<std::string>();
            config.set("metadata.broker.list", brokers);
            mpProducer = std::make_shared<cppkafka::Producer>(config);
        };
        if (configNode["topic"]) {
            mTopic = configNode["topic"].as<std::string>();
        }
    }
}

void KafkaProducer::sendMessage(const QString& message) {
    if (nullptr == mpProducer || mTopic.empty()) {
        return;
    }
    cppkafka::MessageBuilder builder(mTopic);
    builder.payload({message.toStdString().c_str(), message.toStdString().size()});
    mpProducer->produce(builder);
}
