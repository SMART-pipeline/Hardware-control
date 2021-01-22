#include "kinesisstage.h"
#include "zaberstagectrl.h"

#include <Thorlabs.MotionControl.IntegratedStepperMotors.h>
#include <Thorlabs.MotionControl.KCube.BrushlessMotor.h>

#include <QDebug>

static const float s_accValue=20;

KinesisStage::KinesisStage()
{

}

KinesisStage::~KinesisStage(){

}

bool KinesisStage::open(const char *label, const QString &serialNoStr, devType dt){
    if(dt==ZABER_VSR){
        StageInfo* pStageInfo=new StageInfo(serialNoStr,ZABER_VSR,QPoint(0,1));

        ZaberStageCtrl *pCtrl=new ZaberStageCtrl(serialNoStr);
        pStageInfo->obj=pCtrl;
        pCtrl->setAcc(pStageInfo->getDevAcc(5));
        pCtrl->setVel(pStageInfo->getDevVel(1));

        m_devInfos.insert(labelZ,pStageInfo);return true;
    }

    if(!m_devInfos.empty()){return true;}

    char *strStageX=nullptr,*strStageY=nullptr;

    if (TLI_BuildDeviceList() == 0){
        // get device list size
        short n = TLI_GetDeviceListSize();
        // get KBD serial numbers
        char serialNos[100];
        TLI_GetDeviceListExt(serialNos, 100);//, 28);
        // output list of matching devices
        //puts(serialNos);
        char *p = strtok(serialNos, ",");
        while(p != NULL){
            TLI_DeviceInfo deviceInfo;
            // get device info from device
            TLI_GetDeviceInfo(p, &deviceInfo);
            // get strings from device info structure
            char desc[65];
            strncpy(desc, deviceInfo.description, 64);
            desc[64] = '\0';
            char serialNo[9];
            strncpy(serialNo, deviceInfo.serialNo, 8);
            serialNo[8] = '\0';
            // output
            printf("Found Device %s=%s : %s, type id=%d\r\n", p, serialNo, desc, deviceInfo.typeID);

            switch(deviceInfo.typeID){
            case 28:{
                strStageX=p;
            }break;
            case 45:{
                strStageY=p;
            }break;
            }

            p = strtok(NULL, ",");
        }
    }

    if(nullptr==strStageX||nullptr==strStageY){return false;}

    QPoint pBorder;bool bNeedHoming=false;double minPos=-1,maxPos=-1;

    if(nullptr!=strStageY){
        const char *pStr=strStageY;StageInfo* pStageInfo=new StageInfo(QString::fromStdString(std::string(pStr)),KINESIS_LTS,pBorder);

        if(0!=ISC_Open(pStr)||!ISC_StartPolling(pStr,10)){
            qWarning()<<"failed to open x stage"<<pStr;
            return false;
        }

        minPos=ISC_GetStageAxisMinPos(pStr);
        maxPos=ISC_GetStageAxisMaxPos(pStr);
        pBorder.setX(minPos);pBorder.setY(maxPos);

        bNeedHoming=!ISC_CanMoveWithoutHomingFirst(pStr);

        ISC_SetJogMode(pStr,MOT_Continuous,MOT_Profiled);
        ISC_SetJogVelParams(pStr,pStageInfo->getDevAcc(s_accValue),pStageInfo->getDevVel(2));

        m_devInfos.insert(labelY,pStageInfo);
    }

    if(nullptr!=strStageX){
        const char *pStr=strStageX;StageInfo* pStageInfo=new StageInfo(QString::fromStdString(std::string(pStr)),KINESIS_BMC,pBorder);

        if(0!=KCube_Brushless_Motor_Space::BMC_Open(pStr)
                //||0!=KCube_Brushless_Motor_Space::BMC_EnableChannel(pStr)
                ||0!=KCube_Brushless_Motor_Space::BMC_ClearMessageQueue(pStr)
                ||!KCube_Brushless_Motor_Space::BMC_StartPolling(pStr,10)
                //||0!=KCube_Brushless_Motor_Space::BMC_RequestSettings(pStr)
                ||0!=KCube_Brushless_Motor_Space::BMC_RequestStageAxisParams(pStr)
                ||0!=KCube_Brushless_Motor_Space::BMC_SetTriggerConfigParams(pStr,KCube_Brushless_Motor_Space::KMOT_TrigOut_AtMaxVelocity,KCube_Brushless_Motor_Space::KMOT_TrigPolarityHigh,
                                                                             KCube_Brushless_Motor_Space::KMOT_TrigDisabled,KCube_Brushless_Motor_Space::KMOT_TrigPolarityHigh)
                ){
            return false;
        }

        Sleep(100);

        minPos=KCube_Brushless_Motor_Space::BMC_GetStageAxisMinPos(pStr);
        maxPos=KCube_Brushless_Motor_Space::BMC_GetStageAxisMaxPos(pStr);
        qDebug()<<minPos<<maxPos<<pStr;
        pBorder.setX(minPos);pBorder.setY(maxPos);

        KCube_Brushless_Motor_Space::BMC_OverrideHomeRequirement(pStr);
        bNeedHoming=!KCube_Brushless_Motor_Space::BMC_CanMoveWithoutHomingFirst(pStr);

        KCube_Brushless_Motor_Space::BMC_SetJogMode(pStr,KCube_Brushless_Motor_Space::MOT_Continuous,
                                                    KCube_Brushless_Motor_Space::MOT_Profiled);
        KCube_Brushless_Motor_Space::BMC_SetJogVelParams(pStr,pStageInfo->getDevAcc(s_accValue),pStageInfo->getDevVel(2));

        m_devInfos.insert(labelX,pStageInfo);
    }
}

bool KinesisStage::moveAbs(const char *label, float pos){
    if(!m_devInfos.contains(label)){return false;}
    StageInfo *p=m_devInfos[label];

    int devPos;p->getDevPos(pos,devPos);

    if(p->type==KINESIS_LTS){
        return 0==ISC_MoveToPosition(p->pStr,devPos);
    }else if (p->type==KINESIS_BMC){
        return 0==KCube_Brushless_Motor_Space::BMC_MoveToPosition(p->pStr,devPos);
    }else if(p->type==ZABER_VSR){
        ((ZaberStageCtrl*)p->obj)->moveAbs(devPos);return true;
    }

    return false;
}

bool KinesisStage::setVelocity(float vel){
    QMapIterator<const char*,StageInfo*> iter(m_devInfos);
    bool bOK=true;int velDev,accDev,velDevOld,accDevOld;

    qDebug()<<"set velocity to"<<vel;

    while(iter.hasNext()){
        iter.next();
        StageInfo *p=iter.value();
        velDev=vel*p->velUnit;qDebug()<<p->rangeReal<<velDev;
        accDev=p->getDevAcc(s_accValue);
        if(p->type==KINESIS_LTS){
            bOK&=((0==ISC_GetVelParams(p->pStr,&accDevOld,&velDevOld))
                    &&(0==ISC_SetVelParams(p->pStr,accDev,velDev)));
        }else if(p->type==KINESIS_BMC){
            bOK&=((0==KCube_Brushless_Motor_Space::BMC_GetVelParams(p->pStr,&accDevOld,&velDevOld))
                    &&(0==KCube_Brushless_Motor_Space::BMC_SetVelParams(p->pStr,accDev,velDev)));
        }
    }
    return bOK;
}
bool KinesisStage::isRunning(){
    QMapIterator<const char*,StageInfo*> iter(m_devInfos);
    bool bRunning=false;while(iter.hasNext()){iter.next();bRunning|=isRunning(iter.key());}
    return bRunning;
}
bool KinesisStage::isRunning(const char *label){
    if(!m_devInfos.contains(label)){return false;}
    StageInfo *p=m_devInfos[label];
    DWORD lStatusBits=0;

    if(p->type==KINESIS_LTS){
        lStatusBits=ISC_GetStatusBits(p->pStr);
    }else if (p->type==KINESIS_BMC){
        lStatusBits=KCube_Brushless_Motor_Space::BMC_GetStatusBits(p->pStr);
    }else if(p->type==ZABER_VSR){return false;}
    return 0!=((lStatusBits)&0x30);
}
