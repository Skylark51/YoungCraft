#include "app/DocumentTabs.h"
#include "app/MainWindow.h"
#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QMenu>
#include <QUuid>

namespace youngcraft {
static constexpr auto documentMime = "application/x-youngcraft-document-tab";
DocumentTabs::DocumentTabs(MainWindow *owner) : QTabWidget(owner) {
    setTabBar(new DocumentTabBar(owner));
}
DocumentTabBar::DocumentTabBar(MainWindow *owner) : QTabBar(owner), m_owner(owner) {
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        const int index = tabAt(pos);
        if (index < 0) return;
        QMenu menu(this);
        auto *detach = menu.addAction(QStringLiteral("이 탭을 새 창으로 분리"));
        if (menu.exec(mapToGlobal(pos)) == detach) m_owner->detachDocument(index);
    });
}
void DocumentTabBar::mousePressEvent(QMouseEvent *event) {
    m_press = event->position().toPoint();
    m_pressedPage = m_owner->m_documentTabs->widget(tabAt(m_press));
    QTabBar::mousePressEvent(event);
}
void DocumentTabBar::mouseMoveEvent(QMouseEvent *event) {
    const QPoint pos = event->position().toPoint();
    // Horizontal motion retains Qt's normal in-window tab reordering.
    if ((event->buttons() & Qt::LeftButton) &&
        (pos - m_press).manhattanLength() >= QApplication::startDragDistance() &&
        (pos.y() < -QApplication::startDragDistance() ||
         pos.y() > height() + QApplication::startDragDistance())) {
        const int index = m_owner->m_documentTabs->indexOf(m_pressedPage);
        if (index < 0) return;
        m_page = m_pressedPage;
        m_token = QUuid::createUuid().toByteArray();
        // End QTabBar's internal reorder gesture before starting a native drag.
        QMouseEvent release(QEvent::MouseButtonRelease, event->position(), event->globalPosition(),
                            Qt::LeftButton, Qt::NoButton, event->modifiers());
        QTabBar::mouseReleaseEvent(&release);
        QPointer<DocumentTabBar> guard(this);
        auto *drag = new QDrag(this);
        auto *mime = new QMimeData;
        mime->setData(documentMime, m_token);
        drag->setMimeData(mime);
        drag->setPixmap(grab(tabRect(index)));
        drag->exec(Qt::MoveAction);
        if (!guard) return;
        drag->deleteLater();
        m_page.clear(); m_pressedPage.clear(); m_token.clear();
        return;
    }
    QTabBar::mouseMoveEvent(event);
}
void DocumentTabBar::mouseReleaseEvent(QMouseEvent *event) {
    m_pressedPage.clear();
    QTabBar::mouseReleaseEvent(event);
}
DocumentTabBar *DocumentTabBar::dragSource(QObject *source, const QMimeData *mime) {
    auto *bar = dynamic_cast<DocumentTabBar *>(source);
    return bar && bar->m_page && !bar->m_token.isEmpty() &&
        mime->data(documentMime) == bar->m_token ? bar : nullptr;
}
bool DocumentTabBar::dropInto(MainWindow *target) {
    return m_page && m_owner->moveDocumentTo(m_owner->m_documentTabs->indexOf(m_page), target);
}
void DocumentTabBar::dragEnterEvent(QDragEnterEvent *event) {
    if (dragSource(event->source(), event->mimeData())) event->acceptProposedAction();
    else QTabBar::dragEnterEvent(event);
}
void DocumentTabBar::dragMoveEvent(QDragMoveEvent *event) {
    if (dragSource(event->source(), event->mimeData())) event->acceptProposedAction();
    else QTabBar::dragMoveEvent(event);
}
void DocumentTabBar::dropEvent(QDropEvent *event) {
    if (auto *source = dragSource(event->source(), event->mimeData())) {
        if (source->dropInto(m_owner)) event->acceptProposedAction();
    } else QTabBar::dropEvent(event);
}
}
