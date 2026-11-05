#pragma once
#include "settings/AppPattern.h"
#include <QAbstractItemView>
#include <QDockWidget>
#include <QEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>

namespace youngcraft::uiPattern {
// Native controls paint their content over a single window-aligned paper layer.
class SidebarPaper final : public QObject {
public:
    explicit SidebarPaper(QWidget *dock):QObject(dock) {setObjectName("SidebarPaperPainter");}
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        auto *widget=qobject_cast<QWidget*>(object);
        if(widget && active(widget)) {
            if(event->type()==QEvent::Paint){QPainter painter(widget);paintBackdrop(painter,widget);}
            else if(event->type()==QEvent::Move) {
                widget->update();
                for(auto *child:widget->findChildren<QWidget*>())if(child->property("sidebarPaperWatched").toBool())child->update();
            }
        }
        return false;
    }
};

inline void watchSidebar(QWidget *dock) {
    auto *filter=static_cast<SidebarPaper*>(dock->findChild<QObject*>("SidebarPaperPainter",Qt::FindDirectChildrenOnly));
    if(!filter)filter=new SidebarPaper(dock);
    auto watch=[filter](QWidget *widget){
        if(widget->property("sidebarPaperWatched").toBool())return;
        widget->setProperty("sidebarPaperWatched",true);widget->installEventFilter(filter);
    };
    watch(dock);
    for(auto *widget:dock->findChildren<QWidget*>()) {
        if(qobject_cast<QStackedWidget*>(widget)||qobject_cast<QSplitter*>(widget)||qobject_cast<QTabWidget*>(widget)
           ||widget->property("sidebarPatternPaper").toBool())watch(widget);
        auto *area=qobject_cast<QAbstractScrollArea*>(widget);
        if(!area || (!qobject_cast<QAbstractItemView*>(area)&&!qobject_cast<QScrollArea*>(area)))continue;
        watch(area->viewport());
        if(!area->property("sidebarPaperScrollWatched").toBool()) {
            area->setProperty("sidebarPaperScrollWatched",true);
            auto refresh=[dock]{for(auto *child:dock->findChildren<QWidget*>())if(child->property("sidebarPaperWatched").toBool())child->update();};
            QObject::connect(area->verticalScrollBar(),&QScrollBar::valueChanged,area,refresh);
            QObject::connect(area->horizontalScrollBar(),&QScrollBar::valueChanged,area,refresh);
        }
    }
}
} // namespace youngcraft::uiPattern
