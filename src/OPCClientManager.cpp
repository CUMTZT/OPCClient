//
// Created by cumtzt on 25-2-24.
//
#include "OPCClientManager.h"
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <QDateTime>
#include "../include/Logger.h"

OPCClientManager *OPCClientManager::mpInstance = nullptr;
std::recursive_mutex OPCClientManager::mMutex;

OPCClientManager *OPCClientManager::getInstance()
{
    if (nullptr == mpInstance)
    {
        std::scoped_lock lock(mMutex);
        if (nullptr == mpInstance)
        {
            mpInstance = new OPCClientManager();
        }
    }
    return mpInstance;
}

OPCClientManager::OPCClientManager() : QObject(nullptr)
{
    mpKafkaProducer = new KafkaProducer();
    mpKafkaProducerThread = new QThread(this);
    mpKafkaProducer->moveToThread(mpKafkaProducerThread);
    mpKafkaProducerThread->start();

    mpHttpServer = new httplib::Server();
    mpHttpServer->Get("/update", [this](const httplib::Request &req, httplib::Response &res)
                      {
        LogInfo("update");
        std::string machine = req.get_param_value("machine");
        std::string code = req.get_param_value("code");
        std::string value = req.get_param_value("value");
        try {
            onSetDataValue(machine, {code, value});
            res.status = 200;
            res.set_content("Success!\n", "text/plain");
        }
        catch (...) {
            const char *fmt = "<p>Exception Occured: <span style='color:red;'>%s</span></p>";
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
            res.status = 500;
            res.set_content(buf, "text/html");
        } });

    mpHttpServer->set_exception_handler(
        [](const httplib::Request & /*req*/, httplib::Response &res, const std::exception_ptr &ep)
        {
            const char *fmt = "<p>Exception Occured: <span style='color:red;'>%s</span></p>";
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
            res.status = 500;
            res.set_content(buf, "text/html");
        });

    mpHttpServer->set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res)
                                    {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html"); });
    mpHttpServerThread = new std::thread([=, this](){
        if(mpHttpServer->listen("localhost", 1234)){
            LogInfo("Listen Success On{}", 1234);
        } 
    });
    mpHttpServerThread->detach();
}

OPCClientManager::~OPCClientManager()
{
}

void OPCClientManager::loadConfig(const std::string &configFile)
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
            mpHttpServerThread = new std::thread([=, this]()
                                                 {
                LogInfo("Listen:{}", port);
                mpHttpServer->listen("localhost", port); });
            mpHttpServerThread->detach();
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

                auto client = new OPCClient(this);
                client->setCode(code);
                client->setTopic(topic);
                client->setUrl(server);
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
                        for (int j = 0; j < nodeConfig.size(); j++)
                        {
                            if (!nodeConfig[j]["code"])
                            {
                                LogErr("配置文件中不存在code！");
                                continue;
                            }
                            node_code = nodeConfig[j]["code"].as<std::string>();
                            client->addNode(node_code);
                            if (!nodeConfig[j]["name"])
                            {
                                LogErr("配置文件中不存在name！");
                                continue;
                            }
                            auto node_name = nodeConfig[j]["name"].as<std::string>();
                        }
                    }
                    catch (YAML::Exception &e)
                    {
                        LogErr("{}", e.msg);
                    }
                }
                else
                {
                    LogWarn("OPCClient配置中不存在nodes节点!");
                }
                mClients.try_emplace(code, client);
            }
        }
        else
        {
            LogWarn("配置文件中不存在OPC客户端配置！");
        }
        mpKafkaProducer->loadConfig(configFile);
    }
    catch (const YAML::Exception &e)
    {
        LogErr("{}", e.msg);
    }
}

void OPCClientManager::onSetDataValue(const std::string &code, const Data &data)
{
    std::scoped_lock lock(mClientsMutex);
    auto iter = mClients.find(code);
    if (iter == mClients.end())
    {
        LogErr("上行指令没有找到对应客户端：{}", code);
    }
    auto client = iter->second;
    try
    {
        client->setDataValue(data);
    }
    catch (...)
    {
        std::rethrow_exception(std::current_exception());
    }
}
