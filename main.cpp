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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>

#include <QDebug>

#include "main.h"
#include "mainwindow.h"

QQApplication *QQApplication::theApp = nullptr;

QQApplication::QQApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
//         A "proper" exit-on-sigHUP approach:
//         Open a pipe or an eventfd, then install your signal handler. In that signal 
//         handler, write anything to the writing end or write uint64_t(1) the eventfd. 
//         Create a QSocketNotifier on the reading end of the pipe or on the eventfd, 
//         connect its activation signal to a slot that does what you want.

    if (!theApp) {
        int pp[2];
        if (pipe(pp)) {
            qErrnoWarning("Error opening SIGHUP handler pipe");
        } else {
            theApp = this;
#ifdef USE_QSOCKETNOTIFIER
            sigHUPPipeRead = pp[0];
            sigHUPPipeWrite = pp[1];
            sigHUPNotifier = new QSocketNotifier(sigHUPPipeRead, QSocketNotifier::Read);
            connect(sigHUPNotifier, &QSocketNotifier::activated, this, &QQApplication::handleHUP);
            qWarning() << Q_FUNC_INFO << sigHUPNotifier << "calls handleHUP via pipe" << sigHUPPipeRead;
#else
            connect(this, &QQApplication::signalReceived, this, &QQApplication::handleHUP, Qt::QueuedConnection);
            qWarning() << Q_FUNC_INFO << "handleHUP connected to signal signalReceived" << &QQApplication::signalReceived;
#endif
            signal(SIGHUP, &signalhandler);
            signal(SIGINT, &signalhandler);
            signal(SIGTERM, &signalhandler);
        }
    }
}

QQApplication::~QQApplication()
{
#ifdef USE_QSOCKETNOTIFIER
    if (sigHUPPipeRead != -1) {
        close(sigHUPPipeRead);
    }
    if (sigHUPPipeWrite != -1) {
        close(sigHUPPipeWrite);
    }
    delete sigHUPNotifier;
#endif
}

void QQApplication::signalhandler(int sig)
{
#ifdef USE_QSOCKETNOTIFIER
    if (theApp->sigHUPPipeWrite != -1) {
        qCritical() << Q_FUNC_INFO << "signal" << sig << "received";
        theApp->m_signalReceived = sig;
        write(theApp->sigHUPPipeWrite, &sig, sizeof(sig));
        qCritical() << Q_FUNC_INFO << "trigger sent.";
    }
#else
   emit theApp->signalReceived(sig);
#endif
}

void QQApplication::handleHUP(int sckt)
{
#ifdef USE_QSOCKETNOTIFIER
    qCritical() << Q_FUNC_INFO << "called for pipe" << sckt;
    qWarning() << "\tsignal" << m_signalReceived;
#else
    qCritical() << Q_FUNC_INFO << "called for signal" << sckt;
#endif
    sleep(3);
    qCritical() << Q_FUNC_INFO << "done.";
#ifdef USE_QSOCKETNOTIFIER
    close(sigHUPPipeRead);
    close(sigHUPPipeWrite);
    sigHUPPipeRead = sigHUPPipeWrite = -1;
//     _exit(0);
    // re-raise signal with default handler and trigger program termination
    signal(m_signalReceived, SIG_DFL);
    raise(m_signalReceived);
#else
    // re-raise signal with default handler and trigger program termination
    signal(sckt, SIG_DFL);
    raise(sckt);
#endif
}

int main(int argc, char *argv[])
{
    bool nativeMenuBar = true;
    int shortCutActFlags = 3;
    QString shortCut = "Ctrl+<";

    QCommandLineParser commandLineParser;
    const QCommandLineOption noNativeMenuOption(QStringLiteral("no-native-menubar"),
                                                QStringLiteral("do not use a native menubar"));
    const QCommandLineOption r2LOption(QStringLiteral("right-to-left"),
                                                QStringLiteral("activate the right-to-left layout mode"));
    const QCommandLineOption shortCutTestNoMenu(QStringLiteral("no-shortcut-test-in-menubar"),
                                                QStringLiteral("do not add the shortcut test action to the menubar"));
    const QCommandLineOption shortCutTestNoContext(QStringLiteral("no-shortcut-test-in-contextmenu"),
                                                   QStringLiteral("do not add the shortcut test action to the context menu"));
    const QCommandLineOption shortCutOption(QStringLiteral("shortcut"),
                                                QStringLiteral("the shortcut to test (read Ctrl for Command on Mac)"),
                                                "shortcut", shortCut);
    commandLineParser.addOption(noNativeMenuOption);
    commandLineParser.addOption(r2LOption);
    commandLineParser.addOption(shortCutTestNoMenu);
    commandLineParser.addOption(shortCutTestNoContext);
    commandLineParser.addOption(shortCutOption);
    commandLineParser.addHelpOption();

    QQApplication app(argc, argv);

    commandLineParser.process(app);
    if (commandLineParser.isSet(noNativeMenuOption)) {
        qWarning() << "Using non-native menubar";
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
        nativeMenuBar = false;
    } else {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
    }
    if (commandLineParser.isSet(shortCutTestNoMenu)) {
        shortCutActFlags &= ~1;
    }
    if (commandLineParser.isSet(shortCutTestNoContext)) {
        shortCutActFlags &= ~2;
    }
    if (commandLineParser.isSet(shortCutOption)) {
        shortCut = commandLineParser.value(shortCutOption);
    }
    if (commandLineParser.isSet(r2LOption)) {
        app.setLayoutDirection(Qt::RightToLeft);
    }

    qWarning() << "Shortcut test action flags:" << shortCutActFlags;

    MainWindow window(shortCutActFlags, shortCut, nativeMenuBar);
    window.show();
    return app.exec();
}
