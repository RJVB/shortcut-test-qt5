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

#include <QApplication>
#include <QGuiApplication>
#include <QtWidgets>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QContextMenuEvent>
#include <QMessageBox>

#include <QLayout>

#include <QDebug>

#include "mainwindow.h"
#include "qwidgetstyleselector.h"

#ifdef Q_OS_MACOS
#include <Carbon/Carbon.h>
#endif

static bool isMenubarMenu(const QMenu *m, bool checkIsNative=true)
{   bool ret = false;
        static int level = -1;
    QSet<const QMenu*> checkList;
    level += 1;
    if (m && m->menuAction()) {
        QAction *mAct = m->menuAction();
        qWarning() << "##" << level << "isMenubarMenu(" << m << m->title() << ") menuAction=" << mAct << mAct->text();
        foreach (const QWidget *w, mAct->associatedWidgets()) {
            if (w == m) {
                goto done;
            }
            qWarning() << "###" << level << "associated widget" << w << w->windowTitle() << "parent=" << w->parentWidget();
            if (const QMenuBar *mb = qobject_cast<const QMenuBar*>(w)) {
                qWarning() << "#### widget is QMenuBar" << mb << mb->windowTitle() << "with parent" << mb->parentWidget();
                ret = checkIsNative? mb->isNativeMenuBar() : true;
                goto done;
            }
            else if (const QMenu *mm = qobject_cast<const QMenu*>(w)) {
                if (checkList.contains(mm)) {
                    continue;
                }
                checkList += mm;
                qWarning() << "####" << level << "widget is QMenu" << mm << mm->title() << "with parent" << mm->parentWidget();
                if (isMenubarMenu(mm, checkIsNative)) {
                    ret = true;
                    goto done;
                }
            }
        }
    }
done:;
    level -= 1;
    return ret;
}

//! [0]
MainWindow::MainWindow(int shortCutActFlags, QString shortCut, bool nativeMenuBar, QWidget *parent)
    : QMainWindow(parent)
    , m_nativeMenuBar(nativeMenuBar)
    , m_shortCutActFlags(shortCutActFlags)
    , m_shortCut(shortCut)
{
#ifdef Q_OS_MACOS
    if (!nativeMenuBar) {
        qWarning() << Q_FUNC_INFO << "menuBar" << menuBar() << "native=" << menuBar()->isNativeMenuBar();
        QMenuBar *mB = new QMenuBar(this);
        mB->setNativeMenuBar(false);
        mB->setVisible(true);
        qWarning() << "new menuBar:" << mB;
        setMenuBar(mB);
    }
    qWarning() << Q_FUNC_INFO << "menuBar" << menuBar() << "native=" << menuBar()->isNativeMenuBar();
    qWarning() << "\tplatformName=" << QGuiApplication::platformName();
    qWarning() << "\tQt::AA_MacDontSwapCtrlAndMeta=" << qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);;
#endif

#ifdef Q_OS_MACOS
    setWindowFlags(windowFlags() & ~Qt::WindowFullscreenButtonHint);
#endif

    QWidget *widget = new QWidget;
    setCentralWidget(widget);
//! [0]

//! [1]
    QWidget *topFiller = new QWidget;
    topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    infoLabel = new QLabel(tr("<i>Choose a menu option, or right-click to "
                              "invoke a context menu</i>"));
    infoLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    infoLabel->setAlignment(Qt::AlignCenter);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(5);
    layout->addWidget(topFiller);
    layout->addWidget(infoLabel);
    layout->addWidget(bottomFiller);
    widget->setLayout(layout);
//! [1]

//! [2]
    createActions();
    createMenus();

    QString message = tr("A context menu is available by right-clicking");
    statusBar()->showMessage(message);

    setWindowTitle(tr("Menus"));
    setMinimumSize(160, 160);
    resize(480, 320);
}
//! [2]

//! [3]
#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    qWarning() << Q_FUNC_INFO << event << "reason=" << event->reason();
    QQMenu *menu = new QQMenu(tr("Dynamic contextMenu"), this);
    menu->setTearOffEnabled(true);
    menu->addActions(contextMenu->actions());
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
    bool isMB = isMenubarMenu(menu);
    qWarning() << "\tcreated menu" << menu << "isNativeMenubarMenu=" << isMB;
    menu->exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void MainWindow::aboutToShowContextMenu()
{
#ifndef QT_NO_CONTEXTMENU
    QQMenu *menu = qobject_cast<QQMenu *>(sender());

    if (menu) {
        bool isMB = isMenubarMenu(menu);
        qWarning() << Q_FUNC_INFO << "About to show" << menu << "isNativeMenubarMenu=" << isMB;
        QAction *extraAct = new QAction(tr("&Quit"), this);
        extraAct->setStatusTip(tr("Exit the application"));
        connect(extraAct, &QAction::triggered, this, &QWidget::close);
        menu->addAction(extraAct);
    }
#endif
}

void MainWindow::aboutToShowMenu()
{
    QQMenu *menu = qobject_cast<QQMenu *>(sender());

    if (menu) {
        bool isMB = isMenubarMenu(menu);
        qWarning() << Q_FUNC_INFO << "About to show" << menu << "isNativeMenubarMenu=" << isMB;
    }
}

// do as KTextEditor does
void MainWindow::mousePressEvent(QMouseEvent *e)
{
    switch (e->button()) {
        case Qt::LeftButton:
            e->accept();
            qWarning() << Q_FUNC_INFO << "event" << e << "accepted";
            break;
        default:
            e->ignore();
            qWarning() << Q_FUNC_INFO << "event" << e << "ignored";
            break;
    }
}

//! [3]

void MainWindow::newFile()
{
    infoLabel->setText(tr("Invoked <b>File|New</b>"));
}

void MainWindow::newWindow()
{
    infoLabel->setText(tr("Invoked <b>File|New Window</b>"));
    auto w = new MainWindow(m_shortCutActFlags, m_shortCut, m_nativeMenuBar, this);
    w->show();
}

void MainWindow::open()
{
    infoLabel->setText(tr("Invoked <b>File|Open</b>"));
}

void MainWindow::save()
{
    infoLabel->setText(tr("Invoked <b>File|Save</b>"));
}

void MainWindow::print()
{
    infoLabel->setText(tr("Invoked <b>File|Print</b>"));
}

void MainWindow::undo()
{
    infoLabel->setText(tr("Invoked <b>Edit|Undo</b>"));
}

void MainWindow::redo()
{
    infoLabel->setText(tr("Invoked <b>Edit|Redo</b>"));
}

void MainWindow::cut()
{
    infoLabel->setText(tr("Invoked <b>Edit|Cut</b>"));
}

void MainWindow::copy()
{
    infoLabel->setText(tr("Invoked <b>Edit|Copy</b>"));
}

void MainWindow::paste()
{
    infoLabel->setText(tr("Invoked <b>Edit|Paste</b>"));
}

void MainWindow::selectAll()
{
    infoLabel->setText(tr("Invoked <b>Edit|Select All</b>"));
}

void MainWindow::bold()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Bold</b>"));
}

void MainWindow::italic()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Italic</b>"));
}

void MainWindow::leftAlign()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Left Align</b>"));
}

void MainWindow::rightAlign()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Right Align</b>"));
}

void MainWindow::justify()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Justify</b>"));
}

void MainWindow::center()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Center</b>"));
}

void MainWindow::toggleFullScreen()
{
    m_normalParent = parentWidget();
    if (fullScrAct->isChecked()) {
        infoLabel->setText(tr("Invoked <b>Edit|Format|Fullscreen</b> (entering)"));
        showFullScreen();
//         if (m_normalParent) {
//             m_normalFlags = m_normalParent->windowFlags();
//         } else {
//             m_normalFlags = windowFlags();
//         }
//         m_normalGeo = geometry();
//         // taken from qwidget_mac.mm, Qt 4.8.7
//         const QRect fullscreen(qApp->desktop()->screenGeometry(qApp->desktop()->screenNumber(this)));
//         setParent(m_normalParent, Qt::Window | Qt::FramelessWindowHint | (windowFlags() & 0xffff0000)); //save
//         setGeometry(fullscreen);
// #ifdef Q_OS_MACOS
//         if(!qApp->desktop()->screenNumber(this) && QGuiApplication::platformName() == QStringLiteral("cocoa")) {
//             SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
//         }
// #endif
    } else {
        infoLabel->setText(tr("Invoked <b>Edit|Format|Fullscreen</b> (exiting)"));
        showNormal();
//         if (m_normalParent && m_normalParent == parentWidget()) {
//             setParent(m_normalParent, m_normalFlags);
//         } else {
//             setParent(parentWidget(), m_normalFlags);
//         }
//         setGeometry(m_normalGeo);
// #ifdef Q_OS_MACOS
//         if(!qApp->desktop()->screenNumber(this) && QGuiApplication::platformName() == QStringLiteral("cocoa")) {
//             SetSystemUIMode(kUIModeNormal, 0);
//         }
// #endif
    }
//     show();
}

void MainWindow::setLineSpacing()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Set Line Spacing</b>"));
}

void MainWindow::setParagraphSpacing()
{
    infoLabel->setText(tr("Invoked <b>Edit|Format|Set Paragraph Spacing</b>"));
}

void MainWindow::about()
{
    infoLabel->setText(tr("Invoked <b>Help|About</b>"));
    QMessageBox::about(this, tr("About Menu"),
            tr("The <b>Menu</b> example shows how to create "
               "menu-bar menus and context menus."));
}

void MainWindow::aboutQt()
{
    infoLabel->setText(tr("Invoked <b>Help|About Qt</b>"));
}

void MainWindow::shortCutActHandler()
{
    infoLabel->setText(tr("Invoked <b>shortcut test action</b>"));
    qWarning() << Q_FUNC_INFO << "shortCutAct->shortcut=" << shortCutAct->shortcut();
}

//! [4]
void MainWindow::createActions()
{
//! [5]
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    newWindowAct = new QAction(tr("New Window"), this);
    newWindowAct->setStatusTip(tr("Create a new window"));
    connect(newWindowAct, &QAction::triggered, this, &MainWindow::newWindow);
//! [4]

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
//! [5]

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    printAct = new QAction(tr("&Print..."), this);
    printAct->setShortcuts(QKeySequence::Print);
    printAct->setStatusTip(tr("Print the document"));
    connect(printAct, &QAction::triggered, this, &MainWindow::print);

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, qApp, &QApplication::closeAllWindows);

    undoAct = new QAction(tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo the last operation"));
    connect(undoAct, &QAction::triggered, this, &MainWindow::undo);

    redoAct = new QAction(tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo the last operation"));
    connect(redoAct, &QAction::triggered, this, &MainWindow::redo);

    cutAct = new QAction(tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);

    selectAllAct = new QAction(tr("Select &All"), this);
    selectAllAct->setShortcuts(QKeySequence::SelectAll);
    connect(selectAllAct, &QAction::triggered, this, &MainWindow::selectAll);

    boldAct = new QAction(tr("&Bold"), this);
    boldAct->setCheckable(true);
    boldAct->setShortcut(QKeySequence::Bold);
    boldAct->setStatusTip(tr("Make the text bold"));
    connect(boldAct, &QAction::triggered, this, &MainWindow::bold);

    QFont boldFont = boldAct->font();
    boldFont.setBold(true);
    boldAct->setFont(boldFont);

    italicAct = new QAction(tr("&Italic"), this);
    italicAct->setCheckable(true);
    italicAct->setShortcut(QKeySequence::Italic);
    italicAct->setStatusTip(tr("Make the text italic"));
    connect(italicAct, &QAction::triggered, this, &MainWindow::italic);

    QFont italicFont = italicAct->font();
    italicFont.setItalic(true);
    italicAct->setFont(italicFont);

    setLineSpacingAct = new QAction(tr("Set &Line Spacing..."), this);
    setLineSpacingAct->setStatusTip(tr("Change the gap between the lines of a "
                                       "paragraph"));
    connect(setLineSpacingAct, &QAction::triggered, this, &MainWindow::setLineSpacing);

    setParagraphSpacingAct = new QAction(tr("Set &Paragraph Spacing..."), this);
    setParagraphSpacingAct->setStatusTip(tr("Change the gap between paragraphs"));
    connect(setParagraphSpacingAct, &QAction::triggered,
            this, &MainWindow::setParagraphSpacing);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    aboutAct->setIcon(QIcon::fromTheme(QStringLiteral("help-info")));
    aboutAct->setIconVisibleInMenu(true);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(aboutQtAct, &QAction::triggered, this, &MainWindow::aboutQt);

    leftAlignAct = new QAction(tr("&Left Align"), this);
    leftAlignAct->setCheckable(true);
    leftAlignAct->setShortcut(tr("Ctrl+L"));
    leftAlignAct->setStatusTip(tr("Left align the selected text"));
    connect(leftAlignAct, &QAction::triggered, this, &MainWindow::leftAlign);

    rightAlignAct = new QAction(tr("&Right Align"), this);
    rightAlignAct->setCheckable(true);
    rightAlignAct->setShortcut(tr("Ctrl+R"));
    rightAlignAct->setStatusTip(tr("Right align the selected text"));
    connect(rightAlignAct, &QAction::triggered, this, &MainWindow::rightAlign);

    justifyAct = new QAction(tr("&Justify"), this);
    justifyAct->setCheckable(true);
    justifyAct->setShortcut(tr("Ctrl+J"));
    justifyAct->setStatusTip(tr("Justify the selected text"));
    connect(justifyAct, &QAction::triggered, this, &MainWindow::justify);

    centerAct = new QAction(tr("&Center"), this);
    centerAct->setCheckable(true);
    centerAct->setShortcut(tr("Ctrl+E"));
    centerAct->setStatusTip(tr("Center the selected text"));
    connect(centerAct, &QAction::triggered, this, &MainWindow::center);

    fullScrAct = new QAction(tr("&Fullscreen"), this);
    fullScrAct->setCheckable(true);
    fullScrAct->setShortcut(tr("F7"));
    connect(fullScrAct, &QAction::triggered, this, &MainWindow::toggleFullScreen);


    shortCutAct = new QAction(tr("shortcut test"), this);
    shortCutAct->setShortcut(m_shortCut);
    connect(shortCutAct, &QAction::triggered, this, &MainWindow::shortCutActHandler);

//! [6] //! [7]
    alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction(leftAlignAct);
    alignmentGroup->addAction(rightAlignAct);
    alignmentGroup->addAction(justifyAct);
    alignmentGroup->addAction(centerAct);
    leftAlignAct->setChecked(true);
//! [6]
#ifndef QT_NO_CONTEXTMENU
    contextMenu = new QQMenu(tr("Static contextMenu"), this);
    contextMenu->addSection(tr("Context Menu"))->setStatusTip(tr("this is a menu section"));
    contextMenu->addAction(cutAct);
    contextMenu->addAction(copyAct);
    contextMenu->addAction(pasteAct);
    if (m_shortCutActFlags & 2) {
        contextMenu->addSeparator();
        contextMenu->addAction(shortCutAct);
        addAction(shortCutAct);
    }
    connect(contextMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowContextMenu()));
#endif
}
//! [7]

void MainWindow::addMenu(QQMenu *menu, QQMenu *target)
{
    if (target) {
        target->addMenu(menu);
    } else {
        menuBar()->addMenu(menu);
    }
#ifdef SET_MENUFONT
    QFont f = menu->font();
    f.setBold(!f.bold());
    menu->setFont(f);
    f.setBold(!f.bold());
    menu->setFont(f);
#endif
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowMenu()));
}

QQMenu *MainWindow::addMenu(const QString &title, QQMenu *target)
{
    QQMenu *menu;
    if (target) {
        menu = new QQMenu(title, target);
        target->addMenu(menu);
    } else {
        menu = new QQMenu(title, menuBar());
        menuBar()->addMenu(menu);
    }
#ifdef SET_MENUFONT
    QFont f = menu->font();
    f.setBold(!f.bold());
    menu->setFont(f);
    f.setBold(!f.bold());
    menu->setFont(f);
#endif
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowMenu()));
    return menu;
}

//! [8]
void MainWindow::createMenus()
{
    QAction *action;
//! [9] //! [10]
    fileMenu = addMenu(tr("&File"));
    bool isMB = isMenubarMenu(fileMenu);
    qWarning() << Q_FUNC_INFO << "fileMenu" << fileMenu << "isNativeMenubarMenu=" << isMB;

    fileMenu->addSection(tr("\u00A7 File Actions \u00A7"));
    fileMenu->addAction(newAct);
//! [9]
    fileMenu->addAction(openAct);
//! [10]
    fileMenu->addAction(saveAct);
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(newWindowAct);
//! [11]
//     fileMenu->addSeparator();
    fileMenu->addSection(tr("Danger"));
//! [11]
    fileMenu->addAction(exitAct);

    editMenu = addMenu(tr("&Edit"));
    editMenu->addSection(tr("Edit Actions"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addSeparator();
    editMenu->addAction(selectAllAct);

    helpMenu = addMenu(tr("&Help"));
    helpMenu->addSection(tr("Self Service"));
    if (m_shortCutActFlags & 1) {
        helpMenu->addAction(shortCutAct);
        helpMenu->addSeparator();
    }
    action = new QAction(tr("An inactive item"), this);
    action->setDisabled(true);
    helpMenu->addAction(action);

    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
//! [8]

//! [12]
    formatMenu = addMenu(tr("&Format"), editMenu);
    formatMenu->setTearOffEnabled(true);
    formatMenu->addAction(fullScrAct);
    formatMenu->addSection(tr("Layout"));
    formatMenu->addAction(boldAct);
    formatMenu->addAction(italicAct);

    formatMenu->addSeparator()->setText(tr("Alignment"));

    formatMenu->addAction(leftAlignAct);
    formatMenu->addAction(rightAlignAct);
    formatMenu->addAction(justifyAct);
    formatMenu->addAction(centerAct);
    formatMenu->addSeparator();
    formatMenu->addAction(setLineSpacingAct);
    formatMenu->addAction(setParagraphSpacingAct);
    addMenu( m_widgetStyleSelector.createStyleSelectionMenu(
        QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")), tr("Widget Style"), QString(), editMenu), editMenu);
}
//! [12]
