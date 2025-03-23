//
// Created by cumtzt on 25-2-24.
//

#ifndef OPCCLIENTMANAGER_H
#define OPCCLIENTMANAGER_H
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include "Exception.h"
#include "OPCClient.h"
#include "KafkaProducer.h"
#include "cpp-httplib/httplib.h"
#include "Machine.h"

DECLARE_EXCEPTION(OPCClientNotExistException, ExistsException)
DECLARE_EXCEPTION(HttpRuntimeError, RuntimeException)
DECLARE_EXCEPTION(HttpUnsupportedSearchType, HttpRuntimeError)

class OPCClient : public QObject {
    Q_OBJECT

public:
    static OPCClient* getInstance();

    OPCClient(OPCClient const &) = delete;

    OPCClient(OPCClient &&) = delete;

    OPCClient &operator=(OPCClient const &) = delete;

    ~OPCClient() override;

    void loadConfig(const std::string &configFile);

private:

    OPCClient();

    void initHttpServer();

    void stopHttpServer();

    std::string generateResponseContent(int code, const std::string &message, const std::string& data = "",bool isRaw = false);

    static OPCClient* mpInstance;

    static std::recursive_mutex mMutex;

    std::unordered_map<std::string,std::shared_ptr<Machine>>mClients;

    std::recursive_mutex mClientsMutex;

    KafkaProducer* mpKafkaProducer = nullptr;

    QThread* mpKafkaProducerThread = nullptr;

    YAML::Node mConfig;

    httplib::Server* mpHttpServer = nullptr;

    std::thread* mpHttpServerThread = nullptr;
};

#define OPCClientManagerIns OPCClient::getInstance()

#endif //OPCCLIENTMANAGER_H
