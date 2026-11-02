#pragma once
#include <QTabBar>
#include <QTabWidget>
#include <QPointer>

class QMimeData;

namespace youngcraft {
class MainWindow;
// QDrag carries an identity token, never a raw pointer or a file to reopen.
class DocumentTabBar final : public QTabBar {
public:
    explicit DocumentTabBar(MainWindow *owner);
    static DocumentTabBar *dragSource(QObject *source, const QMimeData *mime);
    bool dropInto(MainWindow *target);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    MainWindow *m_owner;
    QPoint m_press;
    QPointer<QWidget> m_pressedPage, m_page;
    QByteArray m_token;
};
class DocumentTabs final : public QTabWidget {
public:
    explicit DocumentTabs(MainWindow *owner);
};
}
