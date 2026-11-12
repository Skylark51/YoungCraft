#pragma once
#include "ui/AppPattern.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QFontMetricsF>
#include <QPointer>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSettings>
#include <QSplashScreen>
#include <QTimer>
#include <functional>

namespace youngcraft {
class StartupSplash final : public QSplashScreen {
public:
    StartupSplash():QSplashScreen(artwork(),Qt::WindowStaysOnTopHint) {
        setObjectName("YoungcraftStartupSplash");
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_QuitOnClose,false);
    }
    void finishWhenReady(QWidget *mainWindow, std::function<void()> onReady = {}) {
        const QPointer<QWidget> ready(mainWindow);
        // Include the fade in the one-second splash interval.
        QTimer::singleShot(splashDurationMs-fadeDurationMs,Qt::PreciseTimer,this,[this,ready,onReady]{
            if(!ready || !isVisible()){hide();return;}
            auto *fade=new QPropertyAnimation(this,"windowOpacity",this);
            fade->setDuration(fadeDurationMs);fade->setStartValue(1.0);fade->setEndValue(0.0);
            connect(fade,&QPropertyAnimation::finished,this,[this,ready,onReady]{
                if(ready){ready->show();finish(ready);if(onReady)onReady();}
                else hide();
            });
            fade->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }
protected:
    void mousePressEvent(QMouseEvent *) override {} // Preserve the startup interval on click.
private:
    static constexpr int splashDurationMs=1000;
    static constexpr int fadeDurationMs=180;
    static QPixmap artwork() {
        const char *papers[]={"#edf4fa","#eef4ee","#f2eef8","#f6f1e7","#f8eef0","#faf0e8"};
        const char *inks[]={"#446781","#4d6c5b","#6d5b85","#79634c","#865e6d","#865f49"};
        const int theme=qBound(0,QSettings().value("ui/theme",0).toInt(),5);
        const QColor paper(papers[theme]),ink(inks[theme]);
        const qreal dpr=qApp->primaryScreen()?qApp->primaryScreen()->devicePixelRatio():1;
        QPixmap result(QSize(qRound(640*dpr),qRound(300*dpr)));result.setDevicePixelRatio(dpr);result.fill(Qt::transparent);
        QPainter p(&result);p.setRenderHint(QPainter::Antialiasing);p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath card;card.addRoundedRect(QRectF(1,1,638,298),20,20);p.setClipPath(card);
        QLinearGradient background(0,0,640,300);background.setColorAt(0,paper.lighter(103));background.setColorAt(1,paper);p.fillRect(QRect(0,0,640,300),background);
        const int motif=QSettings().value("ui/patternTemplate",-1).toInt();
        if(motif>=0 && motif<6){auto faint=ink;faint.setAlpha(24);uiPattern::drawMotif(p,QRect(0,0,640,300),motif,faint);}
        p.setClipping(false);auto border=ink;border.setAlpha(65);p.setPen(QPen(border,1));p.setBrush(Qt::NoBrush);p.drawPath(card);
        p.drawPixmap(QRect(32,30,172,172),QPixmap(QStringLiteral(":/youngcraft/icon.png")));
        QFont title(QStringLiteral("Georgia"));title.setPixelSize(46);title.setItalic(true);
        QPainterPath wordmark;wordmark.addText(QPointF(228,100),title,QStringLiteral("Young"));
        const qreal craftX=228+QFontMetricsF(title).horizontalAdvance(QStringLiteral("Young"))+1;
        title.setItalic(false);wordmark.addText(QPointF(craftX,100),title,QStringLiteral("Craft"));
        QLinearGradient lettering(228,60,520,108);
        lettering.setColorAt(0,ink.darker(120));lettering.setColorAt(0.55,ink);lettering.setColorAt(1,ink.lighter(125));
        p.fillPath(wordmark,lettering);
        QPainterPath flourish;flourish.moveTo(232,114);flourish.cubicTo(300,104,407,124,508,109);
        auto accent=ink;accent.setAlpha(95);p.setPen(QPen(accent,1.2,Qt::SolidLine,Qt::RoundCap));p.drawPath(flourish);
        const QString tagline=QStringLiteral("Molecules Rendering, Orbital Analysis, Capture Tools");
        QFont subtitle(QStringLiteral("Georgia"));subtitle.setPixelSize(15);subtitle.setItalic(true);
        subtitle.setLetterSpacing(QFont::AbsoluteSpacing,0.25);
        while(subtitle.pixelSize()>10 && QFontMetricsF(subtitle).horizontalAdvance(tagline)>374)subtitle.setPixelSize(subtitle.pixelSize()-1);
        p.setFont(subtitle);p.setPen(ink.darker(110));
        p.drawText(QRect(230,132,374,55),Qt::AlignLeft|Qt::AlignTop,tagline);
        auto line=ink;line.setAlpha(50);p.setPen(QPen(line,1));p.drawLine(36,226,604,226);
        subtitle.setFamily(QStringLiteral("맑은 고딕"));subtitle.setPixelSize(13);subtitle.setItalic(false);subtitle.setLetterSpacing(QFont::AbsoluteSpacing,0);p.setFont(subtitle);p.setPen(ink);
        p.drawText(QRect(38,242,460,25),Qt::AlignLeft|Qt::AlignVCenter,QStringLiteral("작업 공간을 준비하고 있습니다…"));
        p.drawText(QRect(505,242,97,25),Qt::AlignRight|Qt::AlignVCenter,QStringLiteral("v%1").arg(QCoreApplication::applicationVersion()));
        return result;
    }
};
}
