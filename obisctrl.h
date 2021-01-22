#ifndef OBISCTRL_H
#define OBISCTRL_H

#include <QMap>

#include "masterthread.h"

enum OBISCTRL_MSGTYPE{
    SET_ONOFF,SET_POWER,GET_WAVELENGTH,GET_ONOFF,GET_POWER,GET_MAXPOWER,NONE
};
struct MsgObj;

QT_BEGIN_NAMESPACE
class ObisDialog;
QT_END_NAMESPACE

class ObisCtrl : public QObject
{
    Q_OBJECT

    QMap<int,int> m_laserPorts;MasterThread *m_pSerialPort;

    QMap<int,MasterThread*> m_pSerialPorts;
    ObisDialog *m_dialog;

    int portWavelen(MasterThread*);
    bool setPowerOnOff(MasterThread*,int,bool);

    bool setPower(MasterThread*,int power);

    bool isLaserBox();
public:
    explicit ObisCtrl();
    ~ObisCtrl();

    bool initAll();
    bool setPowerOnOff(int,int,bool);
signals:
    void connected(int);
public slots:
    void handleReadyRead(QString s,void *obj);

    void powerOffAll();
    void setOnOff(int wavelength,bool bOn);
    void setPower(int wavelength,int power);

    void connectStatus(bool);
};

struct MsgObj{
    QString msg;
    OBISCTRL_MSGTYPE type;

    MsgObj(const QString &msgStr,OBISCTRL_MSGTYPE t){
        msg=msgStr;type=t;
    }
};

#endif // OBISCTRL_H
