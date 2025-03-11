//
// Created by 张腾 on 25-3-10.
//

#ifndef ASCENDINGMESSAGEHANDLER_H
#define ASCENDINGMESSAGEHANDLER_H

#include <QObject>
#include "cpp-httplib/httplib.h"
#include "GlobalDefine.h"
#include <thread>
class AscendingMessageHandler : public QObject
{
    Q_OBJECT

public:

    explicit AscendingMessageHandler(QObject* parent = 0);

    ~AscendingMessageHandler() override;

    void setPort(int port);

    int port() const;

signals:

    void setDataValue(const std::string& code,const Data& data);

private:

    void initServer();

    std::atomic<int> mPort = 1234;

    httplib::Server* mpServer = nullptr;

    std::thread* mpThread = nullptr;
};

#endif //ASCENDINGMESSAGEHANDLER_H
