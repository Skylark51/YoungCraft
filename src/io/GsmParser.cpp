#include "io/GsmParser.h"

#include "io/ParserCommon.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>
#include <utility>

namespace youngcraft {
namespace {

bool finiteNumber(const QString &text, double &value) {
    bool ok = false;
    value = parser::toDouble(text, &ok);
    return ok && std::isfinite(value);
}

// molecularGSM: bare relative kcal/mol title line.
// pyGSM multi-XYZ: value with an optional "kcal/mol" suffix.
double commentEnergy(const QString &comment) {
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[EeDd][+-]?\d+)?)\s*(?:kcal/mol)?\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = pattern.match(comment);
    double value = kNaN;
    return match.hasMatch() && finiteNumber(match.captured(1), value) ? value : kNaN;
}

} // namespace

bool GsmParser::isGsmPath(const QString &filePath) {
    static const QRegularExpression pattern(
        QStringLiteral(R"(^(?:stringfile\.(?:xyz\d*(?:g\d*|fr)?|standard(?:\.fr)?|intel)|(?:opt_converged|grown_string|(?:growth|opt)_iters)_\d+(?:_\d+)?\.xyz)$)"),
        QRegularExpression::CaseInsensitiveOption);
    return pattern.match(QFileInfo(filePath).fileName()).hasMatch();
}

bool GsmParser::isMoldenOutput(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    const QString header = QString::fromUtf8(file.read(2048)).trimmed();
    return header.startsWith(QStringLiteral("[Molden Format]"), Qt::CaseInsensitive) &&
           header.contains(QStringLiteral("[Geometries]"), Qt::CaseInsensitive);
}

ParseResult GsmParser::parse(const QString &filePath) {
    ParseResult result;
    auto &data = result.data;
    data.filePath = filePath;
    data.format = QStringLiteral("GSM");
    data.scanAxisLabel = QStringLiteral("GSM node");
    data.molecule.title = QFileInfo(filePath).fileName();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("파일을 열 수 없습니다: %1").arg(file.errorString());
        return result;
    }
    data.index.fileSize = file.size();

    const bool molden = isMoldenOutput(filePath);
    bool inGeometries = !molden;
    bool inGeoConv = false;
    bool inEnergies = false;
    bool incomplete = false;
    QVector<double> commentEnergies;
    QVector<double> moldenEnergies;

    while (!file.atEnd()) {
        const qint64 offset = file.pos();
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (molden) {
            if (line.startsWith(QStringLiteral("[GEOCONV]"), Qt::CaseInsensitive)) {
                inGeometries = false;
                inGeoConv = true;
                continue;
            }
            if (inGeoConv) {
                if (line.compare(QStringLiteral("energy"), Qt::CaseInsensitive) == 0) {
                    inEnergies = true;
                } else if (inEnergies && !line.isEmpty()) {
                    double value = kNaN;
                    if (finiteNumber(line, value)) moldenEnergies.push_back(value);
                    else inEnergies = false;
                }
                continue;
            }
            if (line.startsWith(QStringLiteral("[Geometries]"), Qt::CaseInsensitive)) {
                inGeometries = true;
                continue;
            }
            if (!inGeometries) continue;
        }
        if (line.isEmpty()) continue;

        bool okCount = false;
        const int count = line.toInt(&okCount);
        if (!okCount || count <= 0 || count > 1000000) {
            result.error = QStringLiteral("GSM 노드의 원자 수를 읽을 수 없습니다.");
            return result;
        }
        if (!data.molecule.atoms.isEmpty() && data.molecule.atoms.size() != count) {
            result.error = QStringLiteral("GSM 노드마다 원자 수가 다릅니다.");
            return result;
        }
        if (file.atEnd()) {
            incomplete = true;
            break;
        }
        const QString comment = QString::fromUtf8(file.readLine()).trimmed();
        QVector<Atom> atoms;
        atoms.reserve(count);
        for (int i = 0; i < count; ++i) {
            if (file.atEnd()) {
                incomplete = true;
                break;
            }
            const auto fields = parser::columns(QString::fromUtf8(file.readLine()));
            bool okX = false, okY = false, okZ = false;
            const int atomicNumber = atomicNumberFor(fields.value(0));
            const double x = parser::toDouble(fields.value(1), &okX);
            const double y = parser::toDouble(fields.value(2), &okY);
            const double z = parser::toDouble(fields.value(3), &okZ);
            if (fields.size() < 4 || atomicNumber <= 0 ||
                !okX || !okY || !okZ ||
                !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
                result.error = QStringLiteral("GSM 노드 %1의 원자 %2 좌표가 잘못되었습니다.")
                                   .arg(data.frames.size() + 1).arg(i + 1);
                return result;
            }
            if (!data.molecule.atoms.isEmpty() &&
                data.molecule.atoms[i].atomicNumber != atomicNumber) {
                result.error = QStringLiteral("GSM 노드 간 원자 순서가 일치하지 않습니다.");
                return result;
            }
            Atom atom;
            atom.index = i + 1;
            atom.atomicNumber = atomicNumber;
            atom.element = elementSymbol(atomicNumber);
            atom.position = QVector3D(float(x), float(y), float(z));
            atoms.push_back(atom);
        }
        if (incomplete) break;

        Frame frame;
        frame.metrics.step = data.frames.size() + 1;
        frame.metrics.label = comment;
        for (const Atom &atom : atoms) frame.coordinates.push_back(atom.position);
        data.frames.push_back(std::move(frame));
        data.molecule.atoms = std::move(atoms);
        commentEnergies.push_back(commentEnergy(comment));
        data.index.sectionOffsets[QStringLiteral("frame_geometry")].push_back(offset);
    }

    if (data.frames.isEmpty()) {
        result.error = QStringLiteral("GSM 경로에서 완전한 XYZ 노드를 찾지 못했습니다.");
        return result;
    }
    data.molecule.rebuildBonds();

    const QVector<double> &energies = molden && !moldenEnergies.isEmpty()
                                          ? moldenEnergies : commentEnergies;
    double reference = kNaN;
    for (double energy : energies) {
        if (std::isfinite(energy)) { reference = energy; break; }
    }
    if (std::isfinite(reference)) {
        data.scanDetected = true;
        for (int i = 0; i < data.frames.size(); ++i) {
            ScanPoint point;
            point.index = i + 1;
            point.coordinate = double(i + 1);
            point.frameIndex = i;
            if (i < energies.size() && std::isfinite(energies[i]))
                point.relativeKcal = energies[i] - reference;
            // No absolute Hartree energies are present in these files.
            data.scanPoints.push_back(point);
        }
    }

    data.scanHasSummary = !incomplete && (!molden || moldenEnergies.size() == data.frames.size());
    data.status = incomplete ? CalculationStatus::Incomplete
                             : CalculationStatus::NormalTermination;
    data.terminationMessage = incomplete ? QStringLiteral("마지막 GSM 노드가 불완전합니다.")
                                         : QStringLiteral("GSM 경로 파일 읽기 완료");
    if (molden && !moldenEnergies.isEmpty() && moldenEnergies.size() != data.frames.size())
        data.warningMessage = QStringLiteral("GSM 노드 수와 에너지 개수가 다릅니다.");
    result.success = true;
    return result;
}

} // namespace youngcraft
