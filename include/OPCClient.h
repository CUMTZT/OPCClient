//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <QObject>
#include <QTimer>
#include <open62541pp/open62541pp.hpp>
#include "GlobalDefine.h"
#include <mutex>
#include <yaml-cpp/node/convert.h>
#include <thread>

class OPCClient : public QObject {
    Q_OBJECT

public:
    explicit OPCClient(QObject *parent = nullptr);

    ~OPCClient() override;

    void setCode(const std::string& code);

    std::string code();

    void setUrl(const std::string& url);

    std::string url();

    void addNode(const std::string &node);

    std::map<std::string,std::string> nodes();

    void setTopic(const std::string& topic);

    std::string topic();

    void setDataValue(const Data& data);

private slots:

    void connectServer();

private:

    std::pair<bool,std::string> mUrl = {false,""};

    std::string mCode;

    std::string mTopic;

    opcua::Client *mpClient = nullptr;

    std::thread* mpThread = nullptr;

    std::mutex mClientLocker;

    std::map<std::string,std::string> mNodes;

    std::mutex mNodesMutex;

    QTimer* mpReconnectTimer = nullptr;
};
#endif //OPCCLIENT_OPCCLIENT_H
