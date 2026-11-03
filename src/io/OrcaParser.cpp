#include "io/OrcaParser.h"

#include "io/ParserCommon.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>

namespace youngcraft {
namespace {

bool number(const QString &text, double &value) {
    bool ok = false;
    value = parser::toDouble(text, &ok);
    return ok && std::isfinite(value);
}

bool isSeparator(const QString &text) {
    return !text.isEmpty() && (text.at(0) == QLatin1Char('-') ||
                               text.at(0) == QLatin1Char('='));
}

// Read the adjacent coordinate table without consuming the next section header.
QVector<Atom> readCoordinates(QFile &file) {
    QVector<Atom> atoms;
    while (!file.atEnd()) {
        const qint64 start = file.pos();
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || isSeparator(line)) {
            if (!atoms.isEmpty()) break;
            continue;
        }
        const QStringList cols = parser::columns(line);
        if (cols.size() < 4) {
            if (!atoms.isEmpty()) file.seek(start);
            break;
        }
        const int atomicNumber = atomicNumberFor(cols[0]);
        double x = 0.0, y = 0.0, z = 0.0;
        if (!atomicNumber || !number(cols[1], x) || !number(cols[2], y) ||
            !number(cols[3], z)) {
            if (!atoms.isEmpty()) file.seek(start);
            break;
        }
        Atom atom;
        atom.index = atoms.size() + 1;
        atom.atomicNumber = atomicNumber;
        atom.element = elementSymbol(atomicNumber);
        atom.position = QVector3D(float(x), float(y), float(z));
        atoms.push_back(atom);
    }
    return atoms;
}

void readMulliken(QFile &file, QVector<Atom> &atoms) {
    // ORCA: "0 C : -0.20 0.13", or charge-only "0 C : -0.20".
    int parsed = 0;
    while (!file.atEnd()) {
        const qint64 start = file.pos();
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty()) {
            if (parsed > 0) break;
            continue;
        }
        if (isSeparator(line)) continue;
        const QStringList cols = parser::columns(line);
        if (cols.size() < 4 || !cols[0].endsWith(QLatin1Char(':')) &&
            !cols.value(2).endsWith(QLatin1Char(':'))) {
            if (parsed > 0) file.seek(start);
            break;
        }
        bool ok = false;
        const int idx = cols[0].toInt(&ok);
        if (!ok || idx < 0 || idx >= atoms.size()) {
            if (parsed > 0) file.seek(start);
            break;
        }
        const int values = cols[2] == QStringLiteral(":") ? 3 : 2;
        double charge = 0.0;
        if (!number(cols.value(values), charge)) {
            // Standard table: 0 C : -0.20 [spin]
            if (!number(cols.value(3), charge)) break;
        }
        atoms[idx].charge = charge;
        double spin = 0.0;
        if (number(cols.value(values + 1), spin)) atoms[idx].spinDensity = spin;
        ++parsed;
    }
}

Frame frameFrom(const QVector<Atom> &atoms) {
    Frame frame;
    for (const Atom &atom : atoms) frame.coordinates.push_back(atom.position);
    return frame;
}

} // namespace

bool OrcaParser::isOrcaOutput(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;
    // ORCA prints a distinctive banner near the beginning. Avoid scanning huge outputs twice.
    const QByteArray header = file.read(65536);
    return header.contains("O   R   C   A") ||
           header.contains("Program Version") && header.contains("ORCA") ||
           header.contains("This ORCA version") ||
           header.contains("ORCA - An Ab Initio") ||
           header.contains("ORCA quantum chemistry");
}

ParseResult OrcaParser::parse(const QString &filePath, const ParseOptions &options) {
    Q_UNUSED(options);
    ParseResult result;
    auto &data = result.data;
    data.filePath = filePath;
    data.format = QStringLiteral("ORCA");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("파일을 열 수 없습니다: %1").arg(file.errorString());
        return result;
    }
    data.index.fileSize = file.size();

    QVector<Atom> atoms;
    double latestEnergy = kNaN;
    int pendingFrame = -1;
    bool hasContent = false;
    bool inFrequencyTable = false;
    int step = 0;
    const QRegularExpression freqRe(QStringLiteral(
        R"(^\s*(\d+)\s*:\s*([+-]?\d+(?:\.\d+)?(?:[EeDd][+-]?\d+)?)\s*cm\*\*-1)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression chargeRe(QStringLiteral(R"(Total Charge\s*[:.]?\s*(-?\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression multRe(QStringLiteral(R"(Multiplicity\s*[:.]?\s*(\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression energyRe(QStringLiteral(
        R"(FINAL SINGLE POINT ENERGY\s+([+-]?[0-9.EeDd]+))"));
    const QRegularExpression scfRe(QStringLiteral(
        R"(^\s*Total Energy\s*:\s*([+-]?[0-9.EeDd]+)\s*Eh)"),
        QRegularExpression::CaseInsensitiveOption);

    while (!file.atEnd()) {
        const qint64 offset = file.pos();
        const QString line = QString::fromUtf8(file.readLine());
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) hasContent = true;

        if (trimmed.contains(QStringLiteral("CARTESIAN COORDINATES (ANGSTROEM)"),
                             Qt::CaseInsensitive) ||
            trimmed.contains(QStringLiteral("CARTESIAN COORDINATES (ANGSTROM)"),
                             Qt::CaseInsensitive)) {
            QVector<Atom> next = readCoordinates(file);
            if (!next.isEmpty()) {
                // Keep the previous frame's energy if a new geometry follows it.
                if (atoms.isEmpty() || next.size() == atoms.size()) {
                    atoms = std::move(next);
                    Frame frame = frameFrom(atoms);
                    frame.metrics.step = ++step;
                    data.frames.push_back(std::move(frame));
                    data.index.sectionOffsets[QStringLiteral("frame_geometry")].push_back(offset);
                    pendingFrame = data.frames.size() - 1;
                }
            }
            continue;
        }

        const auto energyMatch = energyRe.match(line);
        const auto scfMatch = scfRe.match(line);
        const auto match = energyMatch.hasMatch() ? energyMatch : scfMatch;
        double energy = kNaN;
        if (match.hasMatch() && number(match.captured(1), energy)) {
            latestEnergy = energy;
            // Final energy supersedes the preceding iteration energy.
            if (energyMatch.hasMatch() || data.scfEnergies.isEmpty() ||
                data.scfEnergies.last() != energy)
                data.scfEnergies.push_back(energy);
            if (pendingFrame >= 0) {
                data.frames[pendingFrame].metrics.energyHartree = energy;
            }
        }

        const auto chargeMatch = chargeRe.match(line);
        if (chargeMatch.hasMatch()) data.molecule.charge = chargeMatch.captured(1).toInt();
        const auto multMatch = multRe.match(line);
        if (multMatch.hasMatch()) data.molecule.multiplicity = multMatch.captured(1).toInt();

        if (trimmed.contains(QStringLiteral("MULLIKEN ATOMIC CHARGES"),
                             Qt::CaseInsensitive) && !atoms.isEmpty()) {
            data.index.sectionOffsets[QStringLiteral("mulliken_charges")].push_back(offset);
            readMulliken(file, atoms);
            continue;
        }

        if (trimmed == QStringLiteral("VIBRATIONAL FREQUENCIES")) {
            inFrequencyTable = true;
            data.index.sectionOffsets[QStringLiteral("frequencies")].push_back(offset);
            continue;
        }
        if (inFrequencyTable) {
            const auto freqMatch = freqRe.match(line);
            if (freqMatch.hasMatch()) {
                double value = kNaN;
                if (number(freqMatch.captured(2), value)) {
                    FrequencyMode mode;
                    mode.number = freqMatch.captured(1).toInt() + 1;
                    mode.wavenumber = value;
                    mode.imaginary = value < 0.0;
                    mode.displacement.resize(atoms.size());
                    data.frequencies.push_back(mode);
                    data.index.sectionOffsets[QStringLiteral("frequency_modes")].push_back(offset);
                }
                continue;
            }
            if (!trimmed.isEmpty() && !isSeparator(trimmed) &&
                !trimmed.startsWith(QStringLiteral("Scaling factor"), Qt::CaseInsensitive) &&
                !trimmed.startsWith(QStringLiteral("Mode"), Qt::CaseInsensitive) &&
                !trimmed.contains(QStringLiteral("cm**-1"), Qt::CaseInsensitive) &&
                !trimmed.startsWith(QStringLiteral("The frequencies"), Qt::CaseInsensitive)) {
                inFrequencyTable = false;
            }
        }

        if (trimmed.contains(QStringLiteral("CPCM"), Qt::CaseInsensitive)) {
            data.hasPcm = true;
            data.index.sectionOffsets[QStringLiteral("pcm")].push_back(offset);
        }
        if (trimmed.contains(QStringLiteral("ORCA TERMINATED NORMALLY"),
                             Qt::CaseInsensitive)) {
            data.status = CalculationStatus::NormalTermination;
            data.terminationMessage = trimmed;
        } else if (trimmed.contains(QStringLiteral("ORCA finished by error"),
                                    Qt::CaseInsensitive) ||
                   trimmed.contains(QStringLiteral("ERROR TERMINATION"),
                                    Qt::CaseInsensitive)) {
            data.status = CalculationStatus::ErrorTermination;
            data.terminationMessage = trimmed;
        }
    }

    if (!atoms.isEmpty()) {
        data.molecule.atoms = atoms;
        data.molecule.rebuildBonds();
        // Reflect the last atomic properties in the final visible frame's model.
        if (data.frames.isEmpty()) data.frames.push_back(frameFrom(atoms));
    }
    if (data.status == CalculationStatus::Unknown && hasContent) {
        data.status = CalculationStatus::Incomplete;
        data.warningMessage = QStringLiteral("ORCA 출력이 정상 종료 문구 없이 끝났습니다.");
    }
    if (std::isfinite(latestEnergy) && !data.frames.isEmpty() &&
        !std::isfinite(data.frames.last().metrics.energyHartree))
        data.frames.last().metrics.energyHartree = latestEnergy;
    result.success = !data.molecule.atoms.isEmpty();
    if (!result.success)
        result.error = QStringLiteral("ORCA Cartesian 좌표를 찾지 못했습니다.");
    return result;
}

} // namespace youngcraft
