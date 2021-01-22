/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "masterthread.h"

#include <QSerialPort>
#include <QTime>
#include <QDebug>

QT_USE_NAMESPACE

MasterThread::MasterThread(const QString &portName, int waitTimeout,
                           const QString &lineEnd,int baudrate)
    : waitTimeout(0), quit(false)
{
    this->portName = portName;
    this->waitTimeout = waitTimeout;
    m_lineEnd=lineEnd;
    m_baudrate=baudrate;

    start();
}

//! [0]
MasterThread::~MasterThread()
{
    close();
}
//! [0]

void MasterThread::close(){
    mutex.lock();
    quit = true;
    cond.wakeOne();
    mutex.unlock();
    wait();
}

void MasterThread::addMsg(const QString &request, void *obj){
    m_msgQueue.append(request+m_lineEnd);m_objs.append(obj);

    mutex.lock();
    cond.wakeOne();
    mutex.unlock();
}

//! [4]
void MasterThread::run()
{
    QSerialPort serial;

    //! [5] //! [6]
    while (!quit) {
        if(!serial.isOpen()){
            serial.setPortName(portName);
            if(m_baudrate>0){serial.setBaudRate(m_baudrate);}
            bool bConnected=serial.open(QIODevice::ReadWrite);
            emit connectStatus(bConnected);
        }

        //! [7] //! [8]
        // write request
        while(serial.isOpen()){
            if(m_msgQueue.empty()){break;}
            QString msg=m_msgQueue.front();void *obj=m_objs.front();
            m_msgQueue.pop_front();m_objs.pop_front();

            serial.write(msg.toLatin1());//qDebug()<<msg.toLower()<<waitTimeout;
            if(!serial.waitForBytesWritten(waitTimeout)||!serial.waitForReadyRead(waitTimeout)){
                qWarning()<<"failed to send data";continue;
            }
            QString responseData;
            while(true){
                 QString s=serial.readAll();
                 responseData+=s;//qDebug()<<responseData<<s;
                 if(s.endsWith(m_lineEnd)){break;}
                 serial.waitForReadyRead(10);
            }

            emit response(responseData.replace(m_lineEnd,""),obj);
        }

        //qDebug()<<"master thread wait";

        //! [9]  //! [13]
        mutex.lock();
        cond.wait(&mutex);
        mutex.unlock();
    }
    //! [13]
    //!

    qDebug()<<"master thread exited";
}
