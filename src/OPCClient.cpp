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
std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// 转换函数：将字符串转换为布尔值
bool string_to_bool(const std::string& s)
{
    std::string trimmed = trim(s);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    if (trimmed == "true" || trimmed == "1")
    {
        return true;
    }
    if (trimmed == "false" || trimmed == "0")
    {
        return false;
    }
    throw std::invalid_argument("Invalid boolean string: " + s);
}

OPCClient::OPCClient(QObject* parent) : QThread(parent)
{
    opcua::ClientConfig config;
    config.setTimeout(500);
    mpClient = std::make_unique<opcua::Client>(std::move(config));
    mpReconnectTimer = new QTimer(this);
    mpReconnectTimer->setTimerType(Qt::VeryCoarseTimer);
    mpReconnectTimer->setInterval(1000);
    connect(mpReconnectTimer, &QTimer::timeout, this, &OPCClient::connectServer);
}

OPCClient::~OPCClient()
{
    std::scoped_lock lock(mClientLocker);
    mpReconnectTimer->stop();
    if (mpClient->isConnected())
    {
        mpClient->disconnect();
    }
}

void OPCClient::setCode(const std::string& code)
{
    std::scoped_lock lock(mClientLocker);
    mCode = code;
}

std::string OPCClient::code()
{
    std::scoped_lock lock(mClientLocker);
    return mCode;
}

std::string OPCClient::url() const
{
    return mUrl;
}

void OPCClient::setUrl(const std::string& url)
{
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
}

void OPCClient::addNode(const std::string& node)
{
    std::scoped_lock lock(mClientLocker);
    mNodes.insert(node);
}

std::set<std::string> OPCClient::nodes()
{
    std::scoped_lock lock(mClientLocker);
    return mNodes;
}

void OPCClient::setTopic(const std::string& dist)
{
    std::scoped_lock lock(mClientLocker);
    mTopic = dist;
}

std::string OPCClient::topic()
{
    std::scoped_lock lock(mClientLocker);
    return mTopic;
}

void OPCClient::setInterval(int interval)
{
    mInterval = interval;
}

int OPCClient::interval()
{
    return mInterval;
}


void OPCClient::start()
{
    mpReconnectTimer->start();
}

void OPCClient::stop()
{
    mpReconnectTimer->stop();
    std::scoped_lock lock(mClientLocker);
    mpClient->disconnect();
}

void OPCClient::setDataValue(const Data& data)
{
    try
    {
        if (!isConnected())
        {
            LogErr("客户端没有连接，发送失败！");
            return;
        }
        std::string nodeCode = data.first;
        std::string value = data.second;
        uint32_t index = nodeCode.find_first_of(':');
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        std::scoped_lock lock(mClientLocker);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        auto oldUaVar = uaNode.readValue();
        uint32_t typeKind = oldUaVar.type()->typeKind;
        if (!uaNode.exists()){
            throw std::runtime_error(fmt::format("节点{}不存在，写入失败！", nodeCode));
        }
        switch (typeKind)
        {
        case UA_DATATYPEKIND_BOOLEAN:
            uaNode.writeValueScalar<bool>(string_to_bool(value));
            break;
        case UA_DATATYPEKIND_SBYTE:
            uaNode.writeValueScalar<int8_t>(static_cast<int8_t>(QString::fromStdString(value).toInt()));
            break;
        case UA_DATATYPEKIND_BYTE:
            uaNode.writeValueScalar<uint8_t>(static_cast<uint8_t>(QString::fromStdString(value).toInt()));
            break;
        case UA_DATATYPEKIND_INT16:
            uaNode.writeValueScalar<int16_t>(QString::fromStdString(value).toShort());
            break;
        case UA_DATATYPEKIND_UINT16:
            uaNode.writeValueScalar<uint16_t>(QString::fromStdString(value).toUShort());
            break;
        case UA_DATATYPEKIND_INT32:
            uaNode.writeValueScalar<int32_t>(QString::fromStdString(value).toInt());
            break;
        case UA_DATATYPEKIND_UINT32:
            uaNode.writeValueScalar<uint32_t>(QString::fromStdString(value).toUInt());
            break;
        case UA_DATATYPEKIND_INT64:
            uaNode.writeValueScalar<int64_t>(QString::fromStdString(value).toLongLong());
            break;
        case UA_DATATYPEKIND_UINT64:
            uaNode.writeValueScalar<uint64_t>(QString::fromStdString(value).toULongLong());
            break;
        case UA_DATATYPEKIND_FLOAT:
            uaNode.writeValueScalar<float>(QString::fromStdString(value).toFloat());
            break;
        case UA_DATATYPEKIND_DOUBLE:
            uaNode.writeValueScalar<double>(QString::fromStdString(value).toDouble());
            break;
        case UA_DATATYPEKIND_STRING:
            uaNode.writeValueScalar<std::string>(value);
            break;
        default:
            throw std::runtime_error(fmt::format("节点：{}数据类型不支持写入,写入失败！",nodeCode));
        }
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}

void OPCClient::connectServer()
{
    std::scoped_lock lock(mClientLocker);
    if (!mpClient->isConnected())
    {
        try
        {
            mpClient->connect(mUrl);
            if (!isRunning())
            {
                QThread::start();
            }
        }
        catch (std::exception& e)
        {
            LogErr("重连服务器失败：{}", e.what());
        }
    }
}

void OPCClient::run()
{
    while (isConnected())
    {
        try
        {
            DataList datas;
            for (auto node : mNodes)
            {
                std::pair<std::string, std::string> data;
                std::string type;
                std::size_t index = node.find_first_of(':');
                if (index == std::string::npos){
                    LogErr("节点code ：{}解析失败！",node);
                    continue;
                }
                std::string first = node.substr(0, index);
                std::string second = node.substr(index + 1);
                opcua::Variant uaValue;
                std::string browseName;
                uint32_t typeKind;
                {
                    std::scoped_lock lock(mClientLocker);
                    try
                    {
                        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
                        uaValue = uaNode.readValue();
                        typeKind = uaValue.type()->typeKind;
                        browseName = std::string(uaNode.readBrowseName().name());
                    }
                    catch (std::exception& e){
                        LogErr("读取节点：{}出错：{}", node, e.what());
                        continue;
                    }
                }
                data.first = node;

                switch (typeKind)
                {
                case UA_DATATYPEKIND_BOOLEAN:
                    type = "bool";
                    data.second = uaValue.to<bool>() ? "1" : "0";
                    break;
                case UA_DATATYPEKIND_SBYTE:
                    type = "int8_t";
                    data.second = QString::number(uaValue.to<int8_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_BYTE:
                    type = "uint8_t";
                    data.second = QString::number(uaValue.to<uint8_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_INT16:
                    type = "int16_t";
                    data.second = QString::number(uaValue.to<int16_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_UINT16:
                    type = "uint16_t";
                    data.second = QString::number(uaValue.to<uint16_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_INT32:
                    type = "int32_t";
                    data.second = QString::number(uaValue.to<int>()).toStdString();
                    break;
                case UA_DATATYPEKIND_UINT32:
                    type = "uint32_t";
                    data.second = QString::number(uaValue.to<uint32_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_INT64:
                    type = "int64_t";
                    data.second = QString::number(uaValue.to<int64_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_UINT64:
                    type = "uint64_t";
                    data.second = QString::number(uaValue.to<uint64_t>()).toStdString();
                    break;
                case UA_DATATYPEKIND_FLOAT:
                    type = "float";
                    data.second = QString::number(uaValue.to<float>()).toStdString();
                    break;
                case UA_DATATYPEKIND_DOUBLE:
                    type = "double";
                    data.second = QString::number(uaValue.to<double>()).toStdString();
                    break;
                case UA_DATATYPEKIND_STRING:
                    type = "string";
                    data.second = uaValue.to<std::string>();
                    break;
                default:
                    LogErr("不支持的数据类型: {}!", typeKind);
                    break;
                }
                if (!type.empty())
                {
                    LogInfo("成功读取到数据,ID:[{}] Name:{} Type:{} Value:{}", node, browseName, type, data.second);
                    datas.emplace_back(data);
                }
            }
            if (!datas.empty())
            {
                emit newData(mTopic, mCode, datas);
            }
        }
        catch (std::exception& e)
        {
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
