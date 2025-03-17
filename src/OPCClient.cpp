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
#include <QString>
#include <QTimer>
#include <mutex>

// 辅助函数：去除字符串两端的空白字符
std::string trim(const std::string &s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// 转换函数：将字符串转换为布尔值
bool string_to_bool(const std::string &s) {
    std::string trimmed = trim(s);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (trimmed == "true" || trimmed == "1") {
        return true;
    }
    if (trimmed == "false" || trimmed == "0") {
        return false;
    }
    throw std::invalid_argument("Invalid boolean string: " + s);
}

OPCClient::OPCClient(QObject *parent) : QThread(parent) {
    opcua::ClientConfig config;
    config.setTimeout(500);
    mpClient = std::make_unique<opcua::Client>(std::move(config));
    mpReconnectTimer = new QTimer(this);
    mpReconnectTimer->setTimerType(Qt::VeryCoarseTimer);
    mpReconnectTimer->setInterval(1000);
    connect(mpReconnectTimer, &QTimer::timeout, this,&OPCClient::connectServer);
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    mpReconnectTimer->stop();
    if (mpClient->isConnected()){
        mpClient->disconnect();
    }
}

void OPCClient::setCode(const std::string &code) {
    std::scoped_lock lock(mClientLocker);
    mCode = code;
}

std::string OPCClient::code() {
    std::scoped_lock lock(mClientLocker);
    return mCode;
}

std::string OPCClient::url() const {
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

std::set<std::string> OPCClient::nodes() {
    std::scoped_lock lock(mClientLocker);
    return mNodes;
}

void OPCClient::setTopic(const std::string &dist) {
    std::scoped_lock lock(mClientLocker);
    mTopic = dist;
}

std::string OPCClient::topic() {
    std::scoped_lock lock(mClientLocker);
    return mTopic;
}

void OPCClient::setInterval(int interval) {
    mInterval = interval;
}

int OPCClient::interval() {
    return mInterval;
}


void OPCClient::start(){
    mpReconnectTimer->start();
}

void OPCClient::stop(){
    mpReconnectTimer->stop();
    std::scoped_lock lock(mClientLocker);
    mpClient->disconnect();
}

void OPCClient::setDataValue(const Data &data) {
    try {
        if (!isConnected()) {
            LogErr("客户端没有连接，发送失败！");
            return;
        }
        std::scoped_lock lock(mClientLocker);
        std::string nodeCode = data.first;
        std::string value = data.second;
        uint32_t index = nodeCode.find_first_of('_');
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        auto oldUaVar = uaNode.readValue();
        uint32_t type = oldUaVar.type()->typeKind;
        if (!uaNode.exists()) {
            throw std::runtime_error(fmt::format("节点{}不存在！", nodeCode));
        }
        opcua::Variant uaVar;
        if (type == UA_DATATYPEKIND_BOOLEAN) {
            uaVar = opcua::Variant(string_to_bool(value));
        } else if (type == UA_DATATYPEKIND_INT32) {
            uaVar = opcua::Variant(QString::fromStdString(value).toInt());
        } else if (type == UA_DATATYPEKIND_BYTE) {
            uaVar = opcua::Variant(static_cast<uint8_t>(QString::fromStdString(value).toUInt()));
        } else if (type == UA_DATATYPEKIND_FLOAT) {
            uaVar = opcua::Variant(QString::fromStdString(value).toDouble());
        } else if (type == UA_DATATYPEKIND_STRING) {
            uaVar = opcua::Variant(value);
        } else {
            LogWarn("不支持的数据类型: {}!", type);
        }
        uaNode.writeValue(uaVar);
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
}

void OPCClient::connectServer() {
    std::scoped_lock lock(mClientLocker);
    if (!mpClient->isConnected()){
        try{
            mpClient->connect(mUrl);
            if (!isRunning()){
                QThread::start();
            }
        }
        catch (std::exception &e){
            LogErr("重连服务器失败：{}",e.what());
        }
    }
}

void OPCClient::run() {
        while (isConnected()) {
            try {
                DataList datas;
                for (auto node: mNodes) {
                    std::pair<std::string, std::string> data;
                    std::string type;
                    std::size_t index = node.find_first_of('_');
                    if (index == std::string::npos) {
                        continue;
                    }
                    std::string first = node.substr(0, index);
                    std::string second = node.substr(index + 1);
                    opcua::Variant uaValue;
                    std::string browseName;
                    uint32_t typeKind;
                    {
                        std::scoped_lock lock(mClientLocker);
                        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
                        uaValue = uaNode.readValue();
                        typeKind = uaValue.type()->typeKind;
                        browseName = std::string(uaNode.readBrowseName().name());
                    }
                    data.first = node;
                    if (typeKind == UA_DATATYPEKIND_BOOLEAN) {
                        type = "bool";
                        if (uaValue.to<bool>()) {
                            data.second = "1";
                        } else {
                            data.second = "0";
                        }
                    } else if (typeKind == UA_DATATYPEKIND_INT32) {
                        type = "int32";
                        data.second = QString::number(uaValue.to<int32_t>()).toStdString();
                    } else if (typeKind == UA_DATATYPEKIND_BYTE) {
                        type = "byte";
                        data.second = QString::number(uaValue.to<uint8_t>()).toStdString();
                    } else if (typeKind == UA_DATATYPEKIND_FLOAT) {
                        type = "float";
                        data.second = QString::number(uaValue.to<float>()).toStdString();
                    } else if (typeKind == UA_DATATYPEKIND_DATETIME) {
                        type = "datetime";
                        data.second = QString::number(uaValue.to<uint32_t>()).toStdString();
                    } else if (typeKind == UA_DATATYPEKIND_STRING) {
                        type = "string";
                        data.second = uaValue.to<std::string>();
                    } else {
                        LogErr("Read Unsupported Data Type: {}!", type);
                        continue;
                    }
                    LogInfo("Successful Read Data,ID:{} Name:{} Type:{} Value:{}", node, browseName, type, data.second);
                    datas.emplace_back(data);
                }
                if (!datas.empty()) {
                    emit newData(mTopic, mCode, datas);
                }
            } catch (std::exception &e) {
                LogErr("{}", e.what());
            }
            msleep(mInterval);
        }
}

bool OPCClient::isConnected()
{
    std::scoped_lock lock(mClientLocker);
    return mpClient->isConnected();
}

