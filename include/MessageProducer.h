//
// Created by cumtzt on 2/27/25.
//

#ifndef MESSAGEPRODUCER_H
#define MESSAGEPRODUCER_H
#include <QObject>

class MessageProducer : public QObject {
    Q_OBJECT

public:
    explicit MessageProducer(QObject *parent = nullptr) : QObject(parent){}

signals:

    void newMessage(const std::string& dist, const std::string& message);
};
#endif //MESSAGEPRODUCER_H
