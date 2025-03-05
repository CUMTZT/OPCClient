//
// Created by cumtzt on 25-1-21.
//
#include "KafkaProducer.h"
#include <yaml-cpp/yaml.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "Logger.h"
#include <QDateTime>

KafkaProducer::KafkaProducer(QObject *parent) : QObject(parent) {}

void KafkaProducer::loadConfig(const std::string& configFile) {
    cppkafka::Configuration config;
    YAML::Node configNode = YAML::LoadFile(configFile);
    if (!configNode.IsNull()) {

        if (configNode["station_code"]) {
            mStationCode = configNode["station_code"].as<std::string>();
        } else {
            LogWarn("配置文件中不存在station_code！");
            return;
        }

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

void KafkaProducer::onNewDatas(const std::string& dist,const std::string& source, const std::vector<std::pair<std::string, std::string>>& datas) {
    if (nullptr == mpProducer) {
        LogWarn("{}","Kafka生产者为nullptr！");//TODO
        return;
    }
    if (dist.empty()){
        LogWarn("{}","发送Topic为空！");//TODO
        return;
    }
    if(source.empty())
    {
        LogWarn("{}","电站code为空！");//TODO
        return;
    }
    if (datas.empty())
    {
        LogWarn("{}","数据为空！");//TODO
        return;
    }
    cppkafka::MessageBuilder builder(dist);
    std::string message;
    try {
        rapidjson::StringBuffer buf;
        rapidjson::Writer writer(buf);
        writer.StartArray();
        writer.StartObject();
        writer.Key("code");
        writer.String((mStationCode+":"+source).c_str());
        auto collectTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
        writer.Key("collectTime");
        writer.String(collectTime.c_str());
        writer.Key("params");
        writer.StartArray();
        for (auto data: datas) {
            writer.StartObject();
            writer.Key("code");writer.String(data.first.c_str());
            writer.Key("val");writer.String(data.second.c_str());
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();
        writer.EndArray();
        message = buf.GetString();
    }
    catch (std::exception& e) {
        LogErr("json数据序列化失败！: {}",e.what());
        return;
    }
    if (message.empty()) {
        LogWarn("json数据为空！");
        return;
    }
    builder.payload({message.c_str(), message.size()});
    mpProducer->produce(builder);
}
