//
// Created by cumtzt on 25-2-24.
//
#include "OPCClientManager.h"
#include <yaml-cpp/yaml.h>
#include "Logger.h"

OPCClientManager* OPCClientManager::mpInstance = nullptr;

std::mutex OPCClientManager::mMutex;

OPCClientManager* OPCClientManager::getInstance() {
    if (mpInstance == nullptr) {
        std::scoped_lock lock(mMutex);
        if (nullptr == mpInstance) {
            mpInstance = new OPCClientManager();
        }
    }
    return mpInstance;
}

OPCClientManager::OPCClientManager() : QObject(nullptr) {
    try {
        mConfig = YAML::LoadFile("./config/config.yml");
        if (mConfig["OPCClients"]) {
            for (int i = 0; i < mConfig["OPCClients"].size(); i++) {
                auto config = mConfig["OPCClients"][i];
                QString url;
                int interval = 1000;
                QStringList nodeIds;
                if (config) {
                    LogErr("{}", "OPCClient config is NULL!");
                    return;
                }
                if (!config["server"]) {
                    LogErr("{}", "OPCClient Config is do not have server host!");
                    return;
                }
                url = QString::fromStdString(config["server"].as<std::string>());
                if (config["interval"]) {
                    interval = config["interval"].as<int>();
                }
                if (config["nodes"]) {
                    for (int i = 0; i < config["nodes"].size(); i++) {
                        nodeIds.append(QString::fromStdString(config["nodes"][i].as<std::string>()));
                    }
                }
                std::scoped_lock lock(mClientMapMutex);
                auto client = new OPCClient(this);
                client->setUrl(url);
                client->setInterval(interval);
                client->setNodeIds(nodeIds);
                mClientMap.insert(client->getURL(),client);
            }
        }
        LogWarn("{}","Config File Not Content OPC configure!");
    }
    catch (const YAML::Exception &e) {
        LogErr("{}",e.msg);
    }
}

OPCClientManager::~OPCClientManager() {}