//
// Created by cumtzt on 25-2-24.
//

#ifndef OPCCLIENTMANAGER_H
#define OPCCLIENTMANAGER_H
#include "OPCClient.h"
#include "KafkaProducer.h"
#include "cpp-httplib/httplib.h"

DECLARE_EXCEPTION(OPCClientNotExistException, ExistsException)
DECLARE_EXCEPTION(HttpRuntimeError, RuntimeException)

class OPCClientManager : public QObject {
    Q_OBJECT

public:
    static OPCClientManager* getInstance();

    OPCClientManager(OPCClientManager const &) = delete;

    OPCClientManager(OPCClientManager &&) = delete;

    OPCClientManager &operator=(OPCClientManager const &) = delete;

    ~OPCClientManager() override;

    void loadConfig(const std::string &configFile);

private:

    OPCClientManager();

    void initHttpServer();

    void stopHttpServer();

    std::string generateResponseContent(const std::string &message,int code);

    static OPCClientManager* mpInstance;

    static std::recursive_mutex mMutex;

    std::unordered_map<std::string,std::shared_ptr<OPCClient>>mClients;

    std::recursive_mutex mClientsMutex;

    KafkaProducer* mpKafkaProducer = nullptr;

    QThread* mpKafkaProducerThread = nullptr;

    YAML::Node mConfig;

    httplib::Server* mpHttpServer = nullptr;

    std::thread* mpHttpServerThread = nullptr;
};

#define OPCClientManagerIns OPCClientManager::getInstance()

#endif //OPCCLIENTMANAGER_H
