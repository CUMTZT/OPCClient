//
// Created by cumtzt on 25-1-20.
//
#include "OPCClient.h"
#include <rapidjson/document.h>
#include <yaml-cpp/yaml.h>
#include "Logger.h"

OPCClient::OPCClient(QObject *parent) : QObject(parent) {
    mpThread = new QThread();
    this->moveToThread(mpThread);
    mpThread->start();
    mpTimer = new QTimer();
    mpTimer->setInterval(1000);
    connect(mpTimer,&QTimer::timeout,this,&OPCClient::onTimerTimeout);
    mpTimer->start();
}

OPCClient::~OPCClient() {
    std::scoped_lock lock(mClientLocker);
    delete mpClient;
    mpClient = nullptr;
    mpThread->deleteLater();
    mpThread = nullptr;
}

QString OPCClient::getURL() {
    return mUrl;
}

void OPCClient::setUrl(const QString &url) {
    std::scoped_lock lock(mClientLocker);
    if (mUrl == url) {
        return;
    }
    if (nullptr != mpClient && mpClient->isConnected()) {
        mpClient->disconnect();
        delete mpClient;
        mpClient = nullptr;
    }
    mUrl = url;
    if (mUrl.isEmpty()) {
        return;
    }
    mpClient = new opcua::Client();
    mpClient->connect(mUrl.toStdString());
    if (!mpClient->isConnected()) {
        delete mpClient;
        mpClient = nullptr;
    }
}

void OPCClient::setInterval(int interval) {
    mpTimer->setInterval(interval);
}

int OPCClient::getInterval() {
    return mpTimer->interval();
}

void OPCClient::setNodeIds(const QStringList& nodeIds){
    mNodeIds = nodeIds;
}

QStringList OPCClient::getNodeIds() {
    return mNodeIds;
}

void OPCClient::onTimerTimeout() {
    std::scoped_lock lock(mClientLocker);
    if (nullptr == mpClient) {
        setUrl(mUrl);
    }
    if (nullptr != mpClient) {
        try {
            opcua::Node node(*mpClient, opcua::ObjectId::RootFolder);
            auto node_ = node.browseChild({{1,"the.answer"}});
        }
        catch (std::exception& e) {
            LogErr("{}",e.what());
        }
    }
}
