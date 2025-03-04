//
// Created by cumtzt on 25-1-20.
//
#include "OPCClient.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include <QMetaType>

OPCClient::OPCClient(QObject *parent) : QObject(parent), mID(0) {
    qRegisterMetaType<OPCData>("OPCData");
    qRegisterMetaType<std::vector<OPCData> >("std::vector<OPCData>");
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
}

void OPCClient::setID(int id) {
    std::scoped_lock lock(mClientLocker);
    mID = id;
}

int OPCClient::getID() {
    std::scoped_lock lock(mClientLocker);
    return mID;
}

void OPCClient::setName(const std::string& name) {
    std::scoped_lock lock(mClientLocker);
    mName = name;
}

std::string OPCClient::getName() {
    std::scoped_lock lock(mClientLocker);
    return mName;
}

std::string OPCClient::getURL() {
    return mUrl;
}

void OPCClient::setUrl(const std::string &url) {
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
}

void OPCClient::addNode(const std::pair<int,int> &node) {
    std::scoped_lock lock(mClientLocker);
    mNodes.insert(node);
}

std::set<std::pair<int,int>> OPCClient::getNodes() {
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
     std::scoped_lock lock(mClientLocker);
    if (nullptr == mpClient) {
        connectServer();
    }
    std::vector<OPCData> message;
    if (nullptr != mpClient) {
        try {
            for (auto node: mNodes) {
                OPCData data;
                opcua::Node uaNode(*mpClient, opcua::NodeId(node.first, node.second));
                data.name = uaNode.readBrowseName().name();
                if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN) {
                    data.type = UA_DATATYPEKIND_BOOLEAN;
                    if (uaNode.readValue().to<bool>()) {
                        data.value = "1";
                    } else {
                        data.value = "0";
                    }
                } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_INT32) {
                    data.type = UA_DATATYPEKIND_INT32;
                    data.value = QString::number(uaNode.readValue().to<int32_t>()).toStdString();
                }else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE) {
                    data.type = UA_DATATYPEKIND_BYTE;
                    data.value = QString::number(uaNode.readValue().to<uint8_t>()).toStdString();
                } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT) {
                    data.type = UA_DATATYPEKIND_FLOAT;
                    data.value = QString::number(uaNode.readValue().to<float>()).toStdString();
                } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_DATETIME) {
                    data.type = UA_DATATYPEKIND_DATETIME;
                    data.value = QString::number(uaNode.readValue().to<uint32_t>()).toStdString();
                } else {
                    LogErr("Read Unsupported Data Type: {}!", uaNode.readValue().type()->typeKind);
                    continue;
                }
                LogInfo("Successful Read Data,ID:[{},{}] Name:{} Type:{} Value:{}", node.first,node.second, data.name, data.type, data.value);
                message.emplace_back(data);
            }
            emit newMessage(message);
        } catch (std::exception &e) {
            LogErr("{}", e.what());
        }
    }
}
