//
// Created by cumtzt on 25-2-24.
//
#include "OPCClientManager.h"
#include <yaml-cpp/yaml.h>
#include "Logger.h"
#include "rapidjson/prettywriter.h"
#include <QDateTime>

std::optional<std::string> getExceptionPtrMsg(const std::exception_ptr& eptr) // passing by value is ok
{
    try {
        if (eptr) {
            std::rethrow_exception(eptr);
        }
    } catch(const std::exception& e) {
        return e.what();
    }
    return {};
}

OPCClientManager* OPCClientManager::mpInstance = nullptr;

std::recursive_mutex OPCClientManager::mMutex;

OPCClientManager* OPCClientManager::getInstance()
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
    mpHttpServer->Get("/update", [this](const httplib::Request& req, httplib::Response& res)
    {
        std::string machine = req.get_param_value("machine");
        std::string code = req.get_param_value("code");
        std::string value = req.get_param_value("value");
        try{
            {
                std::scoped_lock lock(mClientsMutex);
                auto iter = mClients.find(machine);
                if (iter == mClients.end())
                {
                    throw std::runtime_error(fmt::format("上行指令没有找到对应客户端：{}", machine));
                }
                auto client = iter->second;
                client->setDataValue({code, value});
            }
            res.status = 200;
            res.set_content("<p>指令上行成功<span style='color:green;'>%s</span></p>", "text/html");
        }
        catch (std::exception &e)
        {
            const char* fmt = fmt::format("<p>发送上行指令失败！原因：{}<span style='color:red;'>%s</span></p>",e.what()).c_str();
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
            res.status = 500;
            res.set_content(buf, "text/html");
        }
    });

    mpHttpServer->set_exception_handler(
        [](const httplib::Request& /*req*/, httplib::Response& res, const std::exception_ptr& ep)
        {
            std::optional<std::string> exceptionPtrMsg = getExceptionPtrMsg(ep);
            if (!exceptionPtrMsg.has_value()){
                return;
            }
            const char* fmt = fmt::format("<p>发送上行指令失败！原因：{}<span style='color:red;'>%s</span></p>",exceptionPtrMsg.value()).c_str();
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
            res.status = 500;
            res.set_content(buf, "text/html");
        });

    mpHttpServer->set_error_handler([](const httplib::Request& /*req*/, httplib::Response& res)
    {
        const char* fmt = fmt::format("<p>发送上行指令失败！原因：未知<span style='color:red;'>%s</span></p>").c_str();
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, std::strerror(errno));
        res.status = 500;
        res.set_content(buf, "text/html");
    });
}

OPCClientManager::~OPCClientManager()
{
    for (auto& client : mClients)
    {
        client.second->stop();
    }
}

void OPCClientManager::loadConfig(const std::string& configFile)
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

                auto client = std::make_shared<OPCClient>();
                client->setUrl(server);
                client->setCode(code);
                client->setTopic(topic);
                client->setInterval(interval);
                connect(client.get(), &OPCClient::newData, mpKafkaProducer, &KafkaProducer::onNewDatas);
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
                        for (auto && j : nodeConfig)
                        {
                            node_code = j.as<std::string>();
                            client->addNode(node_code);
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

void OPCClientManager::stopHttpServer()
{
    if (mpHttpServer->is_running())
    {
        mpHttpServer->stop();
        mpHttpServerThread->join();
        delete mpHttpServerThread;
        mpHttpServerThread = nullptr;
    }
}
