#pragma once

#include <QAbstractButton>
#include <QPainter>
#include <QStyle>

namespace youngcraft {

inline constexpr int sidebarButtonSize = 32;
inline constexpr int sidebarIconSize = 18;

inline void sizeSidebarButton(QAbstractButton *button) {
    button->setProperty("sidebarButton", true);
    button->setFixedSize(sidebarButtonSize, sidebarButtonSize);
    button->setIconSize(QSize(sidebarIconSize, sidebarIconSize));
    button->setFocusPolicy(Qt::StrongFocus);
}

inline void setSidebarButtonIcon(QAbstractButton *button, QStyle::StandardPixmap icon, const QString &label) {
    button->setIcon(button->style()->standardIcon(icon));
    button->setAccessibleName(label);
    button->setToolTip(label);
}

inline void makeSidebarIconButton(QAbstractButton *button, const QIcon &icon) {
    const auto label = button->text();
    const auto hint = button->toolTip();
    sizeSidebarButton(button);
    button->setText({});
    button->setIcon(icon);
    button->setAccessibleName(label);
    button->setToolTip(hint.isEmpty() ? label : hint);
}

inline void makeSidebarIconButton(QAbstractButton *button, QStyle::StandardPixmap icon) {
    makeSidebarIconButton(button, button->style()->standardIcon(icon));
}

inline QIcon sidebarSaveIcon() {
    QPixmap pixmap(36, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2, 2);
    painter.setPen(QPen(QColor("#446781"), 1.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(QPolygonF{{3, 2.5}, {12, 2.5}, {15, 5.5}, {15, 15.5}, {3, 15.5}});
    painter.drawPolyline(QPolygonF{{5.5, 2.5}, {5.5, 7}, {11.5, 7}, {11.5, 2.5}});
    painter.drawRoundedRect(QRectF(5.5, 10, 7, 5.5), .5, .5);
    painter.end();
    pixmap.setDevicePixelRatio(2);
    return QIcon(pixmap);
}

inline QIcon sidebarCopyIcon() {
    QPixmap pixmap(36, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2, 2);
    painter.setPen(QPen(QColor("#446781"), 1.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawRoundedRect(QRectF(6, 6, 9, 10), 1, 1);
    painter.drawPolyline(QPolygonF{{3, 12}, {3, 3}, {12, 3}});
    painter.end();
    pixmap.setDevicePixelRatio(2);
    return QIcon(pixmap);
}

} // namespace youngcraft
