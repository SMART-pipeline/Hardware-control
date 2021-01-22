#include "hardware.h"
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);

    Hardware hardware;

    if(!hardware.connectDevices()){qWarning()<<"Unable to connect to devices";return -1;}

    return a.exec();
}
