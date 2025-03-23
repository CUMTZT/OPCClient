//
// Created by cumtzt on 25-1-20.
//
#include "Machine.h"
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

IMPLEMENT_EXCEPTION(OPCServerNotConnectException, RuntimeException, "未连接到OPC服务")
IMPLEMENT_EXCEPTION(OPCNodeCodeFormatErrorException, RuntimeException, "OPC节点Code格式解析错误")
IMPLEMENT_EXCEPTION(OPCNodeNotExistException, RuntimeException, "OPC节点不存在")
IMPLEMENT_EXCEPTION(OPCNodeTypeNotSupportException, RuntimeException, "OPC节点格式不被支持")

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

Machine::Machine(QObject* parent) : QThread(parent)
{
    opcua::ClientConfig config;
    config.setTimeout(500);
    mpClient = std::make_unique<opcua::Client>(std::move(config));
    mpReconnectTimer = new QTimer(this);
    mpReconnectTimer->setTimerType(Qt::VeryCoarseTimer);
    mpReconnectTimer->setInterval(1000);
    connect(mpReconnectTimer, &QTimer::timeout, this, &Machine::connectServer);
}

Machine::~Machine()
{
    std::scoped_lock lock(mClientLocker);
    mpReconnectTimer->stop();
    if (mpClient->isConnected())
    {
        mpClient->disconnect();
    }
}

void Machine::setCode(const std::string& code)
{
    std::scoped_lock lock(mClientLocker);
    mMachineCode = code;
}

std::string Machine::code()
{
    std::scoped_lock lock(mClientLocker);
    return mMachineCode;
}

std::string Machine::url() const
{
    return mUrl;
}

void Machine::setUrl(const std::string& url)
{
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
}

void Machine::collectNode(const std::string& node)
{
    std::scoped_lock lock(mClientLocker);
    mNodeCodes.insert(node);
}

void Machine::removeCollectingNode(const std::string& node)
{
    std::scoped_lock lock(mClientLocker);
    auto iter = mNodeCodes.find(node);
    if (iter != mNodeCodes.end())
    {
        mNodeCodes.erase(iter);
    }
}

std::set<std::string> Machine::collectingNodes()
{
    std::scoped_lock lock(mClientLocker);
    return mNodeCodes;
}

std::set<std::string> Machine::allNodes()
{
    std::scoped_lock lock(mClientLocker);

}

void Machine::setTopic(const std::string& dist)
{
    std::scoped_lock lock(mClientLocker);
    mTopic = dist;
}

std::string Machine::topic()
{
    std::scoped_lock lock(mClientLocker);
    return mTopic;
}

void Machine::setInterval(int interval)
{
    mInterval = interval;
}

int Machine::interval()
{
    return mInterval;
}


void Machine::start()
{
    mpReconnectTimer->start();
}

void Machine::stop()
{
    mpReconnectTimer->stop();
    std::scoped_lock lock(mClientLocker);
    mpClient->disconnect();
}

void Machine::setNodeValue(const std::string& nodeCode, const std::string& value)
{
    try
    {
        if (!isConnected())
        {
            OPCServerNotConnectException e(fmt::format("没有连接到OPC服务[{}]，指令[{},{}]上行失败！",mMachineCode, nodeCode, value));
            e.rethrow();
        }
        uint32_t index = nodeCode.find_first_of(':');
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        std::scoped_lock lock(mClientLocker);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        auto oldUaVar = uaNode.readValue();
        uint32_t typeKind = oldUaVar.type()->typeKind;
        if (!uaNode.exists())
        {
            OPCNodeNotExistException e(fmt::format("OPC服务[{}]节点[{}]不存在",mMachineCode, nodeCode));
            e.rethrow();
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
            OPCNodeTypeNotSupportException e(fmt::format("OPC服务[{}]节点[{}]类型[{}]不被支持",mMachineCode, nodeCode, typeKind));
            e.rethrow();
        }
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}

void Machine::getNode(const std::string& nodeCode,std::string& name,std::string& type,std::string& value)
{
    try
    {
        if (!isConnected())
        {
            OPCServerNotConnectException e(fmt::format("没有连接到服务[{}]，获取节点[{}]数据失败！",mMachineCode, nodeCode));
            e.rethrow();
        }
        uint32_t index = nodeCode.find_first_of(':');
        if (index == std::string::npos){
            OPCNodeCodeFormatErrorException e("解析NodeCode[{}],失败",nodeCode);
            e.rethrow();
        }
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        std::scoped_lock lock(mClientLocker);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        if (!uaNode.exists())
        {
            OPCNodeNotExistException e(fmt::format("OPC服务[{}]节点[{}]不存在",mMachineCode, nodeCode));
            e.rethrow();
        }
        auto uaValue = uaNode.readValue();
        name = uaNode.readBrowseName().name();
        switch (uint32_t typeKind = uaValue.type()->typeKind)
        {
        case UA_DATATYPEKIND_BOOLEAN:
            value = uaValue.to<bool>() ? "1" : "0";
            break;
        case UA_DATATYPEKIND_SBYTE:
            type = "int8_t";
            value = QString::number(uaValue.to<int8_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_BYTE:
            type = "uint8_t";
            value = QString::number(uaValue.to<uint8_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_INT16:
            type = "int16_t";
            value = QString::number(uaValue.to<int16_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_UINT16:
            type = "uint16_t";
            value = QString::number(uaValue.to<uint16_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_INT32:
            type = "int32_t";
            value = QString::number(uaValue.to<int>()).toStdString();
            break;
        case UA_DATATYPEKIND_UINT32:
            type = "uint32_t";
            value = QString::number(uaValue.to<uint32_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_INT64:
            type = "int64_t";
            value = QString::number(uaValue.to<int64_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_UINT64:
            type = "uint64_t";
            value = QString::number(uaValue.to<uint64_t>()).toStdString();
            break;
        case UA_DATATYPEKIND_FLOAT:
            type = "float";
            value = QString::number(uaValue.to<float>()).toStdString();
            break;
        case UA_DATATYPEKIND_DOUBLE:
            type = "double";
            value = QString::number(uaValue.to<double>()).toStdString();
            break;
        case UA_DATATYPEKIND_STRING:
            type = "string";
            value = uaValue.to<std::string>();
            break;
        default:
            LogErr("不支持的数据类型: {}!", typeKind);
            break;
        }
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}

void Machine::connectServer()
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

void Machine::run()
{
    while (isConnected())
    {
        try
        {
            std::vector<std::pair<std::string,std::string>> datas;
            datas.reserve(mNodeCodes.size());
            for (auto node : mNodeCodes)
            {
                std::string type;
                std::string value;
                std::string browseName;
                try{
                    getNode(node, browseName, type,value);
                    datas.emplace_back(node, value);
                    LogInfo("成功读取到数据,ID:[{}] Name:{} Type:{} Value:{}", node, browseName, type, value);
                }
                catch (Exception& e){
                    LogErr("{}", e.message());
                }
            }
            if (!datas.empty())
            {
                emit newData(mTopic, mMachineCode, datas);
            }
        }
        catch (std::exception& e)
        {
            LogErr("{}", e.what());
        }
        msleep(mInterval);
    }
}

bool Machine::isConnected()
{
    std::scoped_lock lock(mClientLocker);
    return mpClient->isConnected();
}
