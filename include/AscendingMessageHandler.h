//
// Created by 张腾 on 25-3-10.
//

#ifndef ASCENDINGMESSAGEHANDLER_H
#define ASCENDINGMESSAGEHANDLER_H

#include <QObject>
#include "cpp-httplib/httplib.h"
#include "GlobalDefine.h"

class AscendingMessageHandler : public QObject
{
    Q_OBJECT

public:

    explicit AscendingMessageHandler(QObject* parent = 0);

signals:
    void setDataValue(const std::string& code,const Data& data);
private:
    httplib::Server* mpServer;
};

#endif //ASCENDINGMESSAGEHANDLER_H
