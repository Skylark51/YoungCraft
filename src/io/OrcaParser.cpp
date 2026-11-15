#include "io/OrcaParser.h"

#include "io/ParserCommon.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>
#include <utility>

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

void readNormalModes(QFile &file, QVector<FrequencyMode> &modes, int atomCount) {
    const QRegularExpression header(QStringLiteral(R"(^\s*\d+(?:\s+\d+)*\s*$)"));
    QVector<int> columns;
    QVector<QVector<QVector3D>> vectors;
    int nextRow = 0, preamble = 0;
    bool started = false;
    while (!file.atEnd()) {
        const auto offset = file.pos();
        const auto line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || isSeparator(line)) continue;
        const auto values = parser::columns(line);
        if (header.match(line).hasMatch()) {
            columns.clear(); vectors.clear(); nextRow = 0; started = true;
            for (const auto &value : values) {
                columns.push_back(value.toInt());
                vectors.push_back(QVector<QVector3D>(atomCount));
            }
            continue;
        }
        if (!started && ++preamble < 20) continue;
        bool ok = false;
        const int row = values.value(0).toInt(&ok);
        if (!ok || row != nextRow || row >= 3 * atomCount || values.size() != columns.size() + 1) {
            file.seek(offset); break;
        }
        for (int i = 0; i < columns.size(); ++i) {
            double value = kNaN;
            if (!number(values[i + 1], value)) { file.seek(offset); return; }
            vectors[i][row / 3][row % 3] = float(value);
        }
        if (++nextRow == 3 * atomCount) {
            for (int i = 0; i < columns.size(); ++i)
                for (auto &mode : modes)
                    if (mode.number == columns[i] + 1) mode.displacement = vectors[i];
        }
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
    int vibrationalSpectrum = 0; // 1: IR, 2: Raman
    bool spectrumHasRows = false;
    QString irUnit;
    int intensityColumn = 2;
    int step = 0;
    int currentScan = -1;
    const QRegularExpression scanStepRe(QStringLiteral(R"(RELAXED SURFACE SCAN STEP\s+(\d+))"));
    const QRegularExpression scanAxisRe(QStringLiteral(R"((Bond|Distance|Angle|Dihedral)\s*\(\s*([0-9, ]+)\)\s*:\s*([+\-0-9.EeDd]+))"));
    QString excitedMultiplicity;
    int currentExcited = -1;
    bool absorptionTable = false;
    const QRegularExpression stateRe(QStringLiteral(
        R"(^STATE\s+(\d+):\s+E=\s*([+\-0-9.EeDd]+)\s+au\s+([+\-0-9.EeDd]+)\s+eV)"));
    const QRegularExpression transitionRe(QStringLiteral(
        R"(^\s*(\d+[ab]?)\s*->\s*(\d+[ab]?)\s*:\s*[+\-0-9.EeDd]+\s*\(c=\s*([+\-0-9.EeDd]+)\))"));

    const QRegularExpression freqRe(QStringLiteral(
        R"(^\s*(\d+)\s*:\s*([+-]?\d+(?:\.\d+)?(?:[EeDd][+-]?\d+)?)\s*cm\*\*-1)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression chargeRe(QStringLiteral(R"(Total Charge\s*[.:]*\s*(-?\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression multRe(QStringLiteral(R"(Multiplicity\s*[.:]*\s*(\d+))"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression energyRe(QStringLiteral(
        R"(FINAL SINGLE POINT ENERGY\s+([+-]?[0-9.EeDd]+))"));
    const QRegularExpression convergenceRe(QStringLiteral(
        R"(^\s*(RMS|MAX)\s+(gradient|step)\s+([+\-0-9.EeDd]+)\s+[+\-0-9.EeDd]+\s+(?:YES|NO)\s*$)"));
    const QRegularExpression scfRe(QStringLiteral(
        R"(^\s*Total Energy\s*:\s*([+-]?[0-9.EeDd]+)\s*Eh)"),
        QRegularExpression::CaseInsensitiveOption);

    while (!file.atEnd()) {
        const qint64 offset = file.pos();
        const QString line = QString::fromUtf8(file.readLine());
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) hasContent = true;
        const auto scanStep = scanStepRe.match(line);
        if (scanStep.hasMatch()) {
            data.scanDetected = true;
            ScanPoint point; point.index = scanStep.captured(1).toInt(); point.inProgress = true;
            data.scanPoints.push_back(point);
            currentScan = data.scanPoints.size() - 1;
        }
        const auto scanAxis = scanAxisRe.match(line);
        if (currentScan >= 0 && scanAxis.hasMatch()) {
            double coordinate = kNaN;
            if (number(scanAxis.captured(3), coordinate)) {
                auto &point = data.scanPoints[currentScan];
                const auto type = scanAxis.captured(1);
                if (!std::isfinite(point.coordinate)) {
                    point.coordinate = coordinate;
                    data.scanCoordinateType = type;
                    data.scanAxisLabel = type + (type == "Bond" || type == "Distance" ? " (Å)" : " (°)");
                    data.scanAtoms.clear();
                    for (const auto &atom : scanAxis.captured(2).split(',', Qt::SkipEmptyParts)) data.scanAtoms.push_back(atom.trimmed().toInt());
                } else {
                    point.coordinate2 = coordinate; data.scanDimensions = 2; data.scanAxisLabel2 = type;
                }
            }
        }
        if (currentScan >= 0 && trimmed.contains("THE OPTIMIZATION HAS CONVERGED"))
            data.scanPoints[currentScan].inProgress = false;
        if (trimmed == "RELAXED SURFACE SCAN RESULTS") data.scanHasSummary = true;

        if (trimmed.contains("EXCITED STATES") && trimmed.startsWith("TD-DFT")) {
            const bool triplet = trimmed.contains("TRIPLETS");
            if (!triplet) data.excitedStates.clear();
            excitedMultiplicity = triplet ? QStringLiteral("Triplet") : QStringLiteral("Singlet");
            currentExcited = -1;
        }
        const auto stateMatch = stateRe.match(trimmed);
        if (!excitedMultiplicity.isEmpty() && stateMatch.hasMatch()) {
            double ev = kNaN;
            if (number(stateMatch.captured(3), ev) && ev > 0) {
                ExcitedState state;
                state.state = data.excitedStates.size() + 1;
                state.label = excitedMultiplicity + " " + stateMatch.captured(1);
                state.energyEv = ev;
                state.wavelengthNm = 1239.8419843320026 / ev;
                state.frameIndex = pendingFrame;
                data.excitedStates.push_back(state);
                data.index.sectionOffsets[QStringLiteral("excited_states")].push_back(offset);
                currentExcited = data.excitedStates.size() - 1;
            }
        }
        const auto transitionMatch = transitionRe.match(line);
        if (currentExcited >= 0 && transitionMatch.hasMatch()) {
            double coefficient = kNaN;
            if (number(transitionMatch.captured(3), coefficient))
                data.excitedStates[currentExcited].transitions.push_back(
                    {transitionMatch.captured(1), transitionMatch.captured(2), coefficient});
        }
        if (trimmed.contains("ABSORPTION SPECTRUM")) {
            absorptionTable = trimmed.contains("TRANSITION ELECTRIC DIPOLE MOMENTS");
            currentExcited = -1;
            excitedMultiplicity.clear();
        } else if (absorptionTable) {
            const auto cols = parser::columns(trimmed);
            bool ok = false;
            const int row = cols.value(0).toInt(&ok) - 1;
            double wave = kNaN, strength = kNaN;
            if (ok && row >= 0 && row < data.excitedStates.size() && cols.size() >= 4 &&
                number(cols[2], wave) && wave > 0) {
                auto &state = data.excitedStates[row];
                state.wavelengthNm = wave;
                if (number(cols[3], strength)) state.oscillatorStrength = strength;
                else if (trimmed.contains("spin forbidden")) state.oscillatorStrength = 0.0;
            }
        }


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
                    if (currentScan >= 0) data.scanPoints[currentScan].optimizationFrames.push_back(pendingFrame);
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
                if (currentScan >= 0 && energyMatch.hasMatch()) {
                    auto &point = data.scanPoints[currentScan];
                    point.energyHartree = energy; point.frameIndex = pendingFrame;
                    data.frames[pendingFrame].metrics.scanCoordinate = point.coordinate;
                }
            }
        }

        const auto convergence = convergenceRe.match(line);
        double convergenceValue = kNaN;
        if (pendingFrame >= 0 && convergence.hasMatch() && number(convergence.captured(3), convergenceValue)) {
            auto &metrics = data.frames[pendingFrame].metrics;
            const bool rms = convergence.captured(1) == QStringLiteral("RMS");
            if (convergence.captured(2) == QStringLiteral("gradient")) {
                if (rms) metrics.rmsForce = convergenceValue;
                else metrics.maxForce = convergenceValue;
            } else {
                if (rms) metrics.rmsDisplacement = convergenceValue;
                else metrics.maxDisplacement = convergenceValue;
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

        if (trimmed == QStringLiteral("NORMAL MODES")) {
            inFrequencyTable = false;
            readNormalModes(file, data.frequencies, atoms.size());
            continue;
        }
        if (trimmed == QStringLiteral("IR SPECTRUM") || trimmed == QStringLiteral("RAMAN SPECTRUM")) {
            inFrequencyTable = false;
            vibrationalSpectrum = trimmed.startsWith("IR") ? 1 : 2;
            spectrumHasRows = false;
            intensityColumn = 2;
            irUnit = QStringLiteral("ORCA reported units");
            continue;
        }
        if (vibrationalSpectrum) {
            if (trimmed.startsWith("Mode") && trimmed.contains("Int")) intensityColumn = 3;
            if (trimmed.contains("T**2") && !trimmed.contains("Int")) irUnit = QStringLiteral("ORCA T²");
            if (trimmed.contains("km/mol")) irUnit = QStringLiteral("km/mol");
            const auto values = parser::columns(trimmed);
            bool ok = false;
            QString modeText = values.value(0);
            if (modeText.endsWith(':')) modeText.chop(1);
            const int modeIndex = modeText.toInt(&ok);
            double frequency = kNaN, intensity = kNaN;
            if (ok && values.size() > intensityColumn && number(values[1], frequency) && number(values[intensityColumn], intensity)) {
                for (auto &mode : data.frequencies) if (mode.number == modeIndex + 1) {
                    if (vibrationalSpectrum == 1) { mode.irIntensity = intensity; mode.irIntensityUnit = irUnit; }
                    else mode.ramanActivity = intensity;
                }
                spectrumHasRows = true;
                continue;
            }
            if (spectrumHasRows && !trimmed.isEmpty() && !isSeparator(trimmed)) vibrationalSpectrum = 0;
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
    double scanMinimum = std::numeric_limits<double>::infinity();
    for (const auto &point : data.scanPoints)
        if (std::isfinite(point.energyHartree)) scanMinimum = std::min(scanMinimum, point.energyHartree);
    for (auto &point : data.scanPoints)
        if (std::isfinite(point.energyHartree)) point.relativeKcal = (point.energyHartree - scanMinimum) * 627.509474;
    result.success = !data.molecule.atoms.isEmpty();
    if (!result.success)
        result.error = QStringLiteral("ORCA Cartesian 좌표를 찾지 못했습니다.");
    return result;
}

} // namespace youngcraft
