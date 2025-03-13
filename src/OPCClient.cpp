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

#include "OPCClientManager.h"

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

OPCClient::OPCClient(QObject *parent) : QObject(parent) {
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::pair<std::string, std::string> >("std::pair<std::string,std::string>");
    qRegisterMetaType<std::vector<std::pair<std::string, std::string> > >(
        "std::vector<std::pair<std::string,std::string>>");
    mpReconnectTimer = new QTimer(this);
    mpReconnectTimer->setTimerType(Qt::VeryCoarseTimer);
    mpReconnectTimer->setInterval(1000);
    connect(mpReconnectTimer,&QTimer::timeout,this,&OPCClient::connectServer);
    mpReconnectTimer->start();
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
}

void OPCClient::setCode(const std::string &code) {
    std::scoped_lock lock(mClientLocker);
    mCode = code;
}

std::string OPCClient::code() {
    std::scoped_lock lock(mClientLocker);
    return mCode;
}

void OPCClient::setUrl(const std::string &url)
{
    std::scoped_lock lock(mClientLocker);
    mUrl.first = true;
    mUrl.second = url;
}

std::string OPCClient::url() {
    return mUrl.second;
}

void OPCClient::addNode(const std::string &node) {
    std::scoped_lock lock(mClientLocker);
    mNodes.insert({node, ""});
}

std::map<std::string, std::string> OPCClient::nodes() {
    std::scoped_lock lock(mNodesMutex);
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

void OPCClient::setDataValue(const Data &data) {
    std::scoped_lock lock(mClientLocker);
    try {
        if (nullptr == mpClient || !mpClient->isConnected()) {
            LogErr("未连接到OPCServer,发送失败！");
        }
        std::string nodeCode = data.first;
        std::string value = data.second;
        uint32_t index = nodeCode.find_first_of('_');
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        if (!uaNode.exists()) {
            throw std::runtime_error(fmt::format("节点{}不存在！", nodeCode));
        }
        auto retVar = uaNode.readValueAsync();
        if (retVar.wait_for(std::chrono::milliseconds(1000)) != std::future_status::ready){
            return;
        }
        uint32_t type = retVar.get()->type()->typeKind;
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
        uaNode.writeValueAsync(uaVar);
    } catch (std::exception &e) {
        std::rethrow_exception(std::current_exception());
    }
}

void OPCClient::connectServer() {
    std::scoped_lock lock(mClientLocker);
    try {
        if (!mUrl.first){
            return;
        }
        if (mUrl.second.empty()) {
            LogErr("OPC Url为空,客户端主动断开连接");
            if (nullptr != mpClient){
                if (mpClient->isConnected()){
                    mpClient->disconnect();
                }
                delete mpClient;
                mpClient = nullptr;
            }
            mUrl.first = false;
            return;
        }
        if (nullptr == mpClient){
            mpClient = new opcua::Client();
            mpClient->onConnected([this]() {
                LogInfo("[{}]成功连接到OPC服务端[{}]", mCode, mUrl.second);
            });
            mpClient->onDisconnected([this]() {
                LogInfo("[{}]与OPC服务端[{}]断开连接", mCode, mUrl.second);
            });
            mpClient->onSessionClosed([this]() {
                LogInfo("[{}]与OPC服务端[{}] Session关闭", mCode, mUrl.second);
            });
            mpClient->onSessionActivated([&] {
                std::cout << "Session activated" << std::endl;
                opcua::services::readValueAsync(
                    *mpClient,
                    {1,22},
                    [](opcua::Result<opcua::Variant> &result) {
                        auto value = result.value().to<std::string>();
                        LogInfo("{}", value);
                    }
                );
            });
        }
        if (mpClient->isConnected())
        {
            mpClient->disconnect();
            mpThread->join();
            delete mpThread;
            mpThread = nullptr;
        }
        mpClient->connectAsync(mUrl.second);
        mpThread = new std::thread([this]() {
            try{
                mUrl.first = false;
                mpClient->run();
            }
            catch (...){
                mUrl.first = true;
                LogErr("连接到OPCServer:{}失败!",mUrl.second);
            }
        });
    } catch (...) {
        delete mpClient;
        mpClient = nullptr;
        mUrl.first = true;
        LogErr("连接到OPCServer:{}失败!",mUrl.second);
    }
}