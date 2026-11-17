#pragma once
#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace youngcraft {
inline QIcon transitionAnalysisIcon() {
    QPixmap image(64,64);image.fill(Qt::transparent);image.setDevicePixelRatio(2);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.setPen(Qt::NoPen);
    p.setBrush(QColor("#df7733"));p.drawEllipse(QRectF(2,4,9,12));p.drawEllipse(QRectF(2,16,9,12));
    p.setBrush(QColor("#257fce"));p.drawEllipse(QRectF(21,4,9,12));p.drawEllipse(QRectF(21,16,9,12));
    p.setPen(QPen(QColor("#46647a"),1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.drawLine(QPointF(12,16),QPointF(20,16));p.drawLine(QPointF(17,13),QPointF(20,16));p.drawLine(QPointF(17,19),QPointF(20,16));
    p.end();return QIcon(image);
}
}
