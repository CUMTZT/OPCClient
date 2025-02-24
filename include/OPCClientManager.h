//
// Created by cumtzt on 25-2-24.
//

#ifndef OPCCLIENTMANAGER_H
#define OPCCLIENTMANAGER_H
#include <QMap>
#include <QObject>
#include "OPCClient.h"
class OPCClientManager : public QObject {

    Q_OBJECT

public:
    static OPCClientManager* getInstance();

    OPCClientManager(OPCClientManager const&) = delete;

    OPCClientManager(OPCClientManager&&) = delete;

    OPCClientManager& operator=(OPCClientManager const&) = delete;

    ~OPCClientManager() override;

private:

    OPCClientManager();

    static OPCClientManager* mpInstance;

    static std::mutex mMutex;

    QMap<QString,OPCClient*> mClientMap;

    std::recursive_mutex mClientMapMutex;

    YAML::Node mConfig;
};
#endif //OPCCLIENTMANAGER_H
