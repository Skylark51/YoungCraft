#pragma once
#include "ui/SidebarPaper.h"
#include <QApplication>
#include <QDialog>
#include <QPointer>
#include <QScreen>
#include <QTimer>

namespace youngcraft::uiPattern {
class DialogThemeSync final : public QObject {
public:
    explicit DialogThemeSync(QWidget *owner):QObject(owner),m_owner(owner) {
        setObjectName("DialogThemeSynchronizer");qApp->installEventFilter(this);
    }
    void refresh() {
        if(m_syncing || !m_owner)return;
        m_syncing=true;
        for(auto *dialog:m_owner->findChildren<QDialog*>())apply(dialog);
        m_syncing=false;
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(event->type()!=QEvent::Show || m_syncing)return false;
        auto *dialog=qobject_cast<QDialog*>(object);
        if(!dialog || !m_owner)return false;
        // QWidget::isAncestorOf stops at window boundaries; dialogs are windows.
        bool owned=false;
        for(auto *parent=dialog->parentWidget();parent;parent=parent->parentWidget())
            if(parent==m_owner){owned=true;break;}
        if(!owned)return false;
        m_syncing=true;apply(dialog);m_syncing=false;
        return false;
    }
private:
    void apply(QDialog *dialog) {
        if(!dialog->property("appThemeOriginalSaved").toBool()) {
            dialog->setProperty("appThemeOriginalSaved",true);
            dialog->setProperty("appThemeOriginalStyle",dialog->styleSheet());
        }
        dialog->setProperty("appThemeDialog",true);
        for(const char *key:{"appPattern","appPatternPaper","appPatternInk"})dialog->setProperty(key,m_owner->property(key));
        dialog->setPalette(m_owner->palette());
        if(dialog->property("solidWorkspace").toBool()) {
            dialog->setAutoFillBackground(true);
            dialog->setAttribute(Qt::WA_TranslucentBackground,false);
            dialog->setWindowOpacity(1.0);
        }
        QString rules=QStringLiteral("QDialog[appThemeDialog=\"true\"] {background:%1; color:#34404b;}")
            .arg(m_owner->property("appPatternPaper").value<QColor>().name());
        if(active(dialog))rules+=QStringLiteral(
            "QDialog[appThemeDialog=\"true\"] QStackedWidget, QDialog[appThemeDialog=\"true\"] QScrollArea, "
            "QDialog[appThemeDialog=\"true\"] QAbstractItemView, QDialog[appThemeDialog=\"true\"] QTabWidget::pane, "
            "QDialog[appThemeDialog=\"true\"] QGroupBox {background:transparent; background-image:none;}"
            "QDialog[appThemeDialog=\"true\"] QHeaderView::section {background:transparent;}");
        if(dialog->property("solidWorkspace").toBool())rules+=QStringLiteral(
            "QDialog[solidWorkspace=\"true\"] QScrollArea,QDialog[solidWorkspace=\"true\"] QScrollArea > QWidget,"
            "QDialog[solidWorkspace=\"true\"] QStackedWidget,QDialog[solidWorkspace=\"true\"] QGroupBox,"
            "QDialog[solidWorkspace=\"true\"] QTabWidget::pane {background:%1;}"
            "QDialog[solidWorkspace=\"true\"] QAbstractItemView {background:%2;}")
            .arg(m_owner->palette().color(QPalette::Window).name(),m_owner->palette().color(QPalette::Base).name());
        dialog->setStyleSheet(dialog->property("appThemeOriginalStyle").toString()+m_owner->styleSheet()+rules);
        // Clamp the decorated frame after native placement; never assume one
        // monitor's resolution or origin (secondary monitors may be negative).
        QTimer::singleShot(0,dialog,[dialog] {
            auto *screen=dialog->screen();if(!screen || !dialog->isVisible() || dialog->isMaximized())return;
            const auto area=screen->availableGeometry().adjusted(8,8,-8,-8);
            auto frame=dialog->frameGeometry();const auto decoration=frame.size()-dialog->size();
            dialog->resize(qMin(dialog->width(),qMax(1,area.width()-decoration.width())),
                           qMin(dialog->height(),qMax(1,area.height()-decoration.height())));
            frame=dialog->frameGeometry();
            dialog->move(qBound(area.left(),frame.left(),qMax(area.left(),area.right()-frame.width()+1)),
                         qBound(area.top(),frame.top(),qMax(area.top(),area.bottom()-frame.height()+1)));
        });
        watchSidebar(dialog);
        dialog->update();for(auto *child:dialog->findChildren<QWidget*>())child->update();
    }
    QPointer<QWidget> m_owner;
    bool m_syncing=false;
};
inline void synchronizeDialogs(QWidget *owner) {
    auto *sync=static_cast<DialogThemeSync*>(owner->findChild<QObject*>("DialogThemeSynchronizer",Qt::FindDirectChildrenOnly));
    if(!sync)sync=new DialogThemeSync(owner);
    sync->refresh();
}
} // namespace youngcraft::uiPattern
