#include "ui/PlotWidget.h"

#include <QMouseEvent>
#include <QLineF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScopedValueRollback>

#include <algorithm>
#include <cmath>

namespace youngcraft {
namespace {
double niceStep(double raw) {
    if (!std::isfinite(raw) || raw <= 0) return 1;
    const double scale = std::pow(10.0, std::floor(std::log10(raw)));
    const double fraction = raw / scale;
    for (double factor : {1.0, 2.0, 2.5, 5.0, 10.0})
        if (fraction <= factor + 1e-10) return factor * scale;
    return 10 * scale;
}
}

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void PlotWidget::setSeries(const QVector<QPointF> &series, const QString &xLabel, const QString &yLabel) {
    if (m_heatmap) clearManualRange();
    m_scan = false;
    m_approximateTsPoint = -1;
    m_heatmap = false;
    m_heatmapPoints.clear();
    m_heatmapXValues.clear();
    m_heatmapYValues.clear();
    m_series = series;
    m_markers.clear();
    m_sticks.clear();
    m_options.xTitle = xLabel;
    m_options.yTitle = yLabel;
    m_selectedPoint = -1;
    m_xLabel = xLabel;
    m_yLabel = yLabel;
    m_barSeries = false;
    update();
}

void PlotWidget::setScanSeries(const QVector<QPointF> &series, const QString &xLabel, const QString &yLabel) {
    setSeries(series, xLabel, yLabel);
    m_scan = true;
    auto options = m_options;
    options.xTitle = xLabel;
    options.yTitle = yLabel;
    setStyleOptions(options);
    // Interior scan peaks are approximate TS candidates; endpoints are excluded.
    QVector<int> order;
    for (int i = 0; i < series.size(); ++i)
        if (std::isfinite(series[i].x()) && std::isfinite(series[i].y())) order.push_back(i);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return series[a].x() < series[b].x(); });
    for (int i = 1; i + 1 < order.size(); ++i) {
        const auto &p = series[order[i]];
        const auto &left = series[order[i - 1]], &right = series[order[i + 1]];
        if (left.x() >= p.x() || p.x() >= right.x()) continue;
        if (p.y() >= left.y() && p.y() >= right.y() && (p.y() > left.y() || p.y() > right.y())
            && (m_approximateTsPoint < 0 || p.y() > series[m_approximateTsPoint].y()))
            m_approximateTsPoint = order[i];
    }
    update();
}

QVector<double> PlotWidget::axisTicks(bool xAxis) const {
    const auto a = axisRange();
    const double lo = xAxis ? a.xMin : a.yMin, hi = xAxis ? a.xMax : a.yMax;
    const auto r = plotRect();
    const QFontMetricsF metrics(m_options.tickFont);
    const double labelExtent = xAxis
        ? std::max(metrics.horizontalAdvance(axisLabel(lo, true)), metrics.horizontalAdvance(axisLabel(hi, true))) + 20
        : metrics.height() + 15;
    const int count = std::clamp(int((xAxis ? r.width() : r.height()) / labelExtent) + 1, 2, m_options.majorTicks);
    const double custom = xAxis ? m_options.xIncrement : m_options.yIncrement;
    double step = custom > 0 ? custom : (m_options.customIncrement ? m_options.increment : niceStep((hi - lo) / (count - 1)));
    if (!std::isfinite(step) || step <= 0) step = niceStep((hi - lo) / (count - 1));
    if ((hi-lo)/step > 199) step *= std::ceil((hi-lo)/step/199);
    const double start = std::ceil(lo / step - 1e-10) * step;
    QVector<double> values;
    for (int i = 0; i < 200; ++i) {
        const double v = start + i * step;
        if (!std::isfinite(v) || v > hi + step * 1e-9) break;
        if (v >= lo - step * 1e-9) values.push_back(std::abs(v) < step * 1e-9 ? 0.0 : v);
    }
    return values;
}

QString PlotWidget::axisLabel(double value, bool xAxis) const {
    const auto a = axisRange();
    const double span = xAxis ? a.xMax - a.xMin : a.yMax - a.yMin;
    const double custom = xAxis ? m_options.xIncrement : m_options.yIncrement;
    const double step = custom > 0 ? custom : (m_options.customIncrement ? m_options.increment : niceStep(span / (m_options.majorTicks - 1)));
    if (std::abs(value) < step * 1e-8) value = 0;
    if (std::abs(value) >= 1e5 || (step < 1e-4 && value != 0)) return QString::number(value, 'g', 4);
    int decimals = m_scan && xAxis ? 2 : 0;
    for (int d = 0; d <= 8; ++d) {
        if (std::abs(step * std::pow(10.0, d) - std::round(step * std::pow(10.0, d))) < 1e-6) {
            decimals = std::max(decimals, d); break;
        }
    }
    return QString::number(value, 'f', std::min(decimals, 8));
}

void PlotWidget::paintApproximateTs(QPainter &painter, const QRectF &r) {
    if (!m_options.showTsAnnotation || m_approximateTsPoint < 0 || m_approximateTsPoint >= m_series.size()) return;
    const QPointF point = toScreen(m_series[m_approximateTsPoint], r);
    if (!r.contains(point)) return;
    painter.save();
    const QColor accent(190, 38, 45);
    painter.setFont(m_options.tickFont);
    const QFontMetricsF metrics(painter.font());
    const QString label = m_options.tsLabel;
    const qreal w = metrics.horizontalAdvance(label) + 12, h = metrics.height() + 6;
    const qreal left = std::clamp(point.x() - w / 2, r.left(), std::max(r.left(), r.right() - w));
    const QRectF box(left, std::max(3.0, r.top() - h - 7), w, h);
    painter.setPen(QPen(accent, 0.9));
    painter.drawLine(QPointF(point.x(), box.bottom()+2), point - QPointF(0,7));
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_transparentPaint ? QColor(255,255,255,235) : m_options.background);
    painter.drawRect(box);
    painter.setPen(accent);
    painter.drawText(box, Qt::AlignCenter, label);
    painter.setPen(QPen(accent, 1.0));
    painter.setBrush(Qt::white);
    const qreal d = 4.5;
    painter.drawPolygon(QPolygonF{point+QPointF(0,-d), point+QPointF(d,0), point+QPointF(0,d), point+QPointF(-d,0)});
    painter.restore();
}

void PlotWidget::setMarkers(const QVector<QPointF> &markers) { m_markers = markers; update(); }

void PlotWidget::setStickSeries(const QVector<QPointF> &sticks) {
    m_sticks = sticks;
    update();
}

void PlotWidget::setPlotStyle(const QColor &lineColor, qreal lineWidth,
                              const QColor &frameColor, const QColor &backgroundColor) {
    m_options.showPoints = false;
    m_options.lineColor = m_options.borderColor = lineColor;
    m_options.thickness = lineWidth;
    m_options.axisColor = m_options.tickColor = frameColor;
    m_options.background = backgroundColor;
    m_lineColor = lineColor;
    m_lineWidth = lineWidth;
    m_frameColor = frameColor;
    m_backgroundColor = backgroundColor;
    update();
}

void PlotWidget::setBarSeries(const QVector<QPointF> &series) {
    setSeries(series, m_xLabel, m_yLabel);
    m_barSeries = true;
    update();
}

void PlotWidget::setEmptyMessage(const QString &message) {
    m_emptyMessage = message;
    update();
}

QImage PlotWidget::image(bool transparentBackground) {
    const qreal ratio = devicePixelRatioF();
    QImage result(QSize(qRound(canvasSize().width() * ratio), qRound(canvasSize().height() * ratio)),
                  QImage::Format_ARGB32_Premultiplied);
    result.setDevicePixelRatio(ratio);
    result.fill(Qt::transparent);
    const bool previous = m_transparentPaint;
    m_transparentPaint = transparentBackground;
    QPainter painter(&result);
    render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    m_transparentPaint = previous;
    update();
    return result;
}

void PlotWidget::clear() {
    m_selectedPoint = -1;
    m_approximateTsPoint = -1;
    m_series.clear();
    m_markers.clear();
    m_sticks.clear();
    m_heatmapPoints.clear();
    m_heatmapXValues.clear();
    m_heatmapYValues.clear();
    m_heatmap = false;
    update();
}

void PlotWidget::setSelectedPoint(int index) {
    if (m_heatmap) {
        const auto found = std::find_if(m_heatmapPoints.cbegin(), m_heatmapPoints.cend(),
                                        [index](const HeatmapPoint &point) { return point.sourceIndex == index; });
        m_selectedPoint = found == m_heatmapPoints.cend() ? -1 : index;
    } else {
        m_selectedPoint = index >= 0 && index < m_series.size() ? index : -1;
    }
    update();
}

QRectF PlotWidget::plotRect() const {
    const QFontMetrics ticks(m_options.tickFont), axes(m_options.axisFont);
    const auto range = axisRange();
    const int tickWidth = std::max(ticks.horizontalAdvance(axisLabel(range.yMin, false)), ticks.horizontalAdvance(axisLabel(range.yMax, false)));
    const int left = m_options.showY ? 20 + axes.height() + (m_options.tickLabels ? tickWidth : 0) : 24;
    const int stageSpace = m_options.showMechanism && !m_heatmap ? 2 * QFontMetrics(m_options.mechanismFont).height() + qRound(std::clamp(m_options.mechanismOffset,10.0,100.0)) : 0;
    const int bottom = m_options.showX ? 20 + axes.height() + (m_options.tickLabels ? ticks.height() : 0) + stageSpace : 24;
    int top = m_options.title.isEmpty() ? 18 : QFontMetrics(m_options.titleFont).height() + 24;
    if (m_options.showTsAnnotation && m_approximateTsPoint >= 0) top += ticks.height() + 18;
    const int xTickWidth = std::max(ticks.horizontalAdvance(axisLabel(range.xMin)), ticks.horizontalAdvance(axisLabel(range.xMax)));
    const int right = m_heatmap ? 110 : std::max(24, xTickWidth / 2 + 8);
    return QRectF(left, top, std::max(1, canvasSize().width() - left - right), std::max(1, canvasSize().height() - top - bottom));
}

QPointF PlotWidget::toScreen(const QPointF &point, const QRectF &rect) const {
    if (m_styled || m_heatmap) {
        const auto a = axisRange();
        return {rect.left() + (point.x() - a.xMin) / (a.xMax - a.xMin) * rect.width(),
                rect.bottom() - (point.y() - a.yMin) / (a.yMax - a.yMin) * rect.height()};
    }
    const QVector<QPointF> &basis = m_series.isEmpty() ? m_sticks : m_series;
    if (basis.isEmpty()) return rect.center();
    double minX = basis.first().x(), maxX = minX;
    double minY = m_sticks.isEmpty() ? basis.first().y() : 0.0;
    double maxY = basis.first().y();
    for (const QPointF &value : basis) {
        minX = std::min(minX, value.x()); maxX = std::max(maxX, value.x());
        minY = std::min(minY, value.y()); maxY = std::max(maxY, value.y());
    }
    for (const QPointF &value : m_sticks) {
        minX = std::min(minX, value.x()); maxX = std::max(maxX, value.x());
        maxY = std::max(maxY, value.y());
    }
    if (qFuzzyCompare(minX, maxX)) { minX -= 1.0; maxX += 1.0; }
    if (qFuzzyCompare(minY, maxY)) { minY -= 1.0; maxY += 1.0; }
    const double padding = (maxY - minY) * 0.08;
    minY -= padding; maxY += padding;
    return {rect.left() + (point.x() - minX) / (maxX - minX) * rect.width(),
            rect.bottom() - (point.y() - minY) / (maxY - minY) * rect.height()};
}

void PlotWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    paintPlot(painter);
}

void PlotWidget::paintPlot(QPainter &painter) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    if (m_heatmap) paintHeatmap(painter);
    else paintStyledPlot(painter);
}

int PlotWidget::nearestPoint(const QPoint &position) const {
    if (m_heatmap) return heatmapIndexAt(position);
    if (m_series.isEmpty()) return -1;
    const QRectF r = plotRect();
    if (m_styled && !r.contains(position)) return -1;
    int index = -1;
    double distance = 14.0;
    for (int i = 0; i < m_series.size(); ++i) {
        const QPointF screen = toScreen(m_series[i], r);
        if (m_styled && (!std::isfinite(screen.x()) || !std::isfinite(screen.y()) || !r.contains(screen))) continue;
        const double d = QLineF(position, screen).length();
        if (d < distance) { distance = d; index = i; }
    }
    return index;
}

void PlotWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        const int index = nearestPoint(event->pos());
        if (index >= 0) emit pointClicked(m_heatmap ? m_heatmapPoints[index].sourceIndex : index);
    }
    QWidget::mousePressEvent(event);
}

void PlotWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_heatmap) {
        const int index = heatmapIndexAt(event->pos());
        if (index >= 0) {
            const auto &point = m_heatmapPoints[index];
            setToolTip(QStringLiteral("%1 = %2\n%3 = %4\nΔE = %5 kcal/mol\nE = %6 Hartree")
                           .arg(m_xLabel, QString::number(point.x, 'f', 4),
                                m_yLabel, QString::number(point.y, 'f', 4),
                                QString::number(point.z, 'f', 3),
                                QString::number(point.energyHartree, 'f', 9)));
        } else {
            setToolTip({});
        }
    }
    if (!m_heatmap) {
        const int index = nearestPoint(event->pos());
        if (index >= 0) {
            const auto &p = m_series[index];
            QString text = QStringLiteral("%1: %2\n%3: %4").arg(m_xLabel,QString::number(p.x(),'g',8),m_yLabel,QString::number(p.y(),'g',8));
            if (index == m_approximateTsPoint) text += QStringLiteral("\nTS candidate: scan의 내부 극댓값입니다. Saddle point 확정에는 진동수·IRC 검증이 필요합니다.");
            setToolTip(text);
        } else setToolTip({});
    }
    QWidget::mouseMoveEvent(event);
}

} // namespace youngcraft

namespace youngcraft {

namespace {

QVector<double> uniqueCoordinates(const QVector<PlotWidget::HeatmapPoint> &points, bool xAxis) {
    QVector<double> values;
    for (const auto &point : points) values.push_back(xAxis ? point.x : point.y);
    std::sort(values.begin(), values.end());
    QVector<double> unique;
    constexpr double tolerance = 1e-4;
    for (double value : values)
        if (unique.isEmpty() || std::abs(value - unique.last()) > tolerance) unique.push_back(value);
    return unique;
}

QPair<double, double> cellBounds(const QVector<double> &values, int index) {
    if (values.size() == 1) return {values[0] - 0.5, values[0] + 0.5};
    const double lower = index == 0 ? values[0] - (values[1] - values[0]) * 0.5
                                    : (values[index - 1] + values[index]) * 0.5;
    const double upper = index == values.size() - 1 ? values[index] + (values[index] - values[index - 1]) * 0.5
                                                    : (values[index] + values[index + 1]) * 0.5;
    return {lower, upper};
}

QColor heatmapColor(double value) {
    const double t = std::clamp(value, 0.0, 1.0);
    struct Stop { double position; QColor color; };
    const Stop stops[] = {{0.0, QColor(68, 1, 84)}, {0.34, QColor(49, 104, 142)},
                          {0.68, QColor(53, 183, 121)}, {1.0, QColor(253, 231, 37)}};
    for (int i = 1; i < 4; ++i) {
        if (t > stops[i].position) continue;
        const double f = (t - stops[i - 1].position) / (stops[i].position - stops[i - 1].position);
        return QColor::fromRgbF(stops[i - 1].color.redF() + (stops[i].color.redF() - stops[i - 1].color.redF()) * f,
                                stops[i - 1].color.greenF() + (stops[i].color.greenF() - stops[i - 1].color.greenF()) * f,
                                stops[i - 1].color.blueF() + (stops[i].color.blueF() - stops[i - 1].color.blueF()) * f);
    }
    return stops[3].color;
}

} // namespace

void PlotWidget::setHeatmapSeries(const QVector<HeatmapPoint> &points, const QString &xLabel,
                                  const QString &yLabel, const QString &zLabel) {
    if (!m_heatmap) clearManualRange();
    m_scan = true;
    m_approximateTsPoint = -1;
    m_heatmap = true;
    m_heatmapPoints.clear();
    m_series.clear();
    for (const auto &point : points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) continue;
        m_heatmapPoints.push_back(point);
        m_series.push_back({point.x, point.y});
    }
    m_heatmapXValues = uniqueCoordinates(m_heatmapPoints, true);
    m_heatmapYValues = uniqueCoordinates(m_heatmapPoints, false);
    m_selectedPoint = -1;
    m_xLabel = xLabel;
    m_yLabel = yLabel;
    m_options.xTitle = xLabel;
    m_options.yTitle = yLabel;
    m_heatmapZLabel = zLabel;
    m_barSeries = false;
    update();
}

QRectF PlotWidget::heatmapCellRect(int index, const QRectF &rect) const {
    if (index < 0 || index >= m_heatmapPoints.size()) return {};
    const auto &point = m_heatmapPoints[index];
    auto nearestIndex = [](const QVector<double> &values, double value) {
        int nearest = 0;
        for (int i = 1; i < values.size(); ++i)
            if (std::abs(values[i] - value) < std::abs(values[nearest] - value)) nearest = i;
        return nearest;
    };
    const auto xBounds = cellBounds(m_heatmapXValues, nearestIndex(m_heatmapXValues, point.x));
    const auto yBounds = cellBounds(m_heatmapYValues, nearestIndex(m_heatmapYValues, point.y));
    return QRectF(toScreen({xBounds.first, yBounds.second}, rect),
                  toScreen({xBounds.second, yBounds.first}, rect)).normalized();
}

int PlotWidget::heatmapIndexAt(const QPoint &position) const {
    const QRectF r = plotRect();
    for (int i = 0; i < m_heatmapPoints.size(); ++i)
        if (heatmapCellRect(i, r).contains(position)) return i;
    return -1;
}

void PlotWidget::paintHeatmap(QPainter &painter) {
    const QColor background = m_styled ? m_options.background
        : (m_backgroundColor.isValid() ? m_backgroundColor : palette().base().color());
    const QColor axisColor = m_styled ? m_options.axisColor
        : (m_frameColor.isValid() ? m_frameColor : palette().text().color());
    const QColor tickColor = m_styled ? m_options.tickColor : axisColor;
    const QFont tickFont = m_styled ? m_options.tickFont : QFont(QStringLiteral("Times New Roman"), 10, QFont::Bold);
    const QFont axisFont = m_styled ? m_options.axisFont : QFont(QStringLiteral("Times New Roman"), 12, QFont::Bold);
    if (!m_transparentPaint) painter.fillRect(canvasRect(), background);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF r = plotRect();
    if (m_styled && !m_options.title.isEmpty()) {
        painter.setFont(m_options.titleFont);
        painter.setPen(m_options.titleColor);
        painter.drawText(QRectF(8, 4, canvasSize().width() - 16, r.top() - 8), Qt::AlignCenter, m_options.title);
    }
    painter.fillRect(r, QColor(247, 248, 250));
    painter.setPen(QPen(QColor(205, 208, 214), 1));
    for (double x : m_options.majorGrid ? axisTicks(true) : QVector<double>{}) {
        const QPointF screen = toScreen({x, axisRange().yMin}, r);
        painter.drawLine(QPointF(screen.x(), r.top()), QPointF(screen.x(), r.bottom()));
    }
    for (double y : m_options.majorGrid ? axisTicks(false) : QVector<double>{}) {
        const QPointF screen = toScreen({axisRange().xMin, y}, r);
        painter.drawLine(QPointF(r.left(), screen.y()), QPointF(r.right(), screen.y()));
    }
    double zMin = 0.0, zMax = 1.0;
    if (!m_heatmapPoints.isEmpty()) {
        zMin = zMax = m_heatmapPoints.first().z;
        for (const auto &point : m_heatmapPoints) {
            zMin = std::min(zMin, point.z);
            zMax = std::max(zMax, point.z);
        }
    }
    const double zSpan = zMax > zMin ? zMax - zMin : 1.0;
    painter.save();
    painter.setClipRect(r);
    for (int i = 0; i < m_heatmapPoints.size(); ++i) {
        const auto &point = m_heatmapPoints[i];
        const QRectF cell = heatmapCellRect(i, r).intersected(r);
        painter.fillRect(cell, heatmapColor((point.z - zMin) / zSpan));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(m_highlight && point.z >= m_highlightCutoff
                           ? QPen(QColor(205, 30, 30), 2) : QPen(Qt::NoPen));
        painter.drawRect(cell);
        if (!m_publicationPaint && point.sourceIndex == m_selectedPoint) {
            painter.setPen(QPen(Qt::white, 5));
            painter.drawRect(cell.adjusted(2, 2, -2, -2));
            painter.setPen(QPen(Qt::black, 2));
            painter.drawRect(cell.adjusted(2, 2, -2, -2));
        }
    }
    painter.restore();
    painter.setPen(QPen(axisColor, 0.8));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(r);
    painter.setFont(tickFont);
    painter.setPen(tickColor);
    if (!m_styled || m_options.tickLabels) {
        qreal lastX = -1e30;
        for (double x : m_options.showX ? axisTicks(true) : QVector<double>{}) {
            const qreal screenX = toScreen({x, axisRange().yMin}, r).x();
            const qreal labelWidth = QFontMetricsF(tickFont).horizontalAdvance(axisLabel(x)) + 12;
            if (screenX - labelWidth / 2 < lastX) continue;
            lastX = screenX + labelWidth / 2;
            painter.drawText(QRectF(screenX - labelWidth/2, r.bottom() + 5, labelWidth, QFontMetricsF(tickFont).height()+2), Qt::AlignHCenter | Qt::AlignTop,
                             axisLabel(x));
        }
        qreal lastY = 1e30;
        for (double y : m_options.showY ? axisTicks(false) : QVector<double>{}) {
            const qreal screenY = toScreen({axisRange().xMin, y}, r).y();
            if (lastY - screenY < QFontMetricsF(tickFont).height() + 6) continue;
            lastY = screenY;
            painter.drawText(QRectF(4, screenY - QFontMetricsF(tickFont).height()/2, r.left() - 12, QFontMetricsF(tickFont).height()+2), Qt::AlignRight | Qt::AlignVCenter,
                             axisLabel(y, false));
        }
    }
    painter.setFont(axisFont);
    painter.setPen(axisColor);
    const QString xTitle = m_styled ? m_options.xTitle : m_xLabel;
    const QString yTitle = m_styled ? m_options.yTitle : m_yLabel;
    if (m_options.showX) painter.drawText(QRectF(r.left(), r.bottom() + 10 + QFontMetricsF(tickFont).height(), r.width(), QFontMetricsF(axisFont).height()+4), Qt::AlignCenter, xTitle);
    painter.save();
    painter.translate(16, r.center().y());
    painter.rotate(-90);
    if (m_options.showY) painter.drawText(QRectF(-r.height() / 2, -14, r.height(), 28), Qt::AlignCenter, yTitle);
    painter.restore();

    const QRectF colorBar(r.right() + 34, r.top() + 42, 18, std::max(20.0, r.height() - 84));
    QLinearGradient gradient(colorBar.bottomLeft(), colorBar.topLeft());
    for (int i = 0; i <= 20; ++i) gradient.setColorAt(i / 20.0, heatmapColor(i / 20.0));
    painter.fillRect(colorBar, gradient);
    painter.setPen(QPen(axisColor, 1));
    painter.drawRect(colorBar);
    painter.setFont(tickFont);
    painter.drawText(QRectF(colorBar.right() + 5, colorBar.top() - 8, 52, 20), Qt::AlignLeft | Qt::AlignVCenter,
                     QString::number(zMax, 'g', 4));
    painter.drawText(QRectF(colorBar.right() + 5, colorBar.bottom() - 12, 52, 20), Qt::AlignLeft | Qt::AlignVCenter,
                     QString::number(zMin, 'g', 4));
    painter.drawText(QRectF(r.right() + 8, r.top(), canvasSize().width() - r.right() - 12, 38),
                     Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_heatmapZLabel);
    if (m_heatmapPoints.isEmpty()) {
        painter.setPen(tickColor);
        painter.drawText(r, Qt::AlignCenter, m_emptyMessage);
    }
}

void PlotWidget::setStyleOptions(const StyleOptions &options) {
    m_styled = true;
    m_options = options;
    if (m_options.tickFontFollowsAxis) m_options.tickFont = m_options.axisFont;
    m_options.majorTicks = std::clamp(options.majorTicks, 2, 20);
    m_options.minorTicks = std::clamp(options.minorTicks, 0, 10);
    update();
}

PlotWidget::AxisRange PlotWidget::autoRange() const {
    AxisRange a;
    if (m_heatmap && !m_heatmapXValues.isEmpty() && !m_heatmapYValues.isEmpty()) {
        const auto xFirst = cellBounds(m_heatmapXValues, 0);
        const auto xLast = cellBounds(m_heatmapXValues, m_heatmapXValues.size() - 1);
        const auto yFirst = cellBounds(m_heatmapYValues, 0);
        const auto yLast = cellBounds(m_heatmapYValues, m_heatmapYValues.size() - 1);
        return {xFirst.first, xLast.second, yFirst.first, yLast.second};
    }
    bool first = true;
    QVector<QPointF> basis = m_series;
    basis += m_sticks;
    if (!m_sticks.isEmpty() || m_barSeries) {
        for (const auto &p : m_sticks.isEmpty() ? m_series : m_sticks) basis.push_back({p.x(), 0});
    }
    for (const auto &p : basis) {
        if (!std::isfinite(p.x()) || !std::isfinite(p.y())) continue;
        if (first) { a = {p.x(), p.x(), p.y(), p.y()}; first = false; }
        else {
            a.xMin = std::min(a.xMin, p.x()); a.xMax = std::max(a.xMax, p.x());
            a.yMin = std::min(a.yMin, p.y()); a.yMax = std::max(a.yMax, p.y());
        }
    }
    if (first) return a;
    const double px = a.xMax > a.xMin ? (a.xMax - a.xMin) * 0.05 : std::max(1.0, std::abs(a.xMin) * 0.05);
    const double py = a.yMax > a.yMin ? (a.yMax - a.yMin) * 0.08 : std::max(1.0, std::abs(a.yMin) * 0.08);
    return {m_options.nonnegativeX ? std::max(0.0,a.xMin-px) : a.xMin-px, a.xMax + px, m_options.zeroBaseline && a.yMin >= 0 ? 0 : a.yMin - py, a.yMax + py};
}

PlotWidget::AxisRange PlotWidget::axisRange() const { return m_manualRange.value_or(autoRange()); }

bool PlotWidget::setManualRange(const AxisRange &a) {
    if (!std::isfinite(a.xMin) || !std::isfinite(a.xMax) || !std::isfinite(a.yMin) || !std::isfinite(a.yMax)
        || a.xMin >= a.xMax || a.yMin >= a.yMax
        || !std::isfinite(a.xMax - a.xMin) || !std::isfinite(a.yMax - a.yMin)) return false;
    m_manualRange = a;
    update();
    return true;
}

void PlotWidget::clearManualRange() { m_manualRange.reset(); update(); }
void PlotWidget::setHighlightRule(bool enabled, double cutoff) {
    m_highlight = enabled && std::isfinite(cutoff);
    m_highlightCutoff = cutoff;
    update();
}

QVector<PlotWidget::MechanismStage> PlotWidget::defaultMechanismStages(double minimum, double maximum) {
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum
        || !std::isfinite(maximum - minimum)) return {};
    const double step = (maximum - minimum) / 3.0;
    return {{QStringLiteral("Reactant"), minimum, minimum + step},
            {QStringLiteral("Transition state"), minimum + step, minimum + 2 * step},
            {QStringLiteral("Product"), minimum + 2 * step, maximum}};
}

void PlotWidget::paintMechanism(QPainter &painter, const QRectF &r) {
    const auto &s = m_options;
    if (!s.showMechanism || !s.showX || m_heatmap) return;
    const auto a = axisRange();
    painter.save();
    painter.setPen(QPen(s.mechanismColor, 0.8));
    painter.setBrush(Qt::NoBrush);
    const qreal y = r.bottom() + (s.tickLabels ? QFontMetricsF(s.tickFont).height() : 0) + 12 + std::clamp(s.mechanismOffset, 10.0, 100.0);
    for (const auto &stage : s.mechanismStages) {
        if (stage.text.trimmed().isEmpty() || !std::isfinite(stage.start) || !std::isfinite(stage.end)
            || stage.start >= stage.end || stage.end <= a.xMin || stage.start >= a.xMax) continue;
        const qreal left = toScreen({std::max(stage.start, a.xMin), a.yMin}, r).x() + 2;
        const qreal right = toScreen({std::min(stage.end, a.xMax), a.yMin}, r).x() - 2;
        const qreal span = right - left;
        if (span < 30) continue;
        const QFont font = s.mechanismFont;
        const QFontMetricsF metrics(font);
        QString text = stage.text;
        qreal textWidth = metrics.horizontalAdvance(text);
        if (textWidth > span - 16) {
            // Wrap at a word boundary instead of truncating a scientific label.
            const auto words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            qreal bestWidth = textWidth;
            for (int i=1;i<words.size();++i) {
                const auto first=words.mid(0,i).join(QLatin1Char(' '));
                const auto second=words.mid(i).join(QLatin1Char(' '));
                const qreal candidate=std::max(metrics.horizontalAdvance(first),metrics.horizontalAdvance(second));
                if (candidate < bestWidth) { bestWidth=candidate; text=first+QLatin1Char('\n')+second; }
            }
            textWidth=bestWidth;
        }
        const qreal center = (left + right) / 2;
        const qreal halfText = std::min(span/2-6, textWidth/2+5);
        const qreal textHeight=metrics.height()*(text.contains(QLatin1Char('\n'))?2:1);
        painter.setFont(font);
        painter.drawText(QRectF(center - textWidth/2, y - textHeight/2,
                               textWidth, textHeight), Qt::AlignCenter, text);
        // The shaft stops on either side of the text; arrowheads point outward.
        painter.drawLine(QPointF(left, y), QPointF(center - halfText, y));
        painter.drawLine(QPointF(center + halfText, y), QPointF(right, y));
        painter.drawLine(QPointF(left, y), QPointF(left + 5, y - 4));
        painter.drawLine(QPointF(left, y), QPointF(left + 5, y + 4));
        painter.drawLine(QPointF(right, y), QPointF(right - 5, y - 4));
        painter.drawLine(QPointF(right, y), QPointF(right - 5, y + 4));
    }
    painter.restore();
}

void PlotWidget::paintStyledPlot(QPainter &painter) {
    const auto &s = m_options;
    if (!m_transparentPaint) painter.fillRect(canvasRect(), s.background);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF r = plotRect();
    const auto a = axisRange();
    painter.setFont(s.titleFont);
    painter.setPen(s.titleColor);
    painter.drawText(QRectF(8, 4, canvasSize().width() - 16, r.top() - 8), Qt::AlignCenter, s.title);
    auto drawAxis = [&](bool xAxis, const QVector<double> &values) {
        painter.setFont(s.tickFont);
        const qreal h = QFontMetrics(s.tickFont).height();
        auto coordinate = [&](double v) {
            return xAxis ? r.left() + (v - a.xMin) / (a.xMax - a.xMin) * r.width()
                         : r.bottom() - (v - a.yMin) / (a.yMax - a.yMin) * r.height();
        };
        auto grid = [&](qreal c, bool major) {
            painter.setPen(QPen(major ? QColor(222,226,230) : QColor(240,242,244), 0.7, Qt::SolidLine));
            if (xAxis) painter.drawLine(QPointF(c,r.top()), QPointF(c,r.bottom()));
            else painter.drawLine(QPointF(r.left(),c), QPointF(r.right(),c));
        };
        qreal lastLabel = xAxis ? -1e30 : 1e30;
        QString previousLabel;
        for (int i = 0; i < values.size(); ++i) {
            const qreal c = coordinate(values[i]);
            if (s.majorGrid) grid(c, true);
            painter.setPen(QPen(s.tickColor, 0.8));
            if (xAxis) painter.drawLine(QPointF(c,r.bottom()), QPointF(c,r.bottom()-4));
            else painter.drawLine(QPointF(r.left()+4,c), QPointF(r.left(),c));
            if (s.tickLabels) {
                const QString label = axisLabel(values[i], xAxis);
                const qreal labelWidth = QFontMetricsF(s.tickFont).horizontalAdvance(label) + 12;
                const bool fits = (label != previousLabel && (xAxis ? c - labelWidth / 2 >= lastLabel : lastLabel - c >= h + 6));
                if (fits) {
                    if (xAxis) painter.drawText(QRectF(c-labelWidth/2,r.bottom()+6,labelWidth,h+2), Qt::AlignHCenter|Qt::AlignTop,label);
                    else painter.drawText(QRectF(4,c-h/2,r.left()-12,h+2),Qt::AlignRight|Qt::AlignVCenter,label);
                    lastLabel = xAxis ? c + labelWidth / 2 : c;
                    previousLabel = label;
                }
            }
            if (i + 1 == values.size()) continue;
            for (int j = 1; j <= s.minorTicks; ++j) {
                const qreal minor = coordinate(values[i] + (values[i+1]-values[i])*j/(s.minorTicks+1));
                if (s.minorGrid) grid(minor, false);
                painter.setPen(QPen(s.tickColor, 0.8));
                if (xAxis) painter.drawLine(QPointF(minor,r.bottom()),QPointF(minor,r.bottom()-2));
                else painter.drawLine(QPointF(r.left()+2,minor),QPointF(r.left(),minor));
            }
        }
        painter.setPen(QPen(s.axisColor, s.axisThickness));
        painter.setFont(s.axisFont);
        if (xAxis) {
            painter.drawLine(r.bottomLeft(),r.bottomRight());
            painter.drawText(QRectF(r.left(),r.bottom()+10+(s.tickLabels?h:0)+(s.showMechanism ? 2*QFontMetrics(s.mechanismFont).height()+qRound(std::clamp(s.mechanismOffset,10.0,100.0)) : 0),r.width(),QFontMetrics(s.axisFont).height()+4), Qt::AlignCenter,s.xTitle);
        } else {
            painter.drawLine(r.bottomLeft(),r.topLeft());
            painter.save();
            const QFontMetricsF metrics(s.axisFont);
            const qreal length=metrics.horizontalAdvance(s.yTitle)+4;
            const qreal fit=std::min(1.0,std::max(1.0,canvasSize().height()-12.0)/std::max(1.0,length));
            const qreal half=length*fit/2+6;
            const qreal center=std::clamp(r.center().y(),half,std::max(half,canvasSize().height()-half));
            painter.translate(metrics.height()/2.0+2,center);
            painter.rotate(-90);
            painter.scale(fit,fit);
            painter.drawText(QRectF(-length/2,-metrics.height()/2,length,metrics.height()+4),Qt::AlignCenter|Qt::TextDontClip,s.yTitle);
            painter.restore();
        }
    };
    if (s.showX) drawAxis(true,axisTicks(true));
    if (s.showY) drawAxis(false,axisTicks(false));
    painter.save();
    painter.setClipRect(r);
    auto valid = [](const QPointF &p) { return std::isfinite(p.x()) && std::isfinite(p.y()); };
    if (s.showLine && !m_barSeries) {
        for (int i = 1; i < m_series.size(); ++i) {
            QPointF p = m_series[i-1], q = m_series[i];
            if (!valid(p) || !valid(q)) continue;
            painter.setPen(QPen(s.lineColor,s.thickness,s.lineStyle));
            painter.drawLine(toScreen(p,r),toScreen(q,r));
            if (!m_highlight || (p.y() < m_highlightCutoff && q.y() < m_highlightCutoff)) continue;
            if ((p.y() < m_highlightCutoff) != (q.y() < m_highlightCutoff)) {
                const double t = (m_highlightCutoff-p.y())/(q.y()-p.y());
                const QPointF crossing = p+(q-p)*t;
                if (p.y() < m_highlightCutoff) p=crossing; else q=crossing;
            }
            painter.setPen(QPen(QColor(190,38,45),s.thickness,s.lineStyle));
            painter.drawLine(toScreen(p,r),toScreen(q,r));
        }
    }
    // Spectral sticks and bars share the same measured zero baseline.
    painter.setPen(QPen(s.lineColor, s.thickness, Qt::SolidLine, Qt::FlatCap));
    if (m_barSeries) for (const auto &p : m_series) {
        if (valid(p)) painter.drawLine(toScreen({p.x(),0},r), toScreen(p,r));
    }
    painter.setPen(QPen(s.stickColor.isValid() ? s.stickColor : s.lineColor,
                       s.stickThickness > 0 ? s.stickThickness : s.thickness, Qt::SolidLine, Qt::FlatCap));
    for (const auto &p : m_sticks) {
        if (valid(p)) painter.drawLine(toScreen({p.x(),0},r), toScreen(p,r));
    }
    if (s.showPoints && !m_barSeries) for (const auto &p : m_series) {
        if (!valid(p)) continue;
        painter.setPen(s.pointBorder ? QPen(s.borderColor,1) : QPen(Qt::NoPen));
        painter.setBrush(m_highlight && p.y() >= m_highlightCutoff ? QColor(190,38,45) : s.pointColor);
        painter.drawEllipse(toScreen(p,r),s.pointSize/2,s.pointSize/2);
    }
    if (!m_publicationPaint && m_selectedPoint >= 0 && m_selectedPoint < m_series.size() && valid(m_series[m_selectedPoint])) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(50,55,60),1.0,Qt::DashLine));
        painter.drawEllipse(toScreen(m_series[m_selectedPoint],r),s.pointSize/2+4,s.pointSize/2+4);
    }
    painter.restore();
    paintMechanism(painter, r);
    paintApproximateTs(painter, r);
    if (std::none_of(m_series.begin(),m_series.end(),valid) && std::none_of(m_sticks.begin(),m_sticks.end(),valid)) {
        painter.setPen(QPen(s.tickColor, 0.8));
        painter.drawText(r,Qt::AlignCenter,m_emptyMessage);
    }
}

} // namespace youngcraft
