#ifndef ZABERSTAGECTRL_H
#define ZABERSTAGECTRL_H

#include <QObject>
#include <QString>

#include "masterthread.h"

enum ZaberMsgType{
    NONE,GET_POSITION
};

class ZaberStageCtrl : public QObject
{
    Q_OBJECT

    MasterThread *m_port;
public:
    explicit ZaberStageCtrl(const QString&);
    ~ZaberStageCtrl();

    void setAcc(int);void setVel(int);
    void moveAbs(int);
signals:

public slots:
    void connectStatus(bool);
};

struct ZaberMsgObj{
    ZaberMsgType type;

    ZaberMsgObj(ZaberMsgType t){
        type=t;
    }
};

#endif // ZABERSTAGECTRL_H
