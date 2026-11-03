#pragma once
#include "model/Molecule.h"
#include "compute/GeometryConstraints.h"
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <optional>

namespace youngcraft {
// A one-dimensional relaxed internal-coordinate PES scan.
struct XtbScan {
    QString type = QStringLiteral("distance"); // distance / angle / dihedral
    QVector<int> atoms; // zero-based indices
    double from = 1.0, to = 2.0;
    int points = 11;
};
struct XtbOptions {
    QVector<GeometryConstraint> constraints;
    std::optional<XtbScan> scan;
    bool optimize = true;
    bool preserveCoordination = false;
    bool preserveBondLengths = false;
    int method = 2, charge = 0, multiplicity = 1, cycles = 500, threads = 2;
    QString solvent, level = QStringLiteral("normal");
};
struct XtbFrame {
    QVector<QVector3D> coordinates;
    double energy = kNaN, gradient = kNaN;
    int step = 0;
};
// Retains incomplete lines/frames across polls; complete frames are consumed once.
class XtbTrajectoryParser {
public:
    QVector<XtbFrame> append(const QByteArray &bytes, const Molecule &expected);
    QString error;
private:
    QByteArray pending;
    int frameIndex = 0;
};
class XtbRunner : public QObject {
    Q_OBJECT
public:
    enum class State { Idle, Starting, Running, Stopping, Finished, NotConverged, Cancelled, Failed };
    explicit XtbRunner(QObject *parent = nullptr);
    ~XtbRunner() override;
    bool start(const Molecule &, const XtbOptions &, const QString &runtime);
    void cancel();
    bool isRunning() const { return process.state() == QProcess::Running; }
    State state = State::Idle;
    QString directory, output, error, geometryWarning;
    QStringList arguments;
    Molecule result;
    QVector<XtbFrame> scanFrames;
    int tsCandidateIndex = -1; // an interior local maximum, not a verified TS
    bool selectScanFrame(int index);
    double energy = kNaN, gradient = kNaN, gap = kNaN;
    int cycle = 0;
    static QStringList solvents();
    static QString xyz(const Molecule &);
signals:
    void frameReady(const youngcraft::XtbFrame &frame);
    void metricsChanged();
    void completed();
private:
    void readOutput();
    void pollTrajectory();
    void finish(int code, QProcess::ExitStatus status);
    bool fail(const QString &message);
    QProcess process;
    QTimer timer;
    QFile logFile;
    XtbTrajectoryParser parser;
    Molecule input;
    XtbOptions options;
    qint64 offset = 0;
    QByteArray outputPending;
    bool convergence = false, nonconvergence = false;
};
}
