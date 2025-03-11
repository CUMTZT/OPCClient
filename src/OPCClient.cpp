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

void OPCClient::setUrl(const std::string &url) {
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
}

std::string OPCClient::url() {
    return mUrl;
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
        if (nullptr == mpClient) {
            return;
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
        opcua::Variant uaVar;
        if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BOOLEAN) {
            uaVar = opcua::Variant(string_to_bool(value));
        } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_INT32) {
            uaVar = opcua::Variant(QString::fromStdString(value).toInt());
        } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_BYTE) {
            uaVar = opcua::Variant(static_cast<uint8_t>(QString::fromStdString(value).toUInt()));
        } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_FLOAT) {
            uaVar = opcua::Variant(QString::fromStdString(value).toDouble());
        } else if (uaNode.readValue().type()->typeKind == UA_DATATYPEKIND_STRING) {
            uaVar = opcua::Variant(value);
        } else {
            LogWarn("不支持的数据类型: {}!", uaNode.readValue().type()->typeKind);
        }
        uaNode.writeValue(uaVar);
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
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
            LogErr("OPC Server Url为空！");
        }
        mpClient = new opcua::Client();
        mpClient->onConnected([this]() {
            LogInfo("[{}]成功连接到OPC服务端[{}]", mCode, mUrl);
        });
        mpClient->onDisconnected([this]() {
            LogInfo("[{}]与OPC服务端[{}]断开连接", mCode, mUrl);
        });
        mpClient->onSessionClosed([this]() {
            LogInfo("[{}]与OPC服务端[{}] Session关闭", mCode, mUrl);
        });
        mpClient->onSessionActivated([&] {
            std::cout << "Session activated" << std::endl;

            // Schedule async operations once the client session is activated
            // 1. Read request
            opcua::services::readValueAsync(
                *mpClient,
                {1,22},
                [](opcua::Result<opcua::Variant> &result) {
                    auto value = result.value().to<std::string>();
                    //LogInfo("{}", value.data);
                }
            );
            //
            // // 2. Browse request of Server object
            // const opcua::BrowseDescription bd(
            //     opcua::ObjectId::Server,
            //     opcua::BrowseDirection::Forward,
            //     opcua::ReferenceTypeId::References
            // );
            // opcua::services::browseAsync(*mpClient, bd, 0, [](opcua::BrowseResult &result) {
            //     std::cout << "Browse result with " << result.references().size() << " references:\n";
            //     for (const auto &reference: result.references()) {
            //         std::cout << "- " << reference.browseName().name() << std::endl;
            //     }
            // });
            //
            // // 3. Subscription
            // opcua::services::createSubscriptionAsync(
            //     *mpClient,
            //     opcua::SubscriptionParameters{}, // default subscription parameters
            //     true, // publishingEnabled
            //     {}, // statusChangeCallback
            //     [](opcua::IntegerId subId) {
            //         std::cout << "Subscription deleted: " << subId << std::endl;
            //     },
            //     [&](opcua::CreateSubscriptionResponse &response) {
            //         std::cout
            //                 << "Subscription created:\n"
            //                 << "- status code: " << response.responseHeader().serviceResult() << "\n"
            //                 << "- subscription id: " << response.subscriptionId() << std::endl;
            //
            //         // Create MonitoredItem
            //         opcua::services::createMonitoredItemDataChangeAsync(
            //             *mpClient,
            //             response.subscriptionId(),
            //             opcua::ReadValueId(
            //                 {1,22},
            //                 opcua::AttributeId::Value
            //             ),
            //             opcua::MonitoringMode::Reporting,
            //             opcua::MonitoringParametersEx{}, // default monitoring parameters
            //             [](opcua::IntegerId subId, opcua::IntegerId monId, const opcua::DataValue &dv) {
            //                 std::cout
            //                         << "Data change notification:\n"
            //                         << "- subscription id: " << subId << "\n"
            //                         << "- monitored item id: " << monId << "\n"
            //                         << "- timestamp: " << dv.sourceTimestamp() << std::endl;
            //             },
            //             {}, // delete callback
            //             [](opcua::MonitoredItemCreateResult &result) {
            //                 std::cout
            //                         << "MonitoredItem created:\n"
            //                         << "- status code: " << result.statusCode() << "\n"
            //                         << "- monitored item id: " << result.monitoredItemId() << std::endl;
            //             }
            //         );
            //     }
            // );
        });
        mpClient->connectAsync(mUrl);
        mpThread = new std::thread([this]() {
            mpClient->run();
        });
    } catch (const std::exception &e) {
        delete mpClient;
        mpClient = nullptr;
        LogErr("连接OPC Server失败{}！", e.what());
    }
}