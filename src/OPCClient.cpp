//
// Created by cumtzt on 25-1-20.
//
#include "OPCClient.h"
#include <rapidjson/document.h>
#include "Logger.h"
#include <yaml-cpp/yaml.h>
#include <open62541/client_highlevel.h>

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
    UA_Client_delete(mpClient);
    mpClient = nullptr;
    mpThread->deleteLater();
    mpThread = nullptr;
}

QString OPCClient::getURL() {
    return mUrl;
}

void OPCClient::setUrl(const QString &url) {
    std::scoped_lock lock(mClientLocker);
    mUrl = url;
    if (url.isEmpty()) {
        UA_Client_delete(mpClient);
        mpClient = nullptr;
        return;
    }
    if (nullptr != mpClient) {
        UA_Client_delete(mpClient);
        mpClient = nullptr;
    }
    mpClient = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(mpClient));
    UA_StatusCode status = UA_Client_connect(mpClient, url.toStdString().c_str());
    if (status != UA_STATUSCODE_GOOD) {
        UA_Client_delete(mpClient);
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
        for(const auto& id : mNodeIds) {
            UA_Variant value; /* Variants can hold scalar values and arrays of any type */
            UA_Variant_init(&value);
            UA_StatusCode status = UA_Client_readValueAttribute(mpClient, UA_NODEID_STRING(1, const_cast<char*>(id.toStdString().c_str())), &value);
            QString strValue;
            if (status == UA_STATUSCODE_GOOD) {
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
                    bool result = *((UA_Boolean*)value.data);
                    if(result){
                        strValue = "true";
                    }
                    else{
                        strValue = "false";
                    }
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
                    strValue = QString((char*)((UA_String *) value.data)->data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_SBYTE])) {
                    strValue = QString::number(*(UA_SByte*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
                    strValue = QString::number(*(UA_Int16*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
                    strValue = QString::number(*(UA_Int32*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT64])) {
                    strValue = QString::number(*(UA_Int64*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTE])) {
                    strValue = QString::number(*(UA_Byte*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
                    strValue = QString::number(*(UA_UInt16*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
                    strValue = QString::number(*(UA_UInt32*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT64])) {
                    strValue = QString::number(*(UA_UInt64*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
                    strValue = QString::number(*(UA_Double*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
                    strValue = QString::number(*(UA_Float*)value.data);
                }
                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
                    strValue = QString::number(*(UA_DateTime*)value.data);
                }

                if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BYTESTRING])) {
                    strValue = QString((char*)((UA_String *) value.data)->data);
                }
                LogErr("{} : {}",id.toStdString(),strValue.toStdString());

                int value_ = strValue.toInt();
                value_++;
                UA_Variant *myVariant = UA_Variant_new();
                UA_Variant_setScalarCopy(myVariant, &value_, &UA_TYPES[UA_TYPES_INT32]);
                UA_Client_writeValueAttribute(mpClient, UA_NODEID_STRING(1, const_cast<char*>("the.answer")), myVariant);
                UA_Variant_delete(myVariant);
            }
            UA_Variant_clear(&value);
        }
    }
}
