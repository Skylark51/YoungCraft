#include "io/NboParser.h"
#pragma once

#include <QColor>
#include <QMap>
#include <QString>
#include <QVector>
#include <QVector3D>

#include <limits>

namespace youngcraft {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

struct Atom {
    int index = 0;                 // 1-based index from the input file
    int atomicNumber = 0;
    QString element;
    QVector3D position;
    double charge = kNaN;
    double spinDensity = kNaN;
    QVector3D force;
    bool hasForce = false;
    bool visible = true;
    QString label;
    QColor colorOverride;
    float radiusScale = 1.0f;
    bool autoHydrogen = false;
    double occupancy = 1.0;
    QString disorderGroup;
    int formalCharge = 0;         // Integer formal charge; independent of partial charge.
    bool optimizationFixed = false; // Retained core for subsequent partial optimizations.
    bool autoHydrogenLocked = false; // Complete library ligand, e.g. CO / CN, retains its explicit H topology.
    float displayOpacity = 1.0f; // Viewer emphasis only; does not alter molecular data.
    float displayFlash = 0.0f;
};

enum class BondStyle { Solid, Dashed };

struct Bond {
    int first = 0;                // zero-based atom indices
    int second = 0;
    double order = 1.0;
    bool coordination = false;
    BondStyle style = BondStyle::Solid;
    bool manual = false;
    float displayOpacity = 1.0f; // Presentation-only fading of forming/breaking bonds.
    float displayFlash = 0.0f;
};

struct FrameMetrics {
    int step = -1;
    double energyHartree = kNaN;
    double maxForce = kNaN;
    double rmsForce = kNaN;
    double maxDisplacement = kNaN;
    double rmsDisplacement = kNaN;
    double scanCoordinate = kNaN;
    double ircCoordinate = kNaN;
    QString label;
};

struct Frame {
    QVector<QVector3D> coordinates;
    FrameMetrics metrics;
    int jobIndex = 0;
};

struct FrequencyMode {
    int number = 0;
    double wavenumber = kNaN;
    double irIntensity = kNaN;
    double ramanActivity = kNaN;
    bool imaginary = false;
    QVector<QVector3D> displacement;
};

struct ScanPoint {
    int index = 0;
    double coordinate = kNaN;
    double coordinate2 = kNaN;
    double energyHartree = kNaN;
    double relativeKcal = kNaN;
    int frameIndex = -1;
    QVector<int> optimizationFrames;
    bool inProgress = false;
};

struct IRCPoint {
    int index = 0;
    double coordinate = kNaN;
    double energyHartree = kNaN;
    double relativeKcal = kNaN;
    int frameIndex = -1;
    QString direction;
};

struct OrbitalTransition {
    QString from;
    QString to;
    double coefficient = kNaN;
};

struct ExcitedState {
    int state = 0;
    double energyEv = kNaN;
    double wavelengthNm = kNaN;
    double oscillatorStrength = kNaN;
    QVector<OrbitalTransition> transitions;
};

struct OrbitalInfo {
    int number = 0;
    double energyEv = kNaN;
    double occupation = kNaN;
    QString symmetry;
    QString spin = QStringLiteral("Alpha");
};

enum class CalculationStatus {
    Unknown,
    NormalTermination,
    ErrorTermination,
    Incomplete
};

struct CoordinationSite { int center = -1; QVector3D direction; bool occupied = false; int donor = -1; };
struct HapticContact { int metal = -1; QVector<int> atoms; QString name; };

struct Molecule {
    QVector<Atom> atoms;
    QVector<Bond> bonds;
    QVector<CoordinationSite> coordinationSites; // Editor markers, never chemical atoms.
    QVector<HapticContact> hapticContacts;
    int charge = 0;
    int multiplicity = 1;
    QString title;

    bool isEmpty() const { return atoms.isEmpty(); }
    void rebuildBonds(double cutoffScale = 1.25, bool includeHydrogen = true,
                      bool includeMetalCoordination = true, bool preserveManual = true);
    int bondIndex(int first, int second) const;
    void setBond(int first, int second, double order, bool coordination = false,
                 BondStyle style = BondStyle::Solid, bool manual = true);
    void removeBond(int first, int second);
    bool removeAtom(int index);
    void reconcileAutoHydrogens(bool addOnly = false, bool removeOnly = false);
};

struct ParseIndex {
    qint64 fileSize = 0;
    QMap<QString, QVector<qint64>> sectionOffsets;
};

struct CalculationData {
    QString filePath;
    QString format;
    QString route;
    QString method;
    QString basis;
    QString solvent;
    QString nboSummary;
    NboData nbo;
    QString terminationMessage;
    QString warningMessage;

    Molecule molecule;
    QVector<Frame> frames;
    QVector<FrequencyMode> frequencies;
    QVector<ScanPoint> scanPoints;
    bool scanDetected = false;
    bool scanHasSummary = false;
    QString scanCoordinateType;
    QVector<int> scanAtoms; // Zero-based atoms of the first scan coordinate.
    QString scanAxisLabel = QStringLiteral("Point index");
    int scanDimensions = 1;
    QString scanAxisLabel2;
    QString scanStatusText() const {
        if (format == QStringLiteral("GSM")) {
            if (scanPoints.isEmpty()) return QStringLiteral("GSM 경로: 구조만 확인 가능 (노드 에너지 없음)");
            return QStringLiteral("GSM 경로: %1개 노드 · 상대 에너지 (kcal/mol)%2")
                .arg(scanPoints.size())
                .arg(status == CalculationStatus::Incomplete ? QStringLiteral(" · 마지막 노드 불완전") : QString());
        }
        if (!scanDetected) return QStringLiteral("Scan 데이터 없음");
        if (scanPoints.isEmpty()) return QStringLiteral("Scan은 감지되었지만 아직 표시할 point 없음");
        if (scanPoints.last().inProgress)
            return QStringLiteral("PES Scan: %1개 point 완료 · 점 %2 최적화 진행 중 (파일 기준)")
                .arg(scanPoints.size() - 1).arg(scanPoints.last().index);
        if (!scanHasSummary || status != CalculationStatus::NormalTermination)
            return QStringLiteral("출력이 중간에 끝났으며 현재까지 %1개 point 표시 중").arg(scanPoints.size());
        return QStringLiteral("PES Scan: %1개 point").arg(scanPoints.size());
    }
    QVector<IRCPoint> ircPoints;
    QVector<ExcitedState> excitedStates;
    QVector<OrbitalInfo> orbitals;
    QVector<double> scfEnergies;
    QVector<double> scfCycles;
    QVector3D dipoleDebye;
    bool hasDipole = false;
    bool hasNbo = false;
    bool hasPcm = false;
    bool hasCubeGrid = false;
    CalculationStatus status = CalculationStatus::Unknown;
    ParseIndex index;

    int finalFrameIndex() const { return frames.isEmpty() ? -1 : frames.size() - 1; }
};

QString elementSymbol(int atomicNumber);
int atomicNumberFor(QString symbol);
double atomicMass(int atomicNumber);
double covalentRadius(int atomicNumber);
double vanDerWaalsRadius(int atomicNumber);
QColor elementColor(int atomicNumber, bool monochrome = false);
bool isMetal(int atomicNumber);
void inferBonds(Molecule &molecule, double cutoffScale = 1.25,
                bool includeHydrogen = true, bool includeMetalCoordination = true,
                bool preserveManual = true);

} // namespace youngcraft
