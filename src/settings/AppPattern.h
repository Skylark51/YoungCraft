#pragma once
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QWidget>
#include <QVariant>
#include <cmath>

namespace youngcraft::uiPattern {
// A code-drawn, seamless motif keeps every template crisp on high-DPI displays.
inline void drawMotif(QPainter &p, const QRect &bounds, int motif, QColor ink) {
    p.save(); p.setClipRect(bounds); p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(ink, 1.0)); p.setBrush(Qt::NoBrush);
    const int step = motif==0 ? 96 : motif==5 ? 28 : 24;
    for(int y=(bounds.top()/step-1)*step;y<bounds.bottom()+step;y+=step)
        for(int x=(bounds.left()/step-1)*step;x<bounds.right()+step;x+=step) {
            switch(motif) {
            case 0: {
                p.save();
                QRadialGradient glow(QPointF(x+26,y+27),25);auto soft=ink;soft.setAlpha(ink.alpha()/2);
                glow.setColorAt(0,soft);soft.setAlpha(0);glow.setColorAt(1,soft);
                p.setPen(Qt::NoPen);p.setBrush(glow);p.drawEllipse(QPointF(x+26,y+27),25,25);
                QPainterPath moon,cut;moon.addEllipse(QRectF(x+16,y+12,21,21));cut.addEllipse(QRectF(x+23,y+8,21,21));
                p.setBrush(ink);p.drawPath(moon.subtracted(cut));
                for(const auto &star:QPolygonF{{double(x+64),double(y+19)},{double(x+44),double(y+65)},{double(x+83),double(y+77)}}){
                    const double r=star.x()==x+64?5.0:3.5;QPainterPath sparkle;
                    sparkle.moveTo(star.x(),star.y()-r);sparkle.quadTo(star.x()+1,star.y()-1,star.x()+r,star.y());
                    sparkle.quadTo(star.x()+1,star.y()+1,star.x(),star.y()+r);sparkle.quadTo(star.x()-1,star.y()+1,star.x()-r,star.y());
                    sparkle.quadTo(star.x()-1,star.y()-1,star.x(),star.y()-r);p.drawPath(sparkle);
                }
                p.setPen(QPen(ink,.8));p.setBrush(Qt::NoBrush);p.drawEllipse(QRectF(x+62,y+43,16,16));
                p.drawArc(QRectF(x+57,y+48,26,7),15*16,150*16);
                p.setPen(Qt::NoPen);p.setBrush(ink);p.drawEllipse(QPointF(x+9,y+66),1.2,1.2);p.drawEllipse(QPointF(x+83,y+34),1.1,1.1);
                p.restore();break;
            }
            case 1: p.drawLine(x,y,x+step,y);p.drawLine(x,y,x,y+step);break;
            case 2: p.drawLine(x,y+step,x+step,y);p.drawLine(x+12,y+step,x+step,y+12);p.drawLine(x,y+12,x+12,y);break;
            case 3: {QPainterPath wave;wave.moveTo(x,y+12);wave.cubicTo(x+6,y+3,x+6,y+3,x+12,y+12);wave.cubicTo(x+18,y+21,x+18,y+21,x+24,y+12);p.drawPath(wave);break;}
            case 4: {QPainterPath leaf;leaf.moveTo(x+5,y+20);leaf.cubicTo(x+2,y+9,x+12,y+3,x+19,y+4);leaf.cubicTo(x+22,y+14,x+13,y+21,x+5,y+20);p.drawPath(leaf);p.drawLine(x+4,y+22,x+17,y+7);break;}
            case 5: {QPolygonF hex;for(int k=0;k<6;++k){const double a=k*3.141592653589793/3;hex<<QPointF(x+14+10*std::cos(a),y+14+10*std::sin(a));}p.drawPolygon(hex);p.drawLine(x+24,y+14,x+32,y+14);p.setBrush(ink);p.drawEllipse(QPointF(x+4,y+14),1.8,1.8);p.setBrush(Qt::NoBrush);break;}
            }
        }
    p.restore();
}


inline bool active(const QWidget *widget) {
    if(widget->window()->property("solidWorkspace").toBool())return false;
    const auto value=widget->window()->property("appPattern");
    return value.isValid() && value.toInt()>=0 && value.toInt()<6;
}

// Use one window-space origin, so motifs do not restart at the square GL view.
inline bool paintBackdrop(QPainter &p,QWidget *widget,bool fill=true) {
    if(widget->window()->property("solidWorkspace").toBool()) {
        if(fill)p.fillRect(widget->rect(),widget->window()->palette().color(QPalette::Window));
        return true;
    }
    if(!active(widget))return false;
    auto *window=widget->window();
    const auto offset=widget->mapTo(window,QPoint());
    if(fill)p.fillRect(widget->rect(),window->property("appPatternPaper").value<QColor>());
    auto ink=window->property("appPatternInk").value<QColor>();ink.setAlpha(38);
    p.save();p.translate(-offset);
    drawMotif(p,QRect(offset,widget->size()),window->property("appPattern").toInt(),ink);
    p.restore();return true;
}
} // namespace youngcraft::uiPattern
