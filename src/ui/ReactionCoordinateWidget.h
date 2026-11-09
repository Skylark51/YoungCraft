#pragma once
#include "animation/MechanismTimeline.h"
#include <QWidget>
#include <QJsonObject>

class QPainter;
class SmokeTest;
namespace youngcraft {
// Optional presentation layer; all energies and annotations are supplied by the user.
class ReactionCoordinateWidget final : public QWidget {
    Q_OBJECT
    friend class ::SmokeTest;
public:
    explicit ReactionCoordinateWidget(QVector<MechanismStep> *steps, QWidget *parent = nullptr);
    bool ready() const;
    bool hasImages() const;
    bool hasEnergies() const;
    bool active() const { return m_enabled && ready(); }
    void setEnabledPresentation(bool enabled);
    void refresh();
    void setTime(double seconds);
    double mappedTime(double seconds) const;
    double timeForMapped(double seconds) const;
    float impactFlash(double seconds) const;
    void edit(int selectedStage=0);
    QImage imageAtTime(QSize size, double seconds, bool transparent=false, bool force=false) const;
    void setSeparateArtwork(bool separate);
    void setTextStyle(const QFont &font, const QColor &color, bool showCaption);
    void drawArtwork(QPainter &painter, const QRectF &bounds, double seconds, bool transparent) const;
    QJsonObject settings() const;
    void restoreSettings(const QJsonObject &settings);
signals:
    void configurationChanged();
protected:
    void paintEvent(QPaintEvent *) override;
private:
    struct Position { int from=0,to=0; double fraction=0,time=0,impact=0; bool collision=false; };
    Position position(double seconds) const;
    QVector<double> stageTimes() const;
    struct Artwork {
        int from=0,to=0,ts=-1;
        double fraction=0,annotationFraction=-1;
    };
    Artwork artworkAt(double seconds, const Position &position) const;
    void drawArtwork(QPainter &painter, const QRectF &bounds, const Artwork &artwork) const;
    void draw(QPainter &painter, const QRectF &rect, double seconds, bool transparent=false, bool force=false) const;
    QVector<MechanismStep> *m_steps;
    QVector<QImage> m_images;
    QFont m_textFont{QStringLiteral("Arial"), 10, QFont::Bold};
    QColor m_textColor{35, 45, 55};
    bool m_showCaption=true;
    bool m_enabled=false;
    bool m_separateArtwork=false;
    bool m_grid=true,m_values=false;
    int m_reference=0;
    double m_time=0;
};
}
