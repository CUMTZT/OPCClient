/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */


#include <QCoreApplication>
#include "OPCClient.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc,argv);
    OPCClient client;
    return a.exec();
}