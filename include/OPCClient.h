//
// Created by cumtzt on 25-1-20.
//

#ifndef OPCCLIENT_OPCCLIENT_H
#define OPCCLIENT_OPCCLIENT_H

#include <open62541pp/open62541pp.hpp>
#include <cstdio>
#include <mutex>
#include <QTimer>
#include <QStringList>
#include <QThread>
#include <yaml-cpp/node/convert.h>

class OPCClient : public QObject{

    Q_OBJECT

public:

    explicit OPCClient(QObject* parent = nullptr);

    ~OPCClient()override;

    void setUrl(const QString& url);

    QString getURL();

    void setInterval(int interval);

    int getInterval();

    void setNodeIds(const QStringList& nodeIds);

    QStringList getNodeIds();

private slots:

    void onTimerTimeout();

private:

    QTimer* mpTimer = nullptr;

    QString mUrl;

    opcua::Client *mpClient = nullptr;

    std::recursive_mutex mClientLocker;

    QStringList mNodeIds;

    QThread* mpThread = nullptr;

};
#endif //OPCCLIENT_OPCCLIENT_H
