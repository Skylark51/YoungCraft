#pragma once
#include "ui/SidebarPaper.h"
#include <QApplication>
#include <QDialog>
#include <QPointer>

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
        QString rules=QStringLiteral("QDialog[appThemeDialog=\"true\"] {background:%1; color:#34404b;}")
            .arg(m_owner->property("appPatternPaper").value<QColor>().name());
        if(active(dialog))rules+=QStringLiteral(
            "QDialog[appThemeDialog=\"true\"] QStackedWidget, QDialog[appThemeDialog=\"true\"] QScrollArea, "
            "QDialog[appThemeDialog=\"true\"] QAbstractItemView, QDialog[appThemeDialog=\"true\"] QTabWidget::pane, "
            "QDialog[appThemeDialog=\"true\"] QGroupBox {background:transparent; background-image:none;}"
            "QDialog[appThemeDialog=\"true\"] QHeaderView::section {background:transparent;}");
        dialog->setStyleSheet(dialog->property("appThemeOriginalStyle").toString()+m_owner->styleSheet()+rules);
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
