/* This file is part of the KDE project
 * Copyright (C) 2016 René J.V. Bertin <rjvbertin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "qwidgetstyleselector.h"

#ifdef Q_OS_WIN
#include <QSysInfo>
#endif
#include <QString>
#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QStyle>
#include <QStyleFactory>
#include <QApplication>
#include <QDebug>

static QString getDefaultStyle(const char *fallback=Q_NULLPTR)
{
    // TODO: implement a default setting
    if (!fallback) {
#ifdef Q_OS_MACOS
        fallback = "Macintosh";
#elif defined(Q_OS_WIN)
        // taken from QWindowsTheme::styleNames()
        if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA) {
            fallback = "WindowsVista";
        } else if (QSysInfo::windowsVersion() >= QSysInfo::WV_XP) {
            fallback = "WindowsXP";
        } else {
            fallback = "Windows";
        }
#else
        fallback = "Breeze";
#endif
    }
    return fallback;
}

QWidgetStyleSelector::QWidgetStyleSelector(QWidget *parent)
    : QWidget(parent)
    , m_widgetStyle(QString())
    , m_parent(parent)
{
}

QWidgetStyleSelector::~QWidgetStyleSelector()
{
}

QMenu *QWidgetStyleSelector::createStyleSelectionMenu(const QIcon &icon, const QString &text,
                                                            const QString &selectedStyleName, QWidget *parent)
{
    // Taken from Kdenlive:
    // Widget themes for non KDE users
    if (!parent) {
        parent = m_parent;
    }
    QMenu *stylesAction= new QMenu(text, parent);
    if (!icon.isNull()) {
        stylesAction->setIcon(icon);
    }
    stylesAction->setStatusTip(tr("Select the application widget style"));
    QActionGroup *stylesGroup = new QActionGroup(stylesAction);

    QStringList availableStyles = QStyleFactory::keys();
    QString desktopStyle = QApplication::style()->objectName();
    QString defaultStyle = getDefaultStyle();

    // Add default style action
    QAction *defaultStyleAction = new QAction(tr("Default"), stylesGroup);
    defaultStyleAction->setCheckable(true);
    // without a settings store we can only treat the Default entry
    // as "select the default style for this platform".
    // (see also the defaultStyleAction->setData() call below.
    defaultStyleAction->setStatusTip(tr("Default widget style for this platform: %1").arg(getDefaultStyle()));

    stylesAction->addAction(defaultStyleAction);
    m_widgetStyle = selectedStyleName;
    bool setStyle = false;
    if (m_widgetStyle.isEmpty()) {
        if (desktopStyle.compare(defaultStyle, Qt::CaseInsensitive) == 0) {
            defaultStyleAction->setChecked(true);
            m_widgetStyle = defaultStyleAction->text();
        } else {
            m_widgetStyle = desktopStyle;
        }
    } else if (selectedStyleName.compare(desktopStyle, Qt::CaseInsensitive)) {
        setStyle = true;
    }

    foreach(const QString &style, availableStyles) {
        QAction *a = new QAction(style, stylesGroup);
        a->setCheckable(true);
        a->setData(style);
        if (style.compare(defaultStyle, Qt::CaseInsensitive) == 0) {
            QFont defFont = a->font();
            defFont.setBold(true);
            a->setFont(defFont);
            a->setStatusTip(tr("Default widget style for this platform"));
            defaultStyleAction->setData(style);
        }
        if (m_widgetStyle.compare(style, Qt::CaseInsensitive) == 0) {
            a->setChecked(true);
            if (setStyle) {
                // selectedStyleName was not empty and the
                // the style exists: activate it.
                activateStyle(style);
            }
        }
        stylesAction->addAction(a);
    }
    connect(stylesGroup, &QActionGroup::triggered, this, [&](QAction *a) {
        qWarning() << Q_FUNC_INFO << a << "; activating style" << a->data();
        activateStyle(a->data().toString());
    });
    return stylesAction;
}

QMenu *QWidgetStyleSelector::createStyleSelectionMenu(const QString &text,
                                                            const QString &selectedStyleName, QWidget *parent)
{
    return createStyleSelectionMenu(QIcon(), text, selectedStyleName, parent);
}

QMenu *QWidgetStyleSelector::createStyleSelectionMenu(const QString &selectedStyleName, QWidget *parent)
{
    return createStyleSelectionMenu(QIcon(), tr("Style"), selectedStyleName, parent);
}

QString QWidgetStyleSelector::currentStyle() const
{
    if (m_widgetStyle.isEmpty() || m_widgetStyle == QStringLiteral("Default")) {
        return getDefaultStyle();
    }
    return m_widgetStyle;
}

void QWidgetStyleSelector::activateStyle(const QString &styleName)
{
    m_widgetStyle = styleName;
    QApplication::setStyle(QStyleFactory::create(currentStyle()));
}
