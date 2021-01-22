#ifndef NIWORKER_H
#define NIWORKER_H

typedef void*              TaskHandle;
typedef unsigned __int64   uInt64;
typedef double             float64;
typedef signed long        int32;

class NIWorker
{
    TaskHandle m_htaskCtr,m_htaskAO,m_htaskDO;
    uInt64 m_sampleCount;
    const float64 m_rate;

    bool m_bChanged,m_bUsetrigger,m_bShowSlash;

    void closeTask(TaskHandle &);bool failed(int32 e);
    void setSampleCount(uInt64 sampleCount);
public:
    NIWorker();~NIWorker();

    bool init(double amplitude, double offset, int larserTriggerPort);void stop();
    bool stopClock();bool startClock();
    void useTrigger(bool);

    void setInterval(float time);bool changed();
};

#endif // NIWORKER_H
