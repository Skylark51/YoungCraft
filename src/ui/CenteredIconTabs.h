#pragma once
#include <QTabBar>
#include <QTabWidget>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QStyleOptionFocusRect>

namespace youngcraft {
// Tab buttons stay in their original positions; only their glyphs use centered painting.
class CenteredIconTabBar final : public QTabBar {
public:
    explicit CenteredIconTabBar(QWidget *parent=nullptr):QTabBar(parent){}
    QRect buttonRect(int index) const { return tabRect(index).adjusted(0,0,-4,0); } // 4px stylesheet gap.
    QRect iconRect(int index) const {
        const auto button=buttonRect(index);const auto size=iconSize().boundedTo(button.size());
        return QRect(button.x()+(button.width()-size.width())/2,button.y()+(button.height()-size.height())/2,size.width(),size.height());
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QStylePainter painter(this);
        for(int index=0;index<count();++index){
            QStyleOptionTab option;initStyleOption(&option,index);
            painter.drawControl(QStyle::CE_TabBarTabShape,option);
            const auto mode=option.state&QStyle::State_Enabled?QIcon::Normal:QIcon::Disabled;
            const auto state=index==currentIndex()?QIcon::On:QIcon::Off;
            option.icon.paint(&painter,iconRect(index),Qt::AlignCenter,mode,state);
            if(index==currentIndex() && hasFocus()){
                QStyleOptionFocusRect focus;focus.initFrom(this);focus.rect=buttonRect(index).adjusted(3,3,-3,-3);
                painter.drawPrimitive(QStyle::PE_FrameFocusRect,focus);
            }
        }
    }
};
class CenteredIconTabs final : public QTabWidget {
public:
    explicit CenteredIconTabs(QWidget *parent=nullptr):QTabWidget(parent){setTabBar(new CenteredIconTabBar(this));}
};
} // namespace youngcraft
