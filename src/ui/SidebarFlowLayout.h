#pragma once

#include <QLayout>
#include <QWidget>
#include <QList>

namespace youngcraft {

// Fixed-size sidebar controls fill each row in order and wrap at the available width.
class SidebarFlowLayout final : public QLayout {
public:
    explicit SidebarFlowLayout(QWidget *parent = nullptr) : QLayout(parent) {
        setContentsMargins(0, 0, 0, 0);
        setSpacing(4);
    }
    ~SidebarFlowLayout() override {
        while (auto *item = takeAt(0)) delete item;
    }
    void addItem(QLayoutItem *item) override { m_items.append(item); invalidate(); }
    int count() const override { return int(m_items.size()); }
    QLayoutItem *itemAt(int index) const override { return m_items.value(index); }
    QLayoutItem *takeAt(int index) override {
        if (index < 0 || index >= m_items.size()) return nullptr;
        auto *item = m_items.takeAt(index);
        invalidate();
        return item;
    }
    Qt::Orientations expandingDirections() const override { return Qt::Horizontal; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return arrange(QRect(0, 0, width, 0), false); }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize result;
        for (auto *item : m_items) if (!item->isEmpty()) result = result.expandedTo(item->minimumSize());
        const auto margins = contentsMargins();
        return result + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }
    void setGeometry(const QRect &rect) override { QLayout::setGeometry(rect); arrange(rect, true); }
private:
    int arrange(const QRect &rect, bool apply) const {
        const auto margins = contentsMargins();
        const auto area = rect.marginsRemoved(margins);
        const int gap = qMax(0, spacing());
        int x = area.x(), y = area.y(), rowHeight = 0;
        for (auto *item : m_items) {
            if (item->isEmpty()) continue;
            const auto size = item->sizeHint().expandedTo(item->minimumSize()).boundedTo(item->maximumSize());
            if (rowHeight && x + size.width() > area.x() + area.width()) {
                x = area.x();
                y += rowHeight + gap;
                rowHeight = 0;
            }
            if (apply) item->setGeometry(QRect(QPoint(x, y), size));
            x += size.width() + gap;
            rowHeight = qMax(rowHeight, size.height());
        }
        return y + rowHeight - rect.y() + margins.bottom();
    }
    QList<QLayoutItem *> m_items;
};

} // namespace youngcraft
