//
// Created by cumtzt on 25-1-20.
//
#include "OPCClient.h"
#include <rapidjson/document.h>
#include <yaml-cpp/yaml.h>
#include "Logger.h"

OPCClient::OPCClient(QObject *parent) : QObject(parent) {
    mpThread = new QThread();
    this->moveToThread(mpThread);
    mpThread->start();
    mpTimer = new QTimer();
    mpTimer->setInterval(1000);
    connect(mpTimer, &QTimer::timeout, this, &OPCClient::onTimerTimeout);
    mpTimer->start();
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
    mpThread->deleteLater();
    mpThread = nullptr;
}

QString OPCClient::getURL() {
    return mUrl;
}

void OPCClient::setUrl(const QString &url) {
    std::scoped_lock lock(mClientLocker);
    if (mUrl == url) {
        return;
    }
    if (nullptr != mpClient && mpClient->isConnected()) {
        mpClient->disconnect();
        delete mpClient;
        mpClient = nullptr;
    }
    mUrl = url;
    if (mUrl.isEmpty()) {
        return;
    }
    mpClient = new opcua::Client();
    try {
        mpClient->connect(mUrl.toStdString());
    } catch (const std::exception &e) {
        delete mpClient;
        mpClient = nullptr;
    }
    if (!mpClient->isConnected()) {
        delete mpClient;
        mpClient = nullptr;
    }
}

void OPCClient::setInterval(int interval) {
    mpTimer->setInterval(interval);
}

int OPCClient::getInterval() {
    return mpTimer->interval();
}

void OPCClient::setNodeIds(const QStringList &nodeIds) {
    mNodeIds = nodeIds;
}

QStringList OPCClient::getNodeIds() {
    return mNodeIds;
}

void OPCClient::onTimerTimeout() {
    std::scoped_lock lock(mClientLocker);
    if (nullptr == mpClient) {
        setUrl(mUrl);
    }
    if (nullptr != mpClient) {
        try {
            for (auto nodeId: mNodeIds) {
                opcua::Node node(*mpClient, opcua::NodeId(1, nodeId.toUInt()));
                std::string type;
                std::string name(node.readBrowseName().name());
                std::string value;
                if (node.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN) {
                    type = "Bit";
                    if (node.readValue().to<bool>()) {
                        value = "1";
                    } else {
                        value = "0";
                    }
                } else if (node.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE) {
                    type = "Byte";
                    value = QString::number(node.readValue().to<uint8_t>()).toStdString();
                } else if (node.readValue().type()->typeKind == UA_DATATYPEKIND_INT32) {
                    type = "Int32";
                    value = QString::number(node.readValue().to<int32_t>()).toStdString();
                } else if (node.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT) {
                    type = "Float";
                    value = QString::number(node.readValue().to<float>()).toStdString();
                } else if (node.readValue().type()->typeKind == UA_DATATYPEKIND_DATETIME) {
                    type = "DataTime";
                    value = QString::number(node.readValue().to<uint32_t>()).toStdString();
                } else {
                    LogErr("Read Unsupported Data Type: {}!", node.readValue().type()->typeKind);
                }
                LogInfo("Successful Read Data,ID:{} Name:{} Type:{} Value:{}", nodeId.toStdString(), name, type, value);
            }
        } catch (std::exception &e) {
            LogErr("{}", e.what());
        }
    }
}
