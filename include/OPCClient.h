//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541pp/open62541pp.hpp>
#include "GlobalDefine.h"
#include <mutex>
#include <QThread>
#include <yaml-cpp/node/convert.h>
#include <QRunnable>
#include "GlobalDefine.h"

class OPCClient : public QThread {
    Q_OBJECT

public:
    explicit OPCClient(QObject *parent = nullptr);

    ~OPCClient() override;

    void setCode(const std::string& code);

    std::string getCode();

    void setUrl(const std::string &url);

    std::string getURL();

    void addNode(const std::string &node);

    std::set<std::string> nodes();

    void setTopic(const std::string& topic);

    std::string topic();

    void setInterval(int interval);

    int interval();

    void start();

    void stop();

    void setDataValue(const Data& data);

signals:
    void newData(const std::string& topic,const std::string& code, const DataList& datas);

private:
    void run() override;

    void connectServer();

    std::string mUrl;

    std::string mCode;

    std::string mTopic;

    std::atomic<int> mInterval;

    opcua::Client *mpClient = nullptr;

    std::recursive_mutex mClientLocker;

    std::set<std::string> mNodes;

    std::atomic<bool> mRunning = false;

    QTimer* mpReconnectTimer = nullptr;

    QThread* mpReconnectThread = nullptr;//TODO,改成Timer触发，slot在Thread里执行
};
#endif //OPCCLIENT_OPCCLIENT_H
