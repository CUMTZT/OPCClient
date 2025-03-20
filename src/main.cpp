/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */


#include <QCoreApplication>
#include "KafkaProducer.h"
#include "Logger.h"
#include "OPCClient.h"
#include <yaml-cpp/yaml.h>
#include "QDir"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc,argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    std::string configFile = "./config/config.yml";
    LoggerIns.loadConfig(configFile);
    OPCClientManagerIns->loadConfig(configFile);
    return a.exec();
}