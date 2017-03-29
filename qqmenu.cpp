#include <QFont>
#include <QDebug>

#include "qqmenu.h"

QQMenu::QQMenu(const QString &title, QWidget *parent)
    : QMenu(title, parent)
{
    QFont f = font();
//     qWarning() << Q_FUNC_INFO << "menu=" << title << "font=" << f;
    // force QAction::setFont() to reset the font so that menu items show
    // the actual font even when attached to native menubars on Mac.
    f.setBold(!f.bold());
    setFont(f);
    f.setBold(!f.bold());
    setFont(f);
}

void QQMenu::addAction(QAction *action)
{
    if (action) {
        QFont f = action->font();
//         qWarning() << Q_FUNC_INFO << "item=" << action << "font=" << f;
#ifdef SET_MENUFONT
        f.setBold(!f.bold());
        action->setFont(f);
        f.setBold(!f.bold());
        action->setFont(f);
#endif
        QMenu::addAction(action);
    }
}

QAction *QQMenu::addSection(const QString &title)
{
    QAction *section = QMenu::addSection(title);
    QFont f = section->font();
//     qWarning() << Q_FUNC_INFO << "section=" << section << "font=" << f;
#ifdef SET_MENUFONT
    f.setBold(!f.bold());
    section->setFont(f);
    f.setBold(!f.bold());
    section->setFont(f);
#endif
    section->setStatusTip(tr("this is a menu section"));
    return section;
}

