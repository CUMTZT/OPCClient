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

class OPCClientManager : public QObject {
    Q_OBJECT

public:
    static OPCClientManager *getInstance();

    OPCClientManager(OPCClientManager const &) = delete;

    OPCClientManager(OPCClientManager &&) = delete;

    OPCClientManager &operator=(OPCClientManager const &) = delete;

    ~OPCClientManager() override;

    void loadConfig(const std::string &configFile);

//private slots:

    //void onTimerTimeout();

private:
    OPCClientManager();

    static OPCClientManager *mpInstance;

    static std::mutex mMutex;

    QTimer* mpTimer = nullptr;

    std::vector<OPCClient*>mClients;

    std::recursive_mutex mClientsMutex;

    KafkaProducer* mpKafkaProducer = nullptr;

    QThread* mpKafkaProducerThread = nullptr;

    YAML::Node mConfig;
};

#define OPCClientManagerIns OPCClientManager::getInstance()
#endif //OPCCLIENTMANAGER_H
