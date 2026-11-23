#pragma once

#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QWidget>

namespace youngcraft {

// Choose by operation at the callsite, never by translated button text.
enum class UiIcon {
    EditAtoms, EditBonds, InsertFragment, PeriodicTable, Calculate, Optimize,
    Geometry, Frequency, Scan, Irc, Spectrum, Orbital, ImageCopy, ImageSave,
    Export, Library, Style, Show, Hide, Lock, Save, Copy, Measure,
    Play, Pause, Stop, Apply, Reset
};

namespace uiIconDetail {

inline void drawGlyph(QPainter &p, UiIcon icon) {
    const auto line = [&p](qreal x1, qreal y1, qreal x2, qreal y2) {
        p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    };
    const auto atom = [&p](qreal x, qreal y, qreal radius = 2) {
        p.drawEllipse(QPointF(x, y), radius, radius);
    };
    const auto plus = [&line](qreal x, qreal y) {
        line(x - 2, y, x + 2, y);
        line(x, y - 2, x, y + 2);
    };
    const auto image = [&p, &atom] {
        p.drawRoundedRect(QRectF(3, 3, 17, 14), 1.5, 1.5);
        atom(15.5, 6.5, 1);
        p.drawPolyline(QPolygonF{{5, 14}, {9, 9}, {13, 13}, {16, 10}, {18, 12}});
    };
    const auto axes = [&line] {
        line(3, 3, 3, 20);
        line(3, 20, 21, 20);
    };
    const auto molecule = [&atom, &line] {
        line(6, 7, 11, 16);
        line(11, 16, 19, 6);
        atom(5, 5);
        atom(11, 18);
        atom(20, 4);
    };
    switch (icon) {
    case UiIcon::EditAtoms:
        atom(8, 8, 5);
        p.drawPolygon(QPolygonF{{11, 18}, {19, 10}, {22, 13}, {14, 21}, {10, 22}});
        line(17, 12, 20, 15);
        break;
    case UiIcon::EditBonds:
        atom(4, 12, 2.5);
        atom(20, 12, 2.5);
        line(7, 10, 17, 10);
        line(7, 14, 17, 14);
        break;
    case UiIcon::InsertFragment:
        p.drawPolygon(QPolygonF{{3, 6}, {8, 3}, {13, 6}, {13, 12}, {8, 15}, {3, 12}});
        line(13, 12, 17, 14);
        plus(19, 18);
        break;
    case UiIcon::PeriodicTable:
        p.drawRect(QRectF(2, 3, 4, 17));
        p.drawRect(QRectF(6, 11, 8, 9));
        p.drawRect(QRectF(14, 7, 8, 13));
        line(18, 7, 18, 20);
        line(10, 11, 10, 20);
        line(2, 15.5, 22, 15.5);
        break;
    case UiIcon::Calculate:
        p.drawRoundedRect(QRectF(4, 2, 16, 20), 2, 2);
        p.drawRect(QRectF(7, 5, 10, 4));
        plus(9, 14);
        line(14, 14, 17, 14);
        line(7, 19, 10, 19);
        line(14, 18, 17, 18);
        line(14, 20, 17, 20);
        break;
    case UiIcon::Optimize:
        line(5, 6, 10, 15);
        line(10, 15, 15, 7);
        atom(4, 4);
        atom(10, 17);
        atom(16, 5);
        line(21, 12, 21, 21);
        p.drawPolyline(QPolygonF{{18, 18}, {21, 21}, {23, 18}});
        break;
    case UiIcon::Geometry:
        molecule();
        line(6, 6, 18, 4);
        break;
    case UiIcon::Frequency: {
        QPainterPath wave;
        wave.moveTo(2, 12);
        wave.cubicTo(5, 0, 8, 0, 12, 12);
        wave.cubicTo(16, 24, 19, 24, 22, 12);
        p.drawPath(wave);
        break;
    }
    case UiIcon::Scan:
        axes();
        p.drawPolyline(QPolygonF{{6, 6}, {10, 14}, {15, 16}, {20, 7}});
        atom(6, 6, 1.3);
        atom(15, 16, 1.3);
        atom(20, 7, 1.3);
        break;
    case UiIcon::Irc: {
        axes();
        QPainterPath path;
        path.moveTo(6, 16);
        path.cubicTo(10, 16, 9, 6, 13, 6);
        path.cubicTo(17, 6, 16, 16, 21, 16);
        p.drawPath(path);
        atom(13, 6, 1.6);
        break;
    }
    case UiIcon::Spectrum:
        axes();
        p.drawPolyline(QPolygonF{{6, 17}, {8, 17}, {9, 6}, {10, 17},
                                  {13, 17}, {14, 10}, {15, 17}, {18, 17},
                                  {19, 4}, {20, 17}});
        break;
    case UiIcon::Orbital:
        p.save();
        p.translate(12, 12);
        p.rotate(-35);
        p.drawEllipse(QRectF(-10, -4, 10, 8));
        p.drawEllipse(QRectF(0, -4, 10, 8));
        plus(-5, 0);
        line(3, 0, 7, 0);
        p.restore();
        break;
    case UiIcon::ImageCopy:
        image();
        p.drawPolyline(QPolygonF{{6, 20}, {22, 20}, {22, 7}});
        break;
    case UiIcon::ImageSave:
        image();
        line(12, 15, 12, 22);
        p.drawPolyline(QPolygonF{{9, 19}, {12, 22}, {15, 19}});
        break;
    case UiIcon::Export:
        p.drawPolyline(QPolygonF{{4, 12}, {4, 21}, {20, 21}, {20, 12}});
        line(12, 16, 12, 2);
        p.drawPolyline(QPolygonF{{7, 7}, {12, 2}, {17, 7}});
        break;
    case UiIcon::Library:
        p.drawRoundedRect(QRectF(2, 4, 5, 17), 1, 1);
        p.drawRoundedRect(QRectF(8, 4, 5, 17), 1, 1);
        p.drawPolygon(QPolygonF{{15, 5}, {19, 4}, {23, 20}, {18, 21}});
        line(3, 8, 6, 8);
        line(9, 8, 12, 8);
        break;
    case UiIcon::Style:
        line(3, 6, 21, 6);
        line(3, 12, 21, 12);
        line(3, 18, 21, 18);
        p.drawRoundedRect(QRectF(7, 3, 3, 6), 1, 1);
        p.drawRoundedRect(QRectF(15, 9, 3, 6), 1, 1);
        p.drawRoundedRect(QRectF(6, 15, 3, 6), 1, 1);
        break;
    case UiIcon::Show:
    case UiIcon::Hide: {
        QPainterPath eye;
        eye.moveTo(2, 12);
        eye.quadTo(12, 0, 22, 12);
        eye.quadTo(12, 24, 2, 12);
        p.drawPath(eye);
        atom(12, 12, 3);
        if (icon == UiIcon::Hide) line(3, 3, 21, 21);
        break;
    }
    case UiIcon::Lock:
        p.drawRoundedRect(QRectF(5, 10, 14, 11), 1.5, 1.5);
        p.drawArc(QRectF(8, 2, 8, 15), 0, 180 * 16);
        line(12, 14, 12, 17);
        break;
    case UiIcon::Save:
        p.drawPolygon(QPolygonF{{3, 2}, {16, 2}, {21, 7}, {21, 22}, {3, 22}});
        p.drawPolyline(QPolygonF{{7, 2}, {7, 9}, {16, 9}, {16, 2}});
        p.drawRoundedRect(QRectF(7, 14, 10, 8), 1, 1);
        break;
    case UiIcon::Copy:
        p.drawRoundedRect(QRectF(8, 8, 13, 14), 1.5, 1.5);
        p.drawPolyline(QPolygonF{{4, 16}, {3, 16}, {3, 3}, {16, 3}, {16, 4}});
        break;
    case UiIcon::Measure:
        p.drawRect(QRectF(2, 7, 20, 10));
        for (int x = 6; x < 22; x += 4) line(x, 7, x, x == 10 || x == 18 ? 13 : 11);
        break;
    case UiIcon::Play:
        p.drawPolygon(QPolygonF{{7, 3}, {21, 12}, {7, 21}});
        break;
    case UiIcon::Pause:
        p.drawRect(QRectF(6, 3, 3, 18));
        p.drawRect(QRectF(15, 3, 3, 18));
        break;
    case UiIcon::Stop:
        p.drawRect(QRectF(5, 5, 14, 14));
        break;
    case UiIcon::Apply:
        p.drawPolyline(QPolygonF{{3, 12}, {9, 18}, {21, 5}});
        break;
    case UiIcon::Reset:
        p.drawArc(QRectF(5, 5, 15, 15), -55 * 16, 285 * 16);
        p.drawPolyline(QPolygonF{{3, 6}, {5, 12}, {10, 9}});
        break;
    }
}

class IconEngine final : public QIconEngine {
public:
    explicit IconEngine(UiIcon icon, QWidget *context) : m_icon(icon), m_context(context) {}

    QIconEngine *clone() const override { return new IconEngine(m_icon, m_context.data()); }
    QString key() const override { return QStringLiteral("YoungcraftUiIcon"); }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override {
        const QPalette palette = m_context ? m_context->palette() : QApplication::palette();
        const auto group = mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
        const auto role = mode == QIcon::Selected || (state == QIcon::On && mode != QIcon::Disabled)
            ? QPalette::HighlightedText : QPalette::ButtonText;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const qreal side = qMin(rect.width(), rect.height());
        painter->translate(rect.x() + (rect.width() - side) / 2,
                           rect.y() + (rect.height() - side) / 2);
        painter->scale(side / 24, side / 24);
        painter->setPen(QPen(palette.color(group, role), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        drawGlyph(*painter, m_icon);
        painter->restore();
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        return scaledPixmap(size, mode, state, 1);
    }

    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override {
        if (size.isEmpty()) return {};
        // Qt 6.8 passes a logical size here; allocate physical pixels at DPR.
        QPixmap result(size * scale);
        result.setDevicePixelRatio(scale);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, QRect(QPoint(), size), mode, state);
        painter.end();
        return result;
    }

private:
    UiIcon m_icon;
    QPointer<QWidget> m_context;
};

} // namespace uiIconDetail

// Passing the target widget tracks its palette (including theme changes) safely.
// QIconEngine paints at the requested DPI; no PNG/SVG assets or build rules needed.
inline QIcon uiIcon(UiIcon icon, QWidget *context = nullptr) {
    return QIcon(new uiIconDetail::IconEngine(icon, context));
}

} // namespace youngcraft
