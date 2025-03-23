//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541pp/open62541pp.hpp>
#include "GlobalDefine.h"
#include <yaml-cpp/node/convert.h>
#include <QTimer>
#include <QThread>
#include "Exception.h"

DECLARE_EXCEPTION(OPCServerNotConnectException,RuntimeException)
DECLARE_EXCEPTION(OPCNodeCodeFormatErrorException, RuntimeException)
DECLARE_EXCEPTION(OPCNodeNotExistException,RuntimeException)
DECLARE_EXCEPTION(OPCNodeTypeNotSupportException,RuntimeException)

class Machine : public QThread {
    Q_OBJECT

public:
    explicit Machine(QObject *parent = nullptr);

    ~Machine() override;

    void setCode(const std::string& code);

    std::string code();

    void setUrl(const std::string &url);

    [[nodiscard]] std::string url() const;

    void addNode(const std::string &node);

    void removeNode(const std::string &node);

    std::set<std::string> nodes();

    void setTopic(const std::string& topic);

    std::string topic();

    void setInterval(int interval);

    int interval();

    void start();

    void stop();

    void setNodeValue(const std::string& nodeCode,const std::string& value);

    void getNode(const std::string &nodeCode,std::string& name,std::string& type,std::string& value);

signals:
    void newData(const std::string& topic,const std::string& code, const std::vector<std::pair<std::string,std::string>>& datas);

private slots:

    void connectServer();

private:

    void run() override;

    bool isConnected();

    std::string mUrl;

    std::string mMachineCode;

    std::string mTopic;

    std::unique_ptr<opcua::Client> mpClient = nullptr;

    std::mutex mClientLocker;

    std::set<std::string> mNodeCodes;

    QTimer* mpReconnectTimer = nullptr;

    std::atomic<int> mInterval = 1000;
};
#endif //OPCCLIENT_OPCCLIENT_H
