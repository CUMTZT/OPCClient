//
// Created by cumtzt on 25-1-21.
//
#include "KafkaProducer.h"
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

KafkaProducer::KafkaProducer(QObject *parent) : MessageConsumer(parent) {}

void KafkaProducer::loadConfig(const std::string& configFile) {
    cppkafka::Configuration config;
    YAML::Node configNode = YAML::LoadFile("./config/config.yml");
    if (!configNode.IsNull()) {
        if (configNode["kafka_producer"]) {
            auto kafkaNode = configNode["kafka_producer"];
            if (kafkaNode["brokers"]){
                auto brokers = kafkaNode["brokers"].as<std::string>();
                config.set("metadata.broker.list", brokers);
                mpProducer = std::make_shared<cppkafka::Producer>(config);
            }
        }
    }
}

void KafkaProducer::onNewMessage(const std::string& dist, const std::string& message) {
    if (nullptr == mpProducer || dist.empty() || message.empty()) {
        return;
    }
    cppkafka::MessageBuilder builder(dist);
    builder.payload({message.c_str(), message.size()});
    mpProducer->produce(builder);
}
