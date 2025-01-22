//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541/client_config_default.h>
#include <cstdio>
#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <QObject>
#include <mutex>
#include <QTimer>
#include <QStringList>
#include <QThread>
class OPCClient : public QObject{

    Q_OBJECT

public:

    explicit OPCClient(QObject* parent = nullptr);

    ~OPCClient()override;

    void connect(const QString& url);

    void setInterval(int interval);

    void setNodeIds(const QStringList& nodeIds);

private slots:

    void onTimerTimeout();

private:

    QTimer* mpTimer = nullptr;

    QString mUrl;

    UA_Client *mpClient = nullptr;

    std::recursive_mutex mClientLocker;

    QStringList mNodeIds;

    QThread* mpThread = nullptr;

};
#endif //OPCCLIENT_OPCCLIENT_H
