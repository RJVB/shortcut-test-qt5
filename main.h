/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef MAIN_H
#define MAIN_H

#include <unistd.h>
#include <signal.h>

#include <QApplication>

#undef USE_QSOCKETNOTIFIER

#ifdef USE_QSOCKETNOTIFIER
#include <QSocketNotifier>
#else
#ifdef Q_OS_MACOS
#include <mach/mach.h>
typedef semaphore_t sem_t;
#else
#include <semaphore.h>
#endif
#include <QFuture>
#endif

class QQApplication : public QApplication
{
    Q_OBJECT

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
   typedef __sighandler_t InterruptSignalHandler;
#else
   typedef void (*InterruptSignalHandler)(int);
#endif

public:
    explicit QQApplication(int &argc, char **argv);
    ~QQApplication();
#ifndef USE_QSOCKETNOTIFIER
    InterruptSignalHandler catchInterruptSignal(int sig);
#endif

signals:
   void interruptSignalReceived(int sig);

public slots:
    void handleHUP(int sckt);

private:
    static void signalhandler(int sig);

#ifdef USE_QSOCKETNOTIFIER
    int sigHUPPipeRead = -1, sigHUPPipeWrite = -1;
    QSocketNotifier *sigHUPNotifier = nullptr;
#else
    sem_t m_sem;
    bool m_monitorSignals;
    QFuture<void> m_monitorHandle;
#endif
    int m_signalReceived;
    static QQApplication *theApp;
};


#endif
