#pragma once
#include "model/Molecule.h"
#include <QSet>
#include <QByteArray>
#include <QStringList>
#include <QJsonObject>

namespace youngcraft {
struct MechanismStep {
    QString name, path;
    Molecule molecule;
    QVector<QVector<QVector3D>> frames;
    QVector<int> mapping; // Empty: ordered automatic match. -1: new track; otherwise existing track.
    double seconds = 3.0; // Travel time from the preceding step, including this step's path frames.
    bool reversed = false;
    QJsonObject presentation; // Optional reaction-coordinate artwork, energy and annotations.
    static MechanismStep fromCalculation(const CalculationData &data, bool usePath = true);
};
struct MechanismOptions {
    bool align = true;
    bool preserveBonds = true;
    bool keepDeparted = true;
    float lift = 6.0f;
};

// Presentation interpolation of supplied structures, never a molecular dynamics calculation.
class MechanismTimeline {
public:
    bool build(const QVector<MechanismStep> &steps, const MechanismOptions &options, QString *error = nullptr);
    Molecule sample(double seconds) const;
    Molecule envelope() const;
    double duration() const { return m_nodes.isEmpty() ? 0 : m_nodes.last().time; }
    int trackCount() const { return m_tracks.size(); }
    const QVector<QVector<int>> &mappings() const { return m_mappings; }
    QStringList summaries() const { return m_summaries; }
    QString caption(double seconds) const;
    void emphasize(const QSet<int> &keep, int mode); // 0 hide others; 1 dim others; 2 restore.
    void park(const QSet<int> &tracks, double start, double seconds = 1.2);
    static QByteArray saveProject(const QVector<MechanismStep> &steps, const MechanismOptions &options);
    static bool loadProject(const QByteArray &json, QVector<MechanismStep> &steps, MechanismOptions &options, QString *error = nullptr);
private:
    struct Node {
        double time = 0;
        int step = 0;
        QVector<QVector3D> positions;
        QVector<float> presence;
        QVector<Bond> bonds;
    };
    QVector<Node> m_nodes;
    QVector<Atom> m_tracks;
    QVector<QVector<int>> m_mappings;
    QStringList m_names, m_summaries;
    QVector<bool> m_visible;
    QVector<float> m_opacity;
    MechanismOptions m_options;
    QSet<int> m_parked;
    double m_parkStart=0,m_parkEnd=1;
};
}
