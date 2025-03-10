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
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<std::pair<std::string, std::string>>("std::pair<std::string,std::string>");
    qRegisterMetaType<std::vector<std::pair<std::string, std::string>>>(
        "std::vector<std::pair<std::string,std::string>>");
}

OPCClient::~OPCClient()
{
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
}

void OPCClient::setCode(const std::string& code)
{
    std::scoped_lock lock(mClientLocker);
    mCode = code;
}

std::string OPCClient::getCode()
{
    std::scoped_lock lock(mClientLocker);
    return mCode;
}

std::string OPCClient::getURL()
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
    mRunning = true;
    QThread::start();
}

void OPCClient::stop()
{
    mRunning = false;
    wait();
}

void OPCClient::setDataValue(const Data& data)
{
    std::scoped_lock lock(mClientLocker);
    try
    {
        std::string nodeCode = data.first;
        std::string value = data.second;
        uint32_t index = nodeCode.find_first_of('_');
        std::string first = nodeCode.substr(0, index);
        std::string second = nodeCode.substr(index + 1);
        opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
        if (!uaNode.exists())
        {
            throw std::runtime_error(fmt::format("节点{}不存在！", nodeCode));
        }
        opcua::Variant uaVar;
        if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN)
        {
            uaVar = opcua::Variant(string_to_bool(value));
        }
        else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_INT32)
        {
            uaVar = opcua::Variant(QString::fromStdString(value).toInt());
        }
        else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE)
        {
            uaVar = opcua::Variant(static_cast<uint8_t>(QString::fromStdString(value).toUInt()));
        }
        else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT)
        {
            uaVar = opcua::Variant(QString::fromStdString(value).toDouble());
        }
        else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_STRING)
        {
            uaVar = opcua::Variant(value);
        }
        else
        {
            throw std::runtime_error(fmt::format("不支持的数据类型: {}!", uaNode.readValue().type()->typeKind));
        }

        uaNode.writeValue(uaVar);
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}

void OPCClient::connectServer()
{
    std::scoped_lock lock(mClientLocker);
    try
    {
        if (nullptr != mpClient)
        {
            if (mpClient->isConnected())
            {
                mpClient->disconnect();
            }
            delete mpClient;
            mpClient = nullptr;
        }
        if (mUrl.empty())
        {
            return;
        }
        mpClient = new opcua::Client();
        mpClient->connect(mUrl);
    }
    catch (const std::exception& e)
    {
        LogErr("{}", e.what());
        delete mpClient;
        mpClient = nullptr;
    }
}

void OPCClient::run()
{
    while (mRunning)
    {
        std::scoped_lock lock(mClientLocker);
        if (nullptr == mpClient)
        {
            connectServer();
        }
        if (nullptr != mpClient)
        {
            try
            {
                DataList datas;
                for (auto node : mNodes)
                {
                    std::pair<std::string, std::string> data;
                    std::string type;
                    std::size_t index = node.find_first_of('_');
                    if (index == std::string::npos)
                    {
                        continue;
                    }
                    std::string first = node.substr(0, index);
                    std::string second = node.substr(index + 1);
                    opcua::Node uaNode(*mpClient, opcua::NodeId(stoi(first), stoi(second)));
                    std::string name = std::string(uaNode.readBrowseName().name());
                    data.first = node;
                    if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN)
                    {
                        type = "bool";
                        if (uaNode.readValue().to<bool>())
                        {
                            data.second = "1";
                        }
                        else
                        {
                            data.second = "0";
                        }
                    }
                    else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_INT32)
                    {
                        type = "int32";
                        data.second = QString::number(uaNode.readValue().to<int32_t>()).toStdString();
                    }
                    else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE)
                    {
                        type = "byte";
                        data.second = QString::number(uaNode.readValue().to<uint8_t>()).toStdString();
                    }
                    else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT)
                    {
                        type = "float";
                        data.second = QString::number(uaNode.readValue().to<float>()).toStdString();
                    }
                    else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_DATETIME)
                    {
                        type = "datetime";
                        data.second = QString::number(uaNode.readValue().to<uint32_t>()).toStdString();
                    }
                    else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_STRING)
                    {
                        type = "string";
                        data.second = uaNode.readValue().to<std::string>();
                    }
                    else
                    {
                        LogErr("Read Unsupported Data Type: {}!", uaNode.readValue().type()->typeKind);
                        continue;
                    }
                    LogInfo("Successful Read Data,ID:{} Name:{} Type:{} Value:{}", node, name, type, data.second);
                    datas.emplace_back(data);
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
        }
        msleep(mInterval);
    }
}
