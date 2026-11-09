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
    double travelSeconds() const;
    double holdSeconds() const;
    QJsonObject presentation; // Optional reaction-coordinate artwork, energy and annotations.
    static MechanismStep fromCalculation(const CalculationData &data, bool usePath = true);
};
struct MechanismOptions {
    bool align = true;
    bool preserveBonds = true;
    bool keepDeparted = true;
    float lift = 6.0f;
    float departedOpacity = 0.18f;
    float parkedOpacity = 0.35f;
    float dimmedOpacity = 0.20f;
};

// Presentation interpolation of supplied structures, never a molecular dynamics calculation.
class MechanismTimeline {
public:
    bool build(const QVector<MechanismStep> &steps, const MechanismOptions &options, QString *error = nullptr);
    Molecule sample(double seconds) const;
    Molecule envelope() const;
    QVector<QPair<int,int>> cleavableBonds(int stage) const;
    QVector<double> cleavageTimes(int stage, int first, int second) const;
    double duration() const { return m_nodes.isEmpty() ? 0 : m_nodes.last().time; }
    const QVector<double> &topologyChangeTimes() const { return m_topologyChanges; }
    double stageTime(int stage) const { return m_nodes[m_stageEnds[stage]].time; }
    int trackCount() const { return m_tracks.size(); }
    const QVector<QVector<int>> &mappings() const { return m_mappings; }
    QStringList summaries() const { return m_summaries; }
    QString caption(double seconds) const;
    void emphasize(const QSet<int> &keep, int mode); // 0 hide others; 1 dim others; 2 restore.
    void park(const QSet<int> &tracks, double start, double seconds = 1.2);
    static QByteArray saveProject(const QVector<MechanismStep> &steps, const MechanismOptions &options);
    static bool loadProject(const QByteArray &json, QVector<MechanismStep> &steps, MechanismOptions &options, QString *error = nullptr);
private:
    enum class Presence : quint8 { Absent, Present, Departed };
    struct Node {
        double time = 0;
        int step = 0;
        QVector<QVector3D> positions;
        QVector<Presence> presence;
        QVector<Bond> bonds;
    };
    QMap<quint64,QVector<int>> cleavageNodes(int stage) const;
    QVector<Node> m_nodes;
    QVector<int> m_stageEnds;
    QVector<double> m_topologyChanges;
    struct PhotoCleavage { Bond bond; double start, end; };
    QVector<PhotoCleavage> m_photoCleavages;
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
