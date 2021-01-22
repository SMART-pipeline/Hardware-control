#ifndef KINESISSTAGE_H
#define KINESISSTAGE_H

#include <QString>
#include <QMap>
#include <QPointF>
#include <QDebug>

struct StageInfo;

static const char* labelX="stageX";
static const char* labelY="stageY";
static const char* labelZ="stageZ";


class KinesisStage
{
public:
    enum devType{KINESIS_LTS,KINESIS_BMC,ZABER_VSR};
private:
    QMap<const char*,StageInfo*> m_devInfos;
public:
    KinesisStage();
    ~KinesisStage();

    bool open(const char *label,const QString &serialNoStr,devType dt);
    bool moveAbs(const char *label,float pos);
    bool setVelocity(float vel);
    bool isRunning();bool isRunning(const char *label);
};

struct StageInfo{
    char pStr[9];void *obj;
    KinesisStage::devType type;
    QPointF rangeReal;QPoint rangeDev;float velUnit,accUnit;

    StageInfo(const QString &serialNoStr,KinesisStage::devType t,const QPoint &pDev){
        type=t;pStr[8]='\0';obj=NULL;
        memcpy(pStr,serialNoStr.toStdString().c_str(),8);

        rangeDev=pDev;
        if(t==KinesisStage::KINESIS_LTS){
            rangeReal=QPointF(0,150);rangeDev=QPoint(0,61440000);
            velUnit=21990233;accUnit=2506;
        }else if(t==KinesisStage::KINESIS_BMC){
            rangeReal=QPointF(0,100);rangeDev=QPoint(0,200000);
            velUnit=13422;accUnit=1.36;
        }else if(t==KinesisStage::ZABER_VSR){
            rangeReal=QPointF(0,17.5);rangeDev=QPoint(0,183723);
            velUnit=17201;accUnit=1.7;
        }else{
            qWarning()<<"unknown devices"<<serialNoStr;
        }
    }

    bool getPos(int pDev,float &pReal){
        pReal=(float(pDev-rangeDev.x()))*(rangeReal.y()-rangeReal.x())/float(rangeDev.y()-rangeDev.x())
                +rangeReal.x();
        return true;
    }

    bool getDevPos(float pReal,int &pDev){
        pDev=int((pReal-rangeReal.x())*float(rangeDev.y()-rangeDev.x())/(rangeReal.y()-rangeReal.x()))
                +rangeDev.x();
        return true;
    }
    int getDevPos(float pReal){
        int pDev=int((pReal-rangeReal.x())*float(rangeDev.y()-rangeDev.x())/(rangeReal.y()-rangeReal.x()))
                +rangeDev.x();
        return pDev;
    }

    int getDevVel(float vel){return vel*velUnit;}
    int getDevAcc(float acc){return acc*accUnit;}
};

#endif // KINESISSTAGE_H
