//
// Created by cumtzt on 25-1-20.
//
#include "OPCClient.h"

#include <iostream>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include <QMetaType>
#include "QDebug"

OPCClient::OPCClient(QObject *parent) : QThread(parent) {
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::pair<std::string, std::string>>("std::pair<std::string,std::string>");
    qRegisterMetaType<std::vector<std::pair<std::string, std::string>>>("std::vector<std::pair<std::string,std::string>>");
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
}

void OPCClient::setCode(const std::string& code) {
    std::scoped_lock lock(mClientLocker);
    mCode = code;
}

std::string OPCClient::getCode() {
    std::scoped_lock lock(mClientLocker);
    return mCode;
}

std::string OPCClient::getURL() {
    return mUrl;
}

void OPCClient::setUrl(const std::string &url) {
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
}

void OPCClient::addNode(const std::string &node) {
    std::scoped_lock lock(mClientLocker);
    mNodes.insert(node);
}

std::set<std::string> OPCClient::getNodes() {
    return mNodes;
}

void OPCClient::setDist(const std::string &dist) {
    std::scoped_lock lock(mClientLocker);
    mDestination = dist;
}

std::string OPCClient::getDist() {
    std::scoped_lock lock(mClientLocker);
    return mDestination;
}

void OPCClient::start() {
    mRunning = true;
    QThread::start();
}

void OPCClient::stop() {
    mRunning = false;
    wait();
}

void OPCClient::connectServer() {
    std::scoped_lock lock(mClientLocker);
    try {
        if (nullptr != mpClient) {
            if (mpClient->isConnected()) {
                mpClient->disconnect();
            }
            delete mpClient;
            mpClient = nullptr;
        }
        if (mUrl.empty()) {
            return;
        }
        mpClient = new opcua::Client();
        mpClient->connect(mUrl);
    } catch (const std::exception &e) {
        LogErr("{}", e.what());
        delete mpClient;
        mpClient = nullptr;
    }
}

void OPCClient::run() {
    while (mRunning) {
        std::scoped_lock lock(mClientLocker);
        if (nullptr == mpClient) {
            connectServer();
        }
        if (nullptr != mpClient) {
            try {
                std::vector<std::pair<std::string, std::string>> datas;
                for (auto node: mNodes) {
                    std::pair<std::string, std::string> data;
                    std::string type;
                    uint32_t index = node.find_first_of('_');
                    if (index == std::string::npos) {
                        continue;
                    }
                    std::string first = node.substr(0, index);
                    std::string second = node.substr(index + 1);
                    opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
                    std::string name = std::string(uaNode.readBrowseName().name());
                    data.first = node;
                    if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN) {
                        type = "bool";
                        if (uaNode.readValue().to<bool>()) {
                            data.second = "1";
                        } else {
                            data.second = "0";
                        }
                    } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_INT32) {
                        type = "int32";
                        data.second = QString::number(uaNode.readValue().to<int32_t>()).toStdString();
                    }else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE) {
                        type = "byte";
                        data.second = QString::number(uaNode.readValue().to<uint8_t>()).toStdString();
                    } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT) {
                        type = "float";
                        data.second = QString::number(uaNode.readValue().to<float>()).toStdString();
                    } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_DATETIME) {
                        type = "datetime";
                        data.second = QString::number(uaNode.readValue().to<uint32_t>()).toStdString();
                    }else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_STRING) {
                        type = "string";
                        data.second = uaNode.readValue().to<std::string>();
                    }else {
                        LogErr("Read Unsupported Data Type: {}!", uaNode.readValue().type()->typeKind);
                        continue;
                    }
                    LogInfo("Successful Read Data,ID:{} Name:{} Type:{} Value:{}", node, name, type, data.second);
                    datas.emplace_back(data);
                }
                if (!datas.empty()){
                    emit newData(mDestination,mCode, datas);
                }
            } catch (std::exception &e) {
                LogErr("{}", e.what());
            }
        }
        sleep(1);
    }
}
