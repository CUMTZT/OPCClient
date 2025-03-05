//
// Created by cumtzt on 25-2-24.
//
#include "OPCClientManager.h"
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <QDateTime>
OPCClientManager *OPCClientManager::mpInstance = nullptr;

std::mutex OPCClientManager::mMutex;

OPCClientManager *OPCClientManager::getInstance() {
    if (mpInstance == nullptr) {
        std::scoped_lock lock(mMutex);
        if (nullptr == mpInstance) {
            mpInstance = new OPCClientManager();
        }
    }
    return mpInstance;
}

OPCClientManager::OPCClientManager() : QObject(nullptr) {
    mpKafkaProducer = new KafkaProducer();
    mpKafkaProducerThread = new QThread(this);
    mpKafkaProducer->moveToThread(mpKafkaProducerThread);
    mpKafkaProducerThread->start();
    //mpTimer = new QTimer(this);
    //mpTimer->setInterval(1000);
    //mpTimer->setTimerType(Qt::VeryCoarseTimer);
    //connect(mpTimer, &QTimer::timeout, this, &OPCClientManager::onTimerTimeout);
    //mpTimer->start();
}

OPCClientManager::~OPCClientManager() {
    mpTimer->stop();
}

void OPCClientManager::loadConfig(const std::string &configFile) {
    try {
        auto config = YAML::LoadFile(configFile);
        if (config.IsNull() || !config["opc"]) {
            LogErr("配置文件解析错误！");
            return;
        }

        mConfig = config["opc"];
        if (mConfig["interval"]) {
            auto interval = mConfig["interval"].as<int>();
            if (interval < 1) {
                interval = 1;
            }
            //mpTimer->setInterval(interval);
        } else {
            LogWarn("配置文件中不存在采集间隔时间，使用默认值1000ms！");
        }
        if (mConfig["clients"]) {
            for (int i = 0; i < mConfig["clients"].size(); i++) {
                auto clientConfig = mConfig["clients"][i];
                if (!clientConfig["code"]) {
                    LogErr("配置文件中不存在OPC客户端ID！");
                    continue;
                }
                auto code = clientConfig["code"].as<std::string>();
                if (!clientConfig["server"]) {
                    LogErr("配置文件中不存在OPC服务端URL！");
                    continue;
                }
                auto server = clientConfig["server"].as<std::string>();
                if (!clientConfig["dist"]) {
                    LogErr("配置文件中不存在OPC服务端URL！");
                    continue;
                }
                auto dist = clientConfig["dist"].as<std::string>();

                auto client = new OPCClient(this);
                client->setUrl(server);
                client->setCode(code);
                client->setDist(dist);
                connect(client, &OPCClient::newData, mpKafkaProducer, &KafkaProducer::onNewDatas);

                std::string node_code;
                if (clientConfig["nodes_config"]) {
                    auto node_config_path = clientConfig["nodes_config"].as<std::string>();
                    try {
                        auto nodeConfig = YAML::LoadFile(node_config_path);
                        if (nodeConfig.IsNull()) {
                            continue;
                        }
                        for (int j = 0; j < nodeConfig.size(); j++) {
                            if (!nodeConfig[j]["code"]) {
                                LogErr("配置文件中不存在code！");
                                continue;
                            }
                            node_code = nodeConfig[j]["code"].as<std::string>();
                            client->addNode(node_code);
                            if (!nodeConfig[j]["name"]) {
                                LogErr("配置文件中不存在name！");
                                continue;
                            }
                            auto node_name = nodeConfig[j]["name"].as<std::string>();
                        }
                    }
                    catch (YAML::Exception& e) {
                        LogErr("{}", e.msg);
                    }
                } else {
                    LogWarn("OPCClient配置中不存在nodes节点!");
                }
                std::scoped_lock lock(mClientsMutex);
                client->start();
                mClients.emplace_back(client);
            }
        } else {
            LogWarn("配置文件中不存在OPC客户端配置！");
        }
        mpKafkaProducer->loadConfig(configFile);
    } catch (const YAML::Exception &e) {
        LogErr("{}", e.msg);
    }
}

// void OPCClientManager::onTimerTimeout() {
//     std::scoped_lock lock(mClientsMutex);
//     for (auto client: mClients) {
//         client->start();
//     }
// }
