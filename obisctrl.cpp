#include "obisctrl.h"

#include <QSerialPortInfo>
#include <QStringList>
#include <QDebug>

ObisCtrl::ObisCtrl():m_pSerialPort(nullptr){
    m_laserPorts[405]=1;m_laserPorts[488]=2;m_laserPorts[561]=3;m_laserPorts[640]=5;
}

ObisCtrl::~ObisCtrl(){}

bool ObisCtrl::isLaserBox(){return !m_laserPorts.empty();}

void ObisCtrl::powerOffAll(){
    QMapIterator<int,MasterThread*> iter(m_pSerialPorts);
    while(iter.hasNext()){
        iter.next();

        qDebug()<<setPowerOnOff(iter.key(),-1,0);
    }
}

bool ObisCtrl::initAll(){
    QStringList portNames;

    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
        QString name=port.portName();

        if(port.manufacturer()=="Coherent, Inc."
                 &&port.description()=="Coherent OBIS Device"
                 &&!portNames.contains(name)){
            qDebug()<<"will connect to port"<<name<<"as OBIS device";

            MasterThread* p=new MasterThread(name,100,"\r\n",-1);
            connect(p,SIGNAL(connectStatus(bool)),this,SLOT(connectStatus(bool)));
        }
    }

    return true;
}
bool ObisCtrl::setPowerOnOff(MasterThread* p, int power, bool bOn){
    if(NULL==p){return false;}

    bool bOK=true;
    if(power>0){bOK&=setPower(p,power);}
    p->addMsg(QString("SOURce:AM:STATe ")+(bOn?"ON":"OFF"),
              new MsgObj("",OBISCTRL_MSGTYPE::SET_ONOFF));
    return bOK;
}
bool ObisCtrl::setPowerOnOff(int waveLength, int power, bool bOn){
    if(!m_pSerialPorts.contains(waveLength)){qWarning()<<waveLength<<"is not connected";return false;}

    if(!isLaserBox()){
        MasterThread* p=m_pSerialPorts[waveLength];return setPowerOnOff(p,power,bOn);
    }else{
        //to-check
        int id=m_laserPorts[waveLength];
        if(power>0){setPower(waveLength,power);}
        m_pSerialPort->addMsg(QString("SOURce"+QString::number(id)+":AM:STATe ")+(bOn?"ON":"OFF"),
                              new MsgObj(QString::number(waveLength),OBISCTRL_MSGTYPE::SET_ONOFF));
    }
}
bool ObisCtrl::setPower(MasterThread *p, int power){
    p->addMsg("SOURce:POWer:LEVel:IMMediate:AMPLitude "+QString::number(0.001*power),
              new MsgObj("",OBISCTRL_MSGTYPE::SET_POWER));

    return true;
}
void ObisCtrl::setPower(int wavelength, int power){
    if(m_pSerialPorts.contains(wavelength)){
        if(!isLaserBox()){
            MasterThread* p=m_pSerialPorts[wavelength];setPower(p,power);
        }else{
            int id=m_laserPorts[wavelength];
            m_pSerialPort->addMsg("SOURce"+QString::number(id)+":POWer:LEVel:IMMediate:AMPLitude "+QString::number(0.001*power),
                                  new MsgObj(QString::number(wavelength),OBISCTRL_MSGTYPE::SET_POWER));
        }
    }
}
void ObisCtrl::setOnOff(int wavelength, bool bOn){
    qDebug()<<m_pSerialPorts<<wavelength<<bOn;
    setPowerOnOff(wavelength,-1,bOn);
}

void ObisCtrl::handleReadyRead(QString dataRaw,void *obj){
    MasterThread* pThread=(MasterThread*)sender();

    MsgObj* p=(MsgObj*)obj;qDebug()<<dataRaw<<p->type;
    switch(p->type){
    case GET_WAVELENGTH:{
        QString data=QString(dataRaw).replace("OK","");
        bool bOK;int w=data.toInt(&bOK);
        if(bOK&&w>100){
            m_pSerialPorts.insert(w,pThread);qDebug()<<w<<"connected";
        }
    }break;
    case GET_MAXPOWER:{
        QString data=QString(dataRaw).replace("OK","");
        bool bOK;float maxPower=data.toFloat(&bOK);
        if(bOK&&maxPower>0){
            int w;
            if(!isLaserBox()){w=portWavelen(pThread);}
            else{
                w=p->msg.toInt();
            }

            int nMaxPower=floor(1000*maxPower);
            qDebug()<<"max power"<<w<<maxPower<<nMaxPower;
            emit connected(w);
        }
    }break;
    }

    delete obj;
}

int ObisCtrl::portWavelen(MasterThread *p){
    QMapIterator<int,MasterThread*> iter(m_pSerialPorts);
    while(iter.hasNext()){
        iter.next();

        MasterThread *port=iter.value();
        if(p==port){return iter.key();}
    }
    return -1;
}

void ObisCtrl::connectStatus(bool bConnected){
    MasterThread *p=(MasterThread*)sender();
    if(bConnected){
        connect(p,SIGNAL(response(QString,void*)),this,SLOT(handleReadyRead(QString,void*)));

        if(!isLaserBox()){
            p->addMsg("SYSTem:INFormation:WAVelength?",new MsgObj("",GET_WAVELENGTH));
            p->addMsg("SOURce:POWer:LIMit:HIGH?",new MsgObj("",GET_MAXPOWER));
            p->addMsg("SOURce:AM:EXTernal DIGital",new MsgObj("",NONE));
        }else{
            //to-check
            m_pSerialPort=p;
            QMapIterator<int,int> iter(m_laserPorts);
            while(iter.hasNext()){
                iter.next();int w=iter.key(),id=iter.value();m_pSerialPorts[w]=nullptr;
                p->addMsg("SOURce"+QString::number(id)+":POWer:LIMit:HIGH?",new MsgObj(QString::number(w),GET_MAXPOWER));
                p->addMsg("SOURce"+QString::number(id)+":AM:EXTernal DIGital",new MsgObj("",NONE));
            }
        }

        setPowerOnOff(p,1,false);
    }else{
        delete p;
    }
}

