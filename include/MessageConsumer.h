//
// Created by cumtzt on 2/27/25.
//

#ifndef MESSAGECONSUMER_H
#define MESSAGECONSUMER_H
#include <QObject>

class MessageConsumer : public QObject {
    Q_OBJECT

public:
    explicit MessageConsumer(QObject *parent = nullptr) : QObject(parent){}

public slots:

    virtual void onNewMessage(const std::string& dist, const std::string& message) = 0;

};
#endif //MESSAGECONSUMER_H
