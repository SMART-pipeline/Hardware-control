#include "niworker.h"

#include <NIDAQmx.h>
#include <QDebug>
#include <atomic>

static const float64 SAMPLE_FACTOR=1.2;//1
static const uInt64 MAX_COUNT=900000*SAMPLE_FACTOR;

NIWorker::NIWorker():m_rate(2000000.0*SAMPLE_FACTOR){
    m_htaskCtr=NULL;m_htaskAO=NULL;m_htaskDO=NULL;
    m_sampleCount=20006*SAMPLE_FACTOR;m_bChanged=false;m_bUsetrigger=true;
}

NIWorker::~NIWorker(){
    stop();
}

void NIWorker::closeTask(TaskHandle &t){
    if(NULL!=t){
        TaskHandle _t=t;t=NULL;
        DAQmxStopTask(_t);DAQmxClearTask(_t);
    }
}
bool NIWorker::stopClock(){
    return !m_htaskCtr||!failed(DAQmxStopTask(m_htaskCtr));
}
bool NIWorker::startClock(){
    return m_htaskCtr&&!failed(DAQmxStartTask(m_htaskCtr));
}

bool NIWorker::failed(int32 e){
    bool bOK=!DAQmxFailed(e);
    if(!bOK){
        char errBuff[2048]={'\0'};
        DAQmxGetExtendedErrorInfo(errBuff,2048);
        qDebug()<<"DAQmx Error:"<<errBuff;
    }

    return !bOK;
}

bool NIWorker::init(double amplitude, double offset,int larserTriggerPort){
    m_bChanged=false;qDebug()<<"init NI"<<amplitude<<offset;

    if(failed(DAQmxCreateTask("",&m_htaskCtr))||failed(DAQmxCreateTask("",&m_htaskAO)
                          ||failed(DAQmxCreateTask("",&m_htaskDO)))){
        qWarning()<<"failed to create task";return false;
    }

    uInt64 i=0;int32 written;//const uInt64 sampleCount=2006;
    uInt64 sampleCount=m_sampleCount;

    float64 riseCount=sampleCount/2;
    static float64 *dataAO=(float64*)malloc(MAX_COUNT*2);//float64 dataAO[MAX_COUNT];

    for(i=0;i<sampleCount;i++){
        float64 v=0;
        if(i<riseCount){
            v=(i+0.0)*2/riseCount-1;
        }else{
            v=-1*(i+0.0-riseCount)*2/(sampleCount-1-riseCount)+1;
        }
        v*=amplitude;v+=offset;
        dataAO[i]=v;
    }
    if(failed(DAQmxCreateAOVoltageChan(m_htaskAO,"Dev1/ao1","",-10.0,10.0,DAQmx_Val_Volts,NULL))
            ||failed(DAQmxCfgSampClkTiming(m_htaskAO,"/Dev1/PFI12",m_rate,DAQmx_Val_Rising,
                                           DAQmx_Val_ContSamps,sampleCount))
            //||failed(DAQmxCfgDigEdgeStartTrig(m_htaskAO,"/Dev1/PFI12",DAQmx_Val_Rising))
            ||failed(DAQmxWriteAnalogF64(m_htaskAO,sampleCount,0,-1,DAQmx_Val_GroupByChannel,
                                         dataAO,&written,NULL))
            ||failed(DAQmxStartTask(m_htaskAO))){
        qWarning()<<"failed to create AO channel";return false;
    }

    //const uInt64 sampleDOCount=sampleCount*5;//const uInt64 delayCount=8;
    uInt64 laserCount=riseCount,cameraCount=2000*SAMPLE_FACTOR;
    uInt64 laserStart=300*SAMPLE_FACTOR,cameraStart=100*SAMPLE_FACTOR;
    uInt64 sampleCountDO=sampleCount;
    static uInt8 *dataDO=(uInt8*)malloc(MAX_COUNT*5*2);//uInt8 dataDO[MAX_COUNT*5*2];

    const int port=larserTriggerPort-4;
    for(i=0;i<sampleCount;i++){
        for(int j=0;j<4;j++){
            uInt8 v=((i>laserStart&&i<laserCount)?1:0);
            if(port>=0&&port!=j){v=0;}

            dataDO[5*i+j]=v;
        }

        dataDO[5*i+4]=((i>cameraStart&&i<cameraCount)?1:0);
    }
    if(failed(DAQmxCreateDOChan(m_htaskDO,"Dev1/port0/line4:7,Dev1/port0/line0",
                                "",DAQmx_Val_ChanForAllLines))
            ||failed(DAQmxCfgSampClkTiming(m_htaskDO,"/Dev1/PFI12",m_rate,DAQmx_Val_Rising,
                                           DAQmx_Val_ContSamps,sampleCountDO))
            ||failed(DAQmxWriteDigitalLines(m_htaskDO,sampleCountDO,0,-1,DAQmx_Val_GroupByChannel,
                                            dataDO,&written,NULL))
            ||failed(DAQmxStartTask(m_htaskDO))){
        qWarning()<<"failed to create DO channel";return false;
    }

    static const float64 highTime=0.0000003/SAMPLE_FACTOR;
    static const float64 lowTime=0.0000002/SAMPLE_FACTOR;

    if(failed(DAQmxCreateCOPulseChanTime(m_htaskCtr,"Dev1/ctr0","",DAQmx_Val_Seconds,DAQmx_Val_Low,0,highTime,lowTime))
            ||failed(DAQmxCfgImplicitTiming(m_htaskCtr,DAQmx_Val_ContSamps,100000))
            ||failed(DAQmxConnectTerms("/Dev1/Ctr0InternalOutput","/Dev1/PFI12",DAQmx_Val_DoNotInvertPolarity))
            ||(m_bUsetrigger&&failed(DAQmxCfgDigEdgeStartTrig(m_htaskCtr,"/Dev1/PFI0",DAQmx_Val_Rising)))
            ||failed(DAQmxStartTask(m_htaskCtr))
            ){
        qWarning()<<"failed to create counter";return false;
    }

    return true;
}

void NIWorker::stop(){qDebug()<<"NI stop all";
    closeTask(m_htaskCtr);closeTask(m_htaskAO);closeTask(m_htaskDO);
}

void NIWorker::setSampleCount(uInt64 sampleCount){
    static const uInt64 MIN_COUNT=3000*SAMPLE_FACTOR;
    qDebug()<<sampleCount<<MAX_COUNT<<MIN_COUNT;
    if(sampleCount>MAX_COUNT){sampleCount=MAX_COUNT;}
    if(sampleCount<MIN_COUNT){sampleCount=MIN_COUNT;}

    m_sampleCount=sampleCount;
    m_bChanged=true;
}

void NIWorker::setInterval(float time){
    uInt64 t=time*20000.0*SAMPLE_FACTOR/10.0;
    setSampleCount(t);
}
bool NIWorker::changed(){return m_bChanged;}

void NIWorker::useTrigger(bool bUse){
    if(bUse==m_bUsetrigger){return;}
    qDebug()<<(bUse?"":"do not")<<"use ni trigger";
    m_bUsetrigger=bUse;m_bChanged=true;
}
