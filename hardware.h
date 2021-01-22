#ifndef HARDWARE_H
#define HARDWARE_H

#include "kinesisstage.h"
#include "niworker.h"
#include "dcam_common.h"
#include "obisctrl.h"

#include <QObject>

struct ImagingInfo{
    int imageHeight,waveLength;
    double amp,offset,pos,velocity,exposure;QString outputPath;
};

class Hardware : public QObject
{
    Q_OBJECT

    NIWorker m_niWorker;

    HDCAM m_hdcam;
    HDCAMWAIT m_hwait;
    DCAMWAIT_OPEN m_waitopen;
    DCAMWAIT_START m_waitstart;
    DCAMBUF_FRAME m_bufframe;

    const int m_bufferCapacity;

    KinesisStage s_stage;
    ObisCtrl m_obisCtrl;

    bool setSubArrayMode();
    bool prepareCapture();void finishCapture();
    bool setImageHeight(int);
public:
    explicit Hardware();

    bool connectDevices();
    bool imaging(const ImagingInfo &info);
signals:

public slots:
};

#endif // HARDWARE_H
