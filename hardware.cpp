#include "hardware.h"
#include "imagewriter.h"

#include <opencv2/opencv.hpp>
#include <thread>

Hardware::Hardware():m_bufferCapacity(50){}

bool Hardware::connectDevices(){
    m_hdcam=dcamcon_init_open();

    //CMOS camera (Flash 4.0 v3, Hamamatsu)
    if(failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_TRIGGERSOURCE,DCAMPROP_TRIGGERSOURCE__EXTERNAL))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_TRIGGERACTIVE,DCAMPROP_TRIGGERACTIVE__SYNCREADOUT))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_TRIGGERPOLARITY,DCAMPROP_TRIGGERPOLARITY__POSITIVE))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_OUTPUTTRIGGER_KIND,DCAMPROP_OUTPUTTRIGGER_KIND__EXPOSURE))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_TIMESTAMP_PRODUCER,DCAMPROP_TIMESTAMP_PRODUCER__DCAMMODULE))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_READOUTSPEED,2))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_SENSORMODE,DCAMPROP_SENSORMODE__SPLITVIEW))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_VIEW_(1, DCAM_IDPROP_READOUT_DIRECTION),DCAMPROP_READOUT_DIRECTION__FORWARD))
            ||failed(dcamprop_setvalue(m_hdcam,DCAM_IDPROP_VIEW_(2, DCAM_IDPROP_READOUT_DIRECTION),DCAMPROP_READOUT_DIRECTION__BACKWARD))){
        qWarning()<<"Unable to connect to camera";return false;
    }

    //x: linear stage (DDSM100, Thorlabs), y: stepper stage (LTS150, Thorlabs) , z: stepper stage (MCZ20, Zaber)
    if(!s_stage.open("","",KinesisStage::KINESIS_BMC)||!s_stage.open("","",KinesisStage::KINESIS_LTS)
            ||!s_stage.open(labelZ,"COM5",KinesisStage::ZABER_VSR)){qWarning()<<"Unable to connect to stages";return false;}

    //DAQ board (NI PCIe-6374, National Instruments), galvo scanner (GVS011, Thorlabs), lasers (Coherent)
    m_obisCtrl.initAll();

    connect(&m_obisCtrl,&ObisCtrl::connected,this,[this](int w){
        if(w!=488){return;}

        // An example of imaging one column
        ImagingInfo info;
        info.imageHeight=1024;info.offset=0;info.amp=2;info.pos=60;
        info.velocity=1;info.waveLength=488;info.exposure=10;
        info.outputPath="D:/demo.ome.tif";

        if(!imaging(info)){qWarning()<<"Imaging failed";}
    },Qt::QueuedConnection);

    return true;
}

//copied from dcam-sdk sample splitview.cpp
inline bool set_subarray_splitview( HDCAM hdcam, int32 width, int32 height,
                                    int32 v1_hoffset, int32 v2_hoffset, int32 v1_voffset, int32 v2_voffset ){
    DCAMERR err;

    err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__OFF );
    if( failed(err) )
    {
        dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYMODE, VALUE=OFF" );
        return false;
    }

    err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYHSIZE, width );
    if( failed(err) )
    {
        dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYHSIZE, VALUE=%d", width );
        return false;
    }

    if( v1_hoffset = v2_hoffset )
    {
        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYHPOS, v1_hoffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYHPOS, VALUE=%d", v1_hoffset );
            return false;
        }
    }
    else
    {
        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_VIEW_(1, DCAM_IDPROP_SUBARRAYHPOS), v1_hoffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYHPOS_VIEW1, VALUE=%d", v1_hoffset );
            return false;
        }

        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_VIEW_(2, DCAM_IDPROP_SUBARRAYHPOS), v2_hoffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYHPOS_VIEW2, VALUE=%d", v2_hoffset );
            return false;
        }
    }

    err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYVSIZE, height );
    if( failed(err) )
    {
        dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYVSIZE, VALUE=%d",height );
        return false;
    }

    if( v1_voffset == v2_voffset )
    {
        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYVPOS, v1_voffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYVPOS, VALUE=%d", v1_voffset );
            return false;
        }
    }
    else
    {
        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_VIEW_(1, DCAM_IDPROP_SUBARRAYVPOS), v1_voffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYVPOS_VIEW1, VALUE=%d", v1_voffset );
            return false;
        }

        err = dcamprop_setvalue( hdcam, DCAM_IDPROP_VIEW_(2, DCAM_IDPROP_SUBARRAYVPOS), v2_voffset );
        if( failed(err) )
        {
            dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYVPOS_VIEW2, VALUE=%d", v2_voffset );
            return false;
        }
    }

    err = dcamprop_setvalue( hdcam, DCAM_IDPROP_SUBARRAYMODE, DCAMPROP_MODE__ON );
    if( failed(err) )
    {
        dcamcon_show_dcamerr( hdcam, err, "dcamprop_setvalue()", "IDPROP=SUBARRAYMODE, VALUE=ON" );
        return false;
    }

    return true;
}

bool Hardware::setImageHeight(int imageHeight){
    finishCapture();
    bool bOK;int h=imageHeight/2;bOK=set_subarray_splitview(m_hdcam,2048,h,0,0,1024-h,0);
    prepareCapture();
    return bOK;
}

bool Hardware::setSubArrayMode(){
    double w,h;
    dcamprop_getvalue(m_hdcam,DCAM_IDPROP_SUBARRAYHSIZE,&w);
    dcamprop_getvalue(m_hdcam,DCAM_IDPROP_SUBARRAYVSIZE,&h);
    qDebug()<<"setSubArrayMode"<<w<<h;

    double v5=DCAMPROP_MODE__ON;
    if(w==2048&&h==2048){v5=DCAMPROP_MODE__OFF;}

    return !failed(dcamprop_setgetvalue(m_hdcam,DCAM_IDPROP_SUBARRAYMODE,&v5,0));
}

bool Hardware::prepareCapture(){
    memset(&m_waitopen,0,sizeof(m_waitopen));
    m_waitopen.size=sizeof(m_waitopen);
    m_waitopen.hdcam=m_hdcam;

    if(failed(dcamwait_open(&m_waitopen))){return false;}
    m_hwait=m_waitopen.hwait;
    if(failed(dcambuf_alloc(m_hdcam,m_bufferCapacity))){return false;}

    memset(&m_waitstart,0,sizeof(m_waitstart));
    m_waitstart.size=sizeof(m_waitstart);
    m_waitstart.eventmask=DCAMWAIT_CAPEVENT_FRAMEREADY;
    m_waitstart.timeout=1000;

    memset(&m_bufframe,0,sizeof(m_bufframe));
    m_bufframe.size=sizeof(m_bufframe);
    m_bufframe.iFrame=-1;

    return true;
}
void Hardware::finishCapture(){dcambuf_release(m_hdcam);}

bool Hardware::imaging(const ImagingInfo &info){
    s_stage.setVelocity(info.velocity);
    m_niWorker.setInterval(info.exposure);

    static QMap<int,int> map;if(map.empty()){map[405]=7;map[488]=6;map[552]=5;map[647]=4;}
    int triggerPort=map.value(info.waveLength,-1);if(triggerPort<0){return false;}
    m_niWorker.init(info.amp,info.offset,triggerPort);

    m_obisCtrl.setOnOff(info.waveLength,true);

    setImageHeight(info.imageHeight);
    dcamcap_start(m_hdcam,DCAMCAP_START_SEQUENCE);

    s_stage.moveAbs(labelX,info.pos);qDebug()<<"Moving to"<<info.pos;

    DCAMCAP_TRANSFERINFO transInfo;QList<int32> bufferIds;
    int32 lastFrameCount=0;cv::Size imageSize;QList<void*> buffers;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while(s_stage.isRunning(labelX)){
        if(bufferIds.empty()){
            DCAMERR err=dcamwait_start(m_hwait,&m_waitstart);
            if(failed(err)){if(err==DCAMERR_ABORT){break;}continue;}

            memset(&transInfo,0,sizeof(transInfo));transInfo.size=sizeof(transInfo);

            dcamcap_transferinfo(m_hdcam,&transInfo);
            int32 frameCount=transInfo.nFrameCount-lastFrameCount,frameIndex=transInfo.nNewestFrameIndex;
            lastFrameCount=transInfo.nFrameCount;if(frameCount>1){if(frameCount>m_bufferCapacity){frameCount=m_bufferCapacity;}}

            for(int i=0;i<frameCount;i++){
                int32 id=(frameIndex-i+m_bufferCapacity)%m_bufferCapacity;
                bufferIds.prepend(id);
            }
        }

        m_bufframe.iFrame=bufferIds.first();bufferIds.pop_front();
        if(failed(dcambuf_lockframe(m_hdcam,&m_bufframe))){
            qWarning()<<"failed to lock frame"<<m_bufframe.iFrame;
            continue;
        }

        if(imageSize.empty()){imageSize=cv::Size(m_bufframe.width,m_bufframe.height);}
        void *buffer=malloc(imageSize.area()*2);memcpy(buffer,m_bufframe.buf,imageSize.area()*2);
        buffers.append(buffer);std::cout<<m_bufframe.framestamp<<"\r\b";
    }

    dcamcap_stop(m_hdcam);
    m_obisCtrl.setOnOff(info.waveLength,false);
    dcamwait_abort(m_hwait);
    m_niWorker.stop();

    flsmio::ImageWriter writer(info.outputPath);qDebug()<<buffers.length()<<"images";int count=1;
    foreach(void *buffer,buffers){writer.addImage(cv::Mat(imageSize,CV_16UC1,buffer));std::cout<<"Saving image "<<count<<"\r\b";count++;}
    return true;
}
