/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */


#include <QCoreApplication>
#include "KafkaProducer.h"
#include "Logger.h"
#include "OPCClientManager.h"
#include <yaml-cpp/yaml.h>
#include "QDir"
#include "QDebug"

QString configFile = "./config/config.yml";
void parseArgv(int argc, char *argv[]) {
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            configFile = argv[++i];
        }
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc,argv);
    parseArgv(argc,argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    qDebug()<<QCoreApplication::applicationDirPath();
    LoggerIns.loadConfig(configFile.toStdString());
    OPCClientManagerIns->loadConfig(configFile.toStdString());
    return a.exec();
}