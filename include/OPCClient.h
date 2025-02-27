//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541pp/open62541pp.hpp>
#include <cstdio>
#include <mutex>
#include <QObject>
#include <yaml-cpp/node/convert.h>
#include <QRunnable>

struct OPCData {
    std::string name;
    std::string value;
    int type;
    int id;
    int namespaceIndex;
};

class OPCClient : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit OPCClient(QObject *parent = nullptr);

    ~OPCClient() override;

    void setID(int id);

    int getID();

    void setName(const std::string& name);

    std::string getName();

    void setUrl(const std::string &url);

    std::string getURL();

    void addNode(const std::pair<int,int> &nodeIds);

    std::set<std::pair<int,int>> getNodes();

    void setDist(const std::string& dist);

    std::string getDist();

signals:
    void newMessage(const std::vector<OPCData> &message);

private:
    void connectServer();

    void run() override;

    std::string mUrl;

    std::string mName;

    int mID;

    std::string mDestination;

    opcua::Client *mpClient = nullptr;

    std::recursive_mutex mClientLocker;

    std::set<std::pair<int,int>> mNodes;
};
#endif //OPCCLIENT_OPCCLIENT_H
