//
// Created by cumtzt on 25-2-24.
//
#include "OPCClient.h"
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include <QDateTime>

IMPLEMENT_EXCEPTION(OPCClientNotExistException, ExistsException, "OPC客户端不存在")
IMPLEMENT_EXCEPTION(HttpRuntimeError, RuntimeException, "Http响应时出错")
IMPLEMENT_EXCEPTION(HttpUnsupportedSearchType, HttpRuntimeError, "Http不支持的查询操作")


OPCClient* OPCClient::mpInstance = nullptr;

std::recursive_mutex OPCClient::mMutex;

OPCClient* OPCClient::getInstance()
{
    if (nullptr == mpInstance)
    {
        std::scoped_lock lock(mMutex);
        if (nullptr == mpInstance)
        {
            mpInstance = new OPCClient();
        }
    }
    return mpInstance;
}

OPCClient::OPCClient() : QObject(nullptr)
{
    mpKafkaProducer = new KafkaProducer();
    mpKafkaProducerThread = new QThread(this);
    mpKafkaProducer->moveToThread(mpKafkaProducerThread);
    mpKafkaProducerThread->start();
    mpHttpServer = new httplib::Server();
    initHttpServer();
}

OPCClient::~OPCClient()
{
    for (auto& client : mClients)
    {
        client.second->stop();
    }
}

void OPCClient::loadConfig(const std::string& configFile)
{
    std::scoped_lock lock(mClientsMutex);
    try
    {
        auto config = YAML::LoadFile(configFile);
        if (config.IsNull() || !config["opc"])
        {
            LogErr("配置文件解析错误！");
            return;
        }
        mConfig = config["opc"];
        if (mConfig["ascending_server_port"])
        {
            auto port = mConfig["ascending_server_port"].as<int>();
            stopHttpServer();
            mpHttpServerThread = new std::thread([=, this]()
            {
                LogInfo("HttpServer正在监听 : {}", port);
                mpHttpServer->listen("localhost", port);
            });
        }
        if (mConfig["clients"])
        {
            for (int i = 0; i < mConfig["clients"].size(); i++)
            {
                auto clientConfig = mConfig["clients"][i];
                if (!clientConfig["code"])
                {
                    LogErr("配置文件中不存在OPC客户端ID！");
                    continue;
                }
                auto code = clientConfig["code"].as<std::string>();
                if (!clientConfig["server"])
                {
                    LogErr("配置文件中不存在OPC服务端URL！");
                    continue;
                }
                auto server = clientConfig["server"].as<std::string>();
                if (!clientConfig["topic"])
                {
                    LogErr("配置文件中不存在要发送的Topic！");
                    continue;
                }
                auto topic = clientConfig["topic"].as<std::string>();

                int interval = 1000;
                if (clientConfig["interval"])
                {
                    interval = clientConfig["interval"].as<int>();
                    if (interval < 1)
                    {
                        interval = 1000;
                    }
                }
                else
                {
                    LogWarn("配置文件中不存在采集间隔时间，使用默认值1000ms！");
                }

                auto client = std::make_shared<Machine>();
                client->setUrl(server);
                client->setCode(code);
                client->setTopic(topic);
                client->setInterval(interval);
                connect(client.get(), &Machine::newData, mpKafkaProducer, &KafkaProducer::onNewDatas);
                client->start();

                if (clientConfig["nodes_config"])
                {
                    auto node_config_path = clientConfig["nodes_config"].as<std::string>();
                    try
                    {
                        auto nodeConfig = YAML::LoadFile(node_config_path);
                        if (nodeConfig.IsNull())
                        {
                            continue;
                        }
                        std::string node_code;
                        for (auto&& j : nodeConfig)
                        {
                            node_code = j.as<std::string>();
                            client->collectNode(node_code);
                        }
                    }
                    catch (YAML::Exception& e)
                    {
                        LogErr("{}", e.msg);
                    }
                }
                else
                {
                    LogWarn("OPCClient配置中不存在nodes节点!");
                }
                client->start();
                mClients.emplace(code, client);
            }
        }
        else
        {
            LogWarn("配置文件中不存在OPC客户端配置！");
        }
        mpKafkaProducer->loadConfig(configFile);
    }
    catch (const YAML::Exception& e)
    {
        LogErr("{}", e.msg);
    }
}

void OPCClient::stopHttpServer()
{
    if (mpHttpServer->is_running())
    {
        mpHttpServer->stop();
        mpHttpServerThread->join();
        delete mpHttpServerThread;
        mpHttpServerThread = nullptr;
    }
}

std::string OPCClient::generateResponseContent(int code, const std::string& message, const std::string& data,bool isRaw)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer writer(sb);
    writer.StartObject();
    writer.Key("code");
    writer.Int(code);
    writer.Key("message");
    writer.String(message.c_str());
    writer.Key("data");
    if (isRaw){
        writer.RawValue(data.c_str(), data.length(), rapidjson::kArrayType);
    }
    else{
        writer.String(data.c_str(), rapidjson::kObjectType);
    }
    writer.EndObject();
    return sb.GetString();
}

void OPCClient::initHttpServer()
{
    mpHttpServer->Get("/update", [this](const httplib::Request& req, httplib::Response& res)
    {
        try
        {
            std::string machine = req.get_param_value("machine");
            std::string code = req.get_param_value("code");
            std::string value = req.get_param_value("value");
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            client->setNodeValue(code, value);
            auto result = generateResponseContent(
                200, fmt::format("{{发送指令[machine:{}, code:{}, value:{}]成功}}", machine, code, value));
            res.set_content(result, "application/json");
        }
        catch (...)
        {
            std::rethrow_exception(std::current_exception());
        }
    });

    mpHttpServer->Get("/add", [this](const httplib::Request& req, httplib::Response& res)
    {
        try
        {
            std::string machine = req.get_param_value("machine");
            std::string code = req.get_param_value("code");
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            client->collectNode(code);
            auto result =
                generateResponseContent(200, fmt::format("{{添加OPC节点[machine:{}, code:{}]成功}}", machine, code));
            res.set_content(result, "application/json");
        }
        catch (...)
        {
            std::rethrow_exception(std::current_exception());
        }
    });
    mpHttpServer->Get("/remove", [this](const httplib::Request& req, httplib::Response& res)
    {
        try
        {
            std::string machine = req.get_param_value("machine");
            std::string code = req.get_param_value("code");
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            client->removeCollectingNode(code);
            auto result =
                generateResponseContent(200, fmt::format("{{移除OPC节点[machine:{}, code:{}]成功}}", machine, code));
            res.set_content(result, "application/json");
        }
        catch (...)
        {
            std::rethrow_exception(std::current_exception());
        }
    });

    mpHttpServer->Get("/search", [this](const httplib::Request& req, httplib::Response& res)
    {
        std::string machine = req.get_param_value("machine");
        std::string type = req.get_param_value("type");
        if ("nodes" == type)
        {
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            rapidjson::StringBuffer sb;
            rapidjson::Writer writer(sb);
            writer.StartArray();
            std::set<std::string> nodes = client->allNodes();
            for (auto&& node : nodes)
            {
                writer.String(node.c_str());
            }
            writer.EndArray();
            res.set_content(generateResponseContent(200, fmt::format("OPC节点[{}]查询成功", machine), sb.GetString()),
                            "application/json");
        }
        else if ("node" == type)
        {
            std::string code = req.get_param_value("code");
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            std::string name;
            std::string type;
            std::string value;
            client->getNode(code, name, type, value);
            rapidjson::StringBuffer sb;
            rapidjson::Writer writer(sb);
            writer.StartObject();
            writer.Key("code");writer.String(code.c_str());
            writer.Key("name");writer.String(name.c_str());
            writer.Key("type");writer.String(type.c_str());
            writer.Key("value");writer.String(value.c_str());
            writer.EndObject();
            res.set_content(
                generateResponseContent(200, fmt::format("OPC客户端[{}]节点[{}]查询成功", machine, code), sb.GetString(),true),
                "application/json");
        }
        else if ("url" == type)
        {
            std::scoped_lock lock(mClientsMutex);
            auto iter = mClients.find(machine);
            if (iter == mClients.end())
            {
                OPCClientNotExistException exception(fmt::format("OPC客户端[{}]不存在", machine));
                exception.rethrow();
            }
            auto client = iter->second;
            auto url = client->url();
            res.set_content(generateResponseContent(200, fmt::format("OPC客户端[{}]URL查询成功", machine), url),
                            "application/json");
        }
        else
        {
            HttpUnsupportedSearchType e(fmt::format("OPCClient不支持当前查询种类：{}", type));
            e.rethrow();
        }
    });

    mpHttpServer->set_exception_handler(
        [this](const httplib::Request& req, httplib::Response& res, const std::exception_ptr& ep)
        {
            std::string result;
            try
            {
                std::rethrow_exception(ep);
            }
            catch (Exception& e)
            {
                result = generateResponseContent(500, e.message());
            }
            catch (std::exception& e)
            {
                result = generateResponseContent(500, e.what());
            }
            res.set_content(result.c_str(), "application/json");
        });

    mpHttpServer->set_error_handler([](const httplib::Request& req, httplib::Response& res)
    {
        std::string machine = req.get_param_value("machine");
        std::string code = req.get_param_value("code");
        std::string value = req.get_param_value("value");
        const char* fmt = fmt::format("{{发送指令[machine:{}, code:{}, value:{}]失败:{}}}", machine, code, value, "服务器错误！").
            c_str();
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
        res.set_content(buf, "application/json");
    });
}
