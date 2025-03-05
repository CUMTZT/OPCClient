//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541pp/open62541pp.hpp>
#include <cstdio>
#include <mutex>
#include <QThread>
#include <yaml-cpp/node/convert.h>
#include <QRunnable>

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

    std::set<std::string> getNodes();

    void setDist(const std::string& dist);

    std::string getDist();

    void start();

    void stop();

signals:
    void newData(const std::string& dist,const std::string& source, const std::vector<std::pair<std::string, std::string>>& datas);

private:
    void run() override;

    void connectServer();

    std::string mUrl;

    std::string mCode;

    std::string mDestination;

    opcua::Client *mpClient = nullptr;

    std::recursive_mutex mClientLocker;

    std::set<std::string> mNodes;

    std::atomic<bool> mRunning = false;
};
#endif //OPCCLIENT_OPCCLIENT_H
