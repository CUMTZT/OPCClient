//
// Created by cumtzt on 25-2-24.
//

#ifndef OPCCLIENTMANAGER_H
#define OPCCLIENTMANAGER_H
#include <QMap>
#include <QThreadPool>
#include <QThread>
#include <QTimer>
#include "MessageConsumer.h"
#include "MessageProducer.h"
#include "OPCClient.h"
#include <map>

class OPCClientManager : public MessageProducer {
    Q_OBJECT

public:
    static OPCClientManager *getInstance();

    OPCClientManager(OPCClientManager const &) = delete;

    OPCClientManager(OPCClientManager &&) = delete;

    OPCClientManager &operator=(OPCClientManager const &) = delete;

    ~OPCClientManager() override;

    void loadConfig(const std::string &configFile);

private slots:
    void onClientNewMessage(const std::vector<OPCData>& message);

    void onTimerTimeout();

private:
    OPCClientManager();

    static OPCClientManager *mpInstance;

    static std::mutex mMutex;

    QTimer* mpTimer = nullptr;

    std::map<std::string,OPCClient*>mClientMap;

    std::recursive_mutex mClientMapMutex;

    YAML::Node mConfig;

    QThreadPool *mpThreadPool = nullptr;

    int mStationId;

    std::string mStationName;
};

#define OPCClientManagerIns OPCClientManager::getInstance()
#endif //OPCCLIENTMANAGER_H
