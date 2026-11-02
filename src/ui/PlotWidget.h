#pragma once

#include <QColor>
#include <QImage>
#include <QFont>
#include <optional>
#include <QPointF>
#include <QVector>
#include <QWidget>

namespace youngcraft {

class PlotWidget final : public QWidget {
    Q_OBJECT
public:
    struct MechanismStage {
        QString text;
        double start = 0, end = 1;
    };
    struct StyleOptions {
        QString title, xTitle, yTitle;
        QColor background = Qt::white, lineColor = QColor(0, 92, 138);
        QColor pointColor = Qt::white, borderColor = QColor(0, 92, 138);
        QColor titleColor = Qt::black, axisColor = Qt::black, tickColor = Qt::black;
        QColor stickColor;
        qreal stickThickness = 0;
        bool zeroBaseline = false, nonnegativeX = false;
        qreal axisThickness = 0.8;
        QFont titleFont = QFont(QStringLiteral("Arial"), 11, QFont::Bold);
        QFont axisFont = QFont(QStringLiteral("Arial"), 10);
        QFont tickFont = QFont(QStringLiteral("Arial"), 9);
        bool tickFontFollowsAxis = false;
        bool showLine = true, showPoints = true, pointBorder = true;
        bool showX = true, showY = true, tickLabels = true;
        bool majorGrid = false, minorGrid = false, customIncrement = false;
        Qt::PenStyle lineStyle = Qt::SolidLine;
        qreal thickness = 1.6, pointSize = 5.0;
        int majorTicks = 6, minorTicks = 1;
        double increment = 1.0;
        double xIncrement = 0.0, yIncrement = 0.0;
        bool showTsAnnotation = true;
        QString tsLabel = QStringLiteral("TS candidate");
        bool showMechanism = false;
        QVector<MechanismStage> mechanismStages;
        QFont mechanismFont = QFont(QStringLiteral("Arial"), 8);
        QColor mechanismColor = Qt::black;
        qreal mechanismOffset = 22.0;
    };
    static QVector<MechanismStage> defaultMechanismStages(double minimum, double maximum);
    struct AxisRange { double xMin = 0, xMax = 1, yMin = 0, yMax = 1; };
    struct HeatmapPoint {
        double x = 0;
        double y = 0;
        double z = 0;
        int sourceIndex = -1;
        double energyHartree = 0;
    };
    void setStyleOptions(const StyleOptions &options);
    const StyleOptions &styleOptions() const { return m_options; }
    AxisRange autoRange() const;
    AxisRange axisRange() const;
    bool setManualRange(const AxisRange &range);
    void clearManualRange();
    void setHighlightRule(bool enabled, double cutoff);

    explicit PlotWidget(QWidget *parent = nullptr);
    void setSeries(const QVector<QPointF> &series, const QString &xLabel = {}, const QString &yLabel = {});
    void setScanSeries(const QVector<QPointF> &series, const QString &xLabel, const QString &yLabel);
    int approximateTsPoint() const { return m_approximateTsPoint; }
    void setHeatmapSeries(const QVector<HeatmapPoint> &points, const QString &xLabel,
                          const QString &yLabel, const QString &zLabel);
    void setMarkers(const QVector<QPointF> &markers);
    void setStickSeries(const QVector<QPointF> &sticks);
    void setPlotStyle(const QColor &lineColor, qreal lineWidth,
                      const QColor &frameColor, const QColor &backgroundColor);
    void setBarSeries(const QVector<QPointF> &series);
    void setEmptyMessage(const QString &message);
    QImage image(bool transparentBackground = false);
    struct ExportOptions {
        double widthMm = 84.6, heightMm = 64.0;
        int dpi = 600;
        bool transparent = false;
    };
    QImage publicationImage(const ExportOptions &options);
    bool savePublication(const QString &path, const ExportOptions &options, QString *error = nullptr);
    void showExportDialog();
    void showAxisSettings();
    QVector<double> axisTicks(bool xAxis) const;
    QRectF plotArea() const { return plotRect(); }
    void clear();
    void setSelectedPoint(int index);
    int selectedPoint() const { return m_selectedPoint; }
    const QVector<QPointF> &series() const { return m_series; }

signals:
    void pointClicked(int index);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void paintStyledPlot(QPainter &painter);
    void paintPlot(QPainter &painter);
    QSize canvasSize() const { return m_exportSize.isValid() ? m_exportSize : size(); }
    QRect canvasRect() const { return QRect(QPoint(), canvasSize()); }
    QSize m_exportSize;
    bool m_publicationPaint = false;
    void paintMechanism(QPainter &painter, const QRectF &rect);
    void paintHeatmap(QPainter &painter);
    QRectF heatmapCellRect(int index, const QRectF &rect) const;
    int heatmapIndexAt(const QPoint &position) const;
    QString axisLabel(double value, bool xAxis = true) const;
    void paintApproximateTs(QPainter &painter, const QRectF &rect);
    bool m_scan = false;
    int m_approximateTsPoint = -1;
    bool m_styled = true;
    StyleOptions m_options;
    std::optional<AxisRange> m_manualRange;
    bool m_highlight = false;
    double m_highlightCutoff = 0;
    QRectF plotRect() const;
    QPointF toScreen(const QPointF &point, const QRectF &rect) const;
    int nearestPoint(const QPoint &position) const;

    QVector<QPointF> m_series;
    QVector<HeatmapPoint> m_heatmapPoints;
    QVector<double> m_heatmapXValues;
    QVector<double> m_heatmapYValues;
    QString m_heatmapZLabel;
    bool m_heatmap = false;
    int m_selectedPoint = -1;
    QVector<QPointF> m_markers;
    QVector<QPointF> m_sticks;
    QString m_xLabel;
    QString m_yLabel;
    QString m_emptyMessage = QStringLiteral("표시할 데이터가 없습니다");
    QColor m_lineColor = QColor(45, 128, 220);
    QColor m_frameColor;
    QColor m_backgroundColor;
    qreal m_lineWidth = 2.0;
    bool m_transparentPaint = false;
    bool m_barSeries = false;
};

} // namespace youngcraft
