//
// Created by cumtzt on 25-2-24.
//

#ifndef OPCCLIENTMANAGER_H
#define OPCCLIENTMANAGER_H
#include <QMap>
#include <QThreadPool>
#include <QThread>
#include <QTimer>
#include "OPCClient.h"
#include "KafkaProducer.h"
#include <map>
#include "AscendingMessageHandler.h"
class OPCClientManager : public QObject {
    Q_OBJECT

public:
    static OPCClientManager* getInstance();

    OPCClientManager(OPCClientManager const &) = delete;

    OPCClientManager(OPCClientManager &&) = delete;

    OPCClientManager &operator=(OPCClientManager const &) = delete;

    ~OPCClientManager() override;

    void loadConfig(const std::string &configFile);

    void onSetDataValue(const std::string& code, const Data& data);

private:

    OPCClientManager();

    static OPCClientManager* mpInstance;

    static std::recursive_mutex mMutex;

    std::unordered_map<std::string,OPCClient*>mClients;

    std::recursive_mutex mClientsMutex;

    KafkaProducer* mpKafkaProducer = nullptr;

    QThread* mpKafkaProducerThread = nullptr;

    YAML::Node mConfig;

    AscendingMessageHandler* mpAscendingMessageHandler = nullptr;
};

#define OPCClientManagerIns OPCClientManager::getInstance()
#endif //OPCCLIENTMANAGER_H
