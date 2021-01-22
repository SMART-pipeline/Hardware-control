#include "zaberstagectrl.h"

#include <QDebug>
#include <QStringList>

ZaberStageCtrl::ZaberStageCtrl(const QString &name)
{
    m_port=new MasterThread(name,100,"\n",115200);
    connect(m_port,SIGNAL(connectStatus(bool)),this,SLOT(connectStatus(bool)));
}
ZaberStageCtrl::~ZaberStageCtrl(){
    delete m_port;
}

void ZaberStageCtrl::connectStatus(bool bConnected){
    qDebug()<<"ZaberStageCtrl connectStatus"<<bConnected;
}

void ZaberStageCtrl::setAcc(int acc){
    m_port->addMsg(QString("/set accel ")+QString::number(acc),
                   new ZaberMsgObj(ZaberMsgType::NONE));
}
void ZaberStageCtrl::setVel(int vel){
    m_port->addMsg(QString("/set maxspeed ")+QString::number(vel),
                   new ZaberMsgObj(ZaberMsgType::NONE));
}
void ZaberStageCtrl::moveAbs(int pos){
    m_port->addMsg(QString("/move abs ")+QString::number(pos),new ZaberMsgObj(ZaberMsgType::NONE));
}
