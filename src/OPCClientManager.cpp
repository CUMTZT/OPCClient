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

OPCClientManager::OPCClientManager() : MessageProducer(nullptr) {
    mpThreadPool = new QThreadPool(this);
    mpTimer = new QTimer(this);
    mpTimer->setInterval(1000);
    mpTimer->setTimerType(Qt::VeryCoarseTimer);
    connect(mpTimer, &QTimer::timeout, this, &OPCClientManager::onTimerTimeout);
    mpTimer->start();
}

OPCClientManager::~OPCClientManager() {
}

void OPCClientManager::loadConfig(const std::string &configFile) {
    try {
        auto config = YAML::LoadFile(configFile);
        if (config.IsNull() || !config["opc"]) {
            LogErr("配置文件解析错误！");
            return;
        }
        mConfig = config["opc"];

        if (mConfig["station_id"]) {
            mStationId = mConfig["station_id"].as<int>();
        } else {
            LogWarn("配置文件中不存在station_id！");
            return;
        }

        if (mConfig["station_name"]) {
            mStationName = mConfig["station_name"].as<std::string>();
        } else {
            LogWarn("配置文件中不存在station_name！");
            return;
        }

        if (mConfig["max_thread_count"]) {
            auto maxThreadCount = mConfig["max_thread_count"].as<int>();
            if (maxThreadCount < 1) {
                maxThreadCount = 1;
            }
            mpThreadPool->setMaxThreadCount(maxThreadCount);
        } else {
            LogWarn("配置文件中不存在最大线程数设置，使用默认值1！");
        }
        if (mConfig["interval"]) {
            auto interval = mConfig["interval"].as<int>();
            if (interval < 1) {
                interval = 1;
            }
            mpTimer->setInterval(interval);
        } else {
            LogWarn("配置文件中不存在采集间隔时间，使用默认值1000ms！");
        }
        if (mConfig["clients"]) {
            for (int i = 0; i < mConfig["clients"].size(); i++) {
                auto clientConfig = mConfig["clients"][i];
                if (!clientConfig["id"]) {
                    LogErr("配置文件中不存在OPC客户端ID！");
                    continue;
                }
                int id = clientConfig["id"].as<int>();
                std::string name;
                if (!clientConfig["name"]) {
                    LogWarn("配置文件中不存在OPC客户端名称!");
                }
                name = clientConfig["name"].as<std::string>();
                if (!clientConfig["server"]) {
                    LogErr("配置文件中不存在OPC服务端URL！");
                    continue;
                }
                std::string server = clientConfig["server"].as<std::string>();
                std::pair<int, int> nodePair;
                if (clientConfig["nodes"]) {
                    for (auto node: clientConfig["nodes"]) {
                        if (node.size() != 2) {
                            LogWarn("节点配置错误！");
                            continue;
                        }
                        nodePair.first = node[0].as<int>();
                        nodePair.second = node[1].as<int>();
                    }
                } else {
                    LogWarn("OPCClient配置中不存在nodes节点!");
                }
                std::scoped_lock lock(mClientMapMutex);
                auto client = new OPCClient(this);
                client->setUrl(server);
                client->addNode(nodePair);
                client->setName(name);
                client->setID(id);
                client->setAutoDelete(false);
                connect(client, &OPCClient::newMessage, this, &OPCClientManager::onClientNewMessage);
                mClientMap.emplace(QString::number(id).toStdString(), client);
            }
        } else {
            LogWarn("配置文件中不存在OPC客户端配置！");
        }
    } catch (const YAML::Exception &e) {
        LogErr("{}", e.msg);
    }
}


void OPCClientManager::onClientNewMessage(const std::vector<OPCData> &message) {
    auto client = dynamic_cast<OPCClient *>(sender());
    if (client == nullptr) {
        return;
    }
    std::string dist = client->getDist();
    if (dist.empty()) {
        return;
    }
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter writer(buf);
    writer.StartObject();
    writer.Key("stationID");
    writer.Int(mStationId);
    writer.Key("stationName");
    writer.String(mStationName.c_str());
    writer.Key("clientID");
    writer.Int(client->getID());
    writer.Key("clientName");
    writer.String(client->getName().c_str());
    auto collectTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
    writer.Key("collectTime");
    writer.String(collectTime.c_str());
    writer.Key("params");
    writer.StartArray();
    for (auto data: message) {
        writer.StartObject();
        writer.Key("namespace");
        writer.Int(data.namespaceIndex);
        writer.Key("id");
        writer.Int(data.id);
        writer.Key("name");
        writer.String(data.name.c_str());
        writer.Key("val");
        writer.String(data.value.c_str());
        writer.Key("valType");
        writer.Int(data.type);
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    std::string strData = buf.GetString();
    emit newMessage(dist, strData);
}

void OPCClientManager::onTimerTimeout() {
    for (auto client: mClientMap) {
        mpThreadPool->start(client.second);
    }
    mpThreadPool->waitForDone(-1);
}
