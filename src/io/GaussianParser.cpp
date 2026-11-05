#include "io/GaussianParser.h"

#include "io/NboParser.h"
#include "io/ParserCommon.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>
#include <cmath>


namespace youngcraft {

namespace {
#include "io/gaussian/GaussianParserHelpers.inc"
} // namespace

ParseResult GaussianParser::parse(const QString &filePath, const ParseOptions &options) {
    ParseResult result;
    result.data.filePath = filePath;
    result.data.format = QStringLiteral("Gaussian");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("파일을 열 수 없습니다: %1").arg(file.errorString());
        return result;
    }
    result.data.index.fileSize = file.size();

    auto &frames = result.data.frames;
    QVector<Atom> lastAtoms;
    QVector<double> mullikenCharges;
    QVector<double> spinDensities;
    FrameMetrics currentMetrics;
    int jobIndex = 0;
    int currentStep = -1;
    bool lastOrientationStandard = false;
    bool frameHasEnergy = false;
    double pointEnergy = kNaN;
    int pointFrame = -1;
    QVector<CompletedScanFrame> completedScanFrames;
    QVector<int> pointFrames;
    bool ircDetected = false;
    QString ircDirection;
    QVector<int> frequencyGroup;
    bool readingDisplacements = false;
    int currentExcitedState = -1;
    bool sawAnyContent = false;
    bool previousOrbitalLine = false, previousOrbitalOccupied = false;
    QString previousOrbitalSpin;
    const QRegularExpression orbitalLineRe(QStringLiteral(R"(^\s*(Alpha|Beta)\s+(occ\.|virt\.)\s+eigenvalues\s*--)"));

    const QRegularExpression chargeRe(QStringLiteral(R"(Charge\s*=\s*(-?\d+)\s+Multiplicity\s*=\s*(\d+))"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression energyRe(QStringLiteral(R"(SCF Done:\s+.*?=\s*([+-]?[0-9.EeDd]+))"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression stepRe(QStringLiteral(R"(Step number\s+(\d+))"),
                                     QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression maxForceRe(QStringLiteral(R"(Maximum Force\s+([+-]?[0-9.EeDd]+))"),
                                         QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression rmsForceRe(QStringLiteral(R"(RMS\s+Force\s+([+-]?[0-9.EeDd]+))"),
                                         QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression maxDispRe(QStringLiteral(R"(Maximum Displacement\s+([+-]?[0-9.EeDd]+))"),
                                        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression rmsDispRe(QStringLiteral(R"(RMS\s+Displacement\s+([+-]?[0-9.EeDd]+))"),
                                        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression dipoleRe(QStringLiteral(R"(X\s*=\s*([+-]?[0-9.EeDd]+)\s+Y\s*=\s*([+-]?[0-9.EeDd]+)\s+Z\s*=\s*([+-]?[0-9.EeDd]+))"),
                                       QRegularExpression::CaseInsensitiveOption);

    while (!file.atEnd()) {
        const qint64 offset = file.pos();
        const QString line = QString::fromUtf8(file.readLine());
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            sawAnyContent = true;
        }

        if (readingDisplacements) {
            if (trimmed.startsWith(QLatin1Char('-'))) continue;
            const QStringList fields = parser::columns(line);
            bool okIndex = false;
            if (fields.size() >= 2) {
                const int atomIndex = fields[0].toInt(&okIndex);
                if (okIndex && frequencyGroup.size() > 0) {
                    QVector<double> values;
                    for (int i = 2; i < fields.size(); ++i) {
                        bool ok = false;
                        const double value = parser::toDouble(fields[i], &ok);
                        if (ok) {
                            values.push_back(value);
                        }
                    }
                    if (values.size() >= frequencyGroup.size() * 3 && atomIndex > 0) {
                        for (int mode = 0; mode < frequencyGroup.size(); ++mode) {
                            const int modeIndex = frequencyGroup[mode];
                            if (modeIndex < 0 || modeIndex >= result.data.frequencies.size()) {
                                continue;
                            }
                            auto &displacements = result.data.frequencies[modeIndex].displacement;
                            if (displacements.size() < atomIndex) {
                                displacements.resize(atomIndex);
                            }
                            displacements[atomIndex - 1] = QVector3D(
                                float(values[mode * 3]), float(values[mode * 3 + 1]),
                                float(values[mode * 3 + 2]));
                        }
                        continue;
                    }
                }
            }
            readingDisplacements = false;
            frequencyGroup.clear();
        }

        if (trimmed.contains(QStringLiteral("Input orientation:"), Qt::CaseInsensitive) ||
            trimmed.contains(QStringLiteral("Standard orientation:"), Qt::CaseInsensitive)) {
            const bool isStandard = trimmed.contains(QStringLiteral("Standard orientation:"), Qt::CaseInsensitive);
            result.data.index.sectionOffsets[isStandard ? QStringLiteral("standard_orientation")
                                                        : QStringLiteral("input_orientation")]
                .push_back(offset);
            QVector<Atom> atoms = readOrientationBlock(file);
            if (!atoms.isEmpty()) {
                lastAtoms = atoms;
                currentMetrics = {};
                Frame frame = frameFromAtoms(atoms, jobIndex, currentMetrics);
                // Input/Standard blocks preceding the same SCF are one geometry.
                if (!frames.isEmpty() && frames.last().jobIndex == jobIndex &&
                    !frameHasEnergy && isStandard != lastOrientationStandard) {
                    if (isStandard == options.preferStandardOrientation) {frames.last() = frame;result.data.index.sectionOffsets["frame_geometry"].last()=offset;}
                } else {
                    frames.push_back(frame);
                    result.data.index.sectionOffsets["frame_geometry"].push_back(offset);
                }
                lastOrientationStandard = isStandard;
                frameHasEnergy = false;
            }
            continue;
        }

        if (isJobBoundary(trimmed)) {
            ++jobIndex;
            currentMetrics = {};
            currentStep = -1;
            frameHasEnergy = false;
            pointEnergy = kNaN;
            pointFrame = -1;
            pointFrames.clear();
            result.data.status = CalculationStatus::Unknown;
        }

        if (trimmed.startsWith(QStringLiteral("Optimization completed"), Qt::CaseInsensitive)) {
            completedScanFrames.push_back({offset, pointFrame, pointEnergy, jobIndex, pointFrames});
            pointFrames.clear();
            pointEnergy = kNaN;
            pointFrame = -1;
        }

        if (trimmed.startsWith(QLatin1Char('#'))) {
            result.data.route += trimmed + QLatin1Char(' ');
        }

        const auto chargeMatch = chargeRe.match(line);
        if (chargeMatch.hasMatch()) {
            bool okCharge = false;
            bool okMultiplicity = false;
            const int charge = chargeMatch.captured(1).toInt(&okCharge);
            const int multiplicity = chargeMatch.captured(2).toInt(&okMultiplicity);
            if (okCharge) {
                result.data.molecule.charge = charge;
            }
            if (okMultiplicity) {
                result.data.molecule.multiplicity = multiplicity;
            }
        }

        if (trimmed.contains(QStringLiteral("SCF Done:"), Qt::CaseInsensitive)) {
            double energy = kNaN;
            if (matchNumber(line, energyRe, energy)) {
                result.data.scfEnergies.push_back(energy);
                pointEnergy = energy;
                pointFrame = -1;
                if (!frames.isEmpty() && frames.last().jobIndex == jobIndex) {
                    frames.last().metrics.energyHartree = energy;
                    frames.last().metrics.step = currentStep;
                    pointFrame = int(frames.size() - 1);
                    if (pointFrames.isEmpty() || pointFrames.last() != pointFrame)
                        pointFrames.push_back(pointFrame);
                    frameHasEnergy = true;
                }
            }
        }

        if (trimmed.contains(QStringLiteral("Step number"), Qt::CaseInsensitive)) {
            const auto match = stepRe.match(line);
            if (match.hasMatch()) {
                currentStep = match.captured(1).toInt();
                currentMetrics.step = currentStep;
                if (!frames.isEmpty() && frames.last().jobIndex == jobIndex)
                    frames.last().metrics.step = currentStep;
            }
        }

        double metric = kNaN;
        if (matchNumber(line, maxForceRe, metric)) currentMetrics.maxForce = metric;
        if (matchNumber(line, rmsForceRe, metric)) currentMetrics.rmsForce = metric;
        if (matchNumber(line, maxDispRe, metric)) currentMetrics.maxDisplacement = metric;
        if (matchNumber(line, rmsDispRe, metric)) currentMetrics.rmsDisplacement = metric;
        if (!frames.isEmpty() && frames.last().jobIndex == jobIndex &&
            (trimmed.contains(QStringLiteral("Maximum Force"), Qt::CaseInsensitive) ||
             trimmed.contains(QStringLiteral("RMS Force"), Qt::CaseInsensitive) ||
             trimmed.contains(QStringLiteral("Maximum Displacement"), Qt::CaseInsensitive) ||
             trimmed.contains(QStringLiteral("RMS Displacement"), Qt::CaseInsensitive))) {
            const double energy = frames.last().metrics.energyHartree;
            frames.last().metrics = currentMetrics;
            frames.last().metrics.energyHartree = energy;
        }

        if (trimmed.contains(QStringLiteral("Mulliken charges:"), Qt::CaseInsensitive)) {
            result.data.index.sectionOffsets[QStringLiteral("mulliken_charges")].push_back(offset);
            mullikenCharges = readAtomicValues(file, false);
            applyCharges(lastAtoms, mullikenCharges);
        }
        if (trimmed.contains(QStringLiteral("Mulliken atomic spin densities:"), Qt::CaseInsensitive)) {
            result.data.index.sectionOffsets[QStringLiteral("mulliken_spin")].push_back(offset);
            spinDensities = readAtomicValues(file, true);
            applySpins(lastAtoms, spinDensities);
        }

        if (trimmed.contains(QStringLiteral("Forces (Hartrees/Bohr)"), Qt::CaseInsensitive)) {
            result.data.index.sectionOffsets[QStringLiteral("forces")].push_back(offset);
            readForceBlock(file, lastAtoms);
        }

        if (trimmed.startsWith(QStringLiteral("Frequencies --"), Qt::CaseInsensitive)) {
            result.data.index.sectionOffsets[QStringLiteral("frequencies")].push_back(offset);
            frequencyGroup.clear();
            const QVector<double> values = numbersAfterMarker(line, QStringLiteral("--"));
            for (double value : values) {
                FrequencyMode mode;
                mode.number = result.data.frequencies.size() + 1;
                mode.wavenumber = value;
                mode.imaginary = value < 0.0;
                mode.displacement.resize(lastAtoms.size());
                frequencyGroup.push_back(result.data.frequencies.size());
                result.data.frequencies.push_back(mode);
                result.data.index.sectionOffsets["frequency_modes"].push_back(offset);
            }
        } else if (trimmed.contains(QStringLiteral("IR Inten"), Qt::CaseInsensitive) &&
                   !frequencyGroup.isEmpty()) {
            const QVector<double> values = numbersAfterMarker(line, QStringLiteral("--"));
            for (int i = 0; i < frequencyGroup.size() && i < values.size(); ++i) {
                result.data.frequencies[frequencyGroup[i]].irIntensity = values[i];
            }
        } else if (trimmed.contains(QStringLiteral("Raman Activ"), Qt::CaseInsensitive) &&
                   !frequencyGroup.isEmpty()) {
            const QVector<double> values = numbersAfterMarker(line, QStringLiteral("--"));
            for (int i = 0; i < frequencyGroup.size() && i < values.size(); ++i) {
                result.data.frequencies[frequencyGroup[i]].ramanActivity = values[i];
            }
        } else if (line.simplified().startsWith(QStringLiteral("Atom AN"), Qt::CaseInsensitive) &&
                   !frequencyGroup.isEmpty()) {
            readingDisplacements = true;
        }

        ExcitedState excitedState;
        if (parseExcitedState(line, excitedState)) {
            result.data.index.sectionOffsets[QStringLiteral("excited_states")].push_back(offset);
            result.data.excitedStates.push_back(excitedState);
            currentExcitedState = result.data.excitedStates.size() - 1;
        } else if (currentExcitedState >= 0 && currentExcitedState < result.data.excitedStates.size()) {
            OrbitalTransition transition;
            if (parseTransition(line, transition)) {
                result.data.excitedStates[currentExcitedState].transitions.push_back(transition);
            }
        }

        const auto orbitalMatch=orbitalLineRe.match(line);
        if (orbitalMatch.hasMatch()) {
            result.data.index.sectionOffsets["orbital_energies"].push_back(offset);
            const QString spin=orbitalMatch.captured(1);
            const bool occupied=orbitalMatch.captured(2)=="occ.";
            if(spin=="Alpha" && occupied && (!previousOrbitalLine || previousOrbitalSpin=="Beta" || !previousOrbitalOccupied)) result.data.orbitals.clear();
            if(spin=="Beta") for(auto &o:result.data.orbitals) if(o.spin=="Alpha" && o.occupation>0)o.occupation=1;
            int number=0;for(const auto &o:result.data.orbitals)if(o.spin==spin)number=qMax(number,o.number);
            const auto values=numbersAfterMarker(line,QStringLiteral("--"));
            for(double value:values)result.data.orbitals.push_back({++number,value*27.211386245988,occupied?(spin=="Beta"?1.0:2.0):0.0,{},spin});
            previousOrbitalSpin=spin;previousOrbitalOccupied=occupied;
        }
        previousOrbitalLine=orbitalMatch.hasMatch();

        if (trimmed.contains(QStringLiteral("Dipole moment"), Qt::CaseInsensitive)) {
            while (!file.atEnd()) {
                const QString dipoleLine = QString::fromUtf8(file.readLine());
                const auto match = dipoleRe.match(dipoleLine);
                if (match.hasMatch()) {
                    bool okX = false, okY = false, okZ = false;
                    const double x = parser::toDouble(match.captured(1), &okX);
                    const double y = parser::toDouble(match.captured(2), &okY);
                    const double z = parser::toDouble(match.captured(3), &okZ);
                    if (okX && okY && okZ) {
                        result.data.dipoleDebye = QVector3D(float(x), float(y), float(z));
                        result.data.hasDipole = true;
                    }
                    break;
                }
                if (dipoleLine.trimmed().isEmpty()) break;
            }
        }

        if (trimmed.contains(QStringLiteral("NBO"), Qt::CaseInsensitive) ||
            trimmed.contains(QStringLiteral("Natural Population"), Qt::CaseInsensitive)) {
            result.data.hasNbo = true;
            result.data.index.sectionOffsets[QStringLiteral("nbo")].push_back(offset);
            if (result.data.nboSummary.isEmpty()) result.data.nboSummary = trimmed;
        }
        if (trimmed.contains(QStringLiteral("CPCM"), Qt::CaseInsensitive) ||
            trimmed.contains(QStringLiteral("Polarizable Continuum"), Qt::CaseInsensitive) ||
            trimmed.contains(QStringLiteral("Solvent:"), Qt::CaseInsensitive)) {
            result.data.hasPcm = true;
            result.data.index.sectionOffsets[QStringLiteral("pcm")].push_back(offset);
            if (trimmed.contains(QStringLiteral("Solvent:"), Qt::CaseInsensitive)) {
                result.data.solvent = trimmed;
            }
        }

        if (trimmed.contains(QStringLiteral("IRC"), Qt::CaseInsensitive) &&
            (trimmed.contains(QStringLiteral("Forward"), Qt::CaseInsensitive) ||
             trimmed.contains(QStringLiteral("Reverse"), Qt::CaseInsensitive) ||
             trimmed.contains(QStringLiteral("Reaction path"), Qt::CaseInsensitive))) {
            ircDetected = true;
            ircDirection = trimmed.contains(QStringLiteral("Reverse"), Qt::CaseInsensitive)
                               ? QStringLiteral("reverse")
                               : QStringLiteral("forward");
            result.data.index.sectionOffsets[QStringLiteral("irc")].push_back(offset);
        }

        if (trimmed.contains(QStringLiteral("Normal termination of Gaussian"), Qt::CaseInsensitive)) {
            result.data.status = CalculationStatus::NormalTermination;
            result.data.terminationMessage = trimmed;
        } else if (trimmed.contains(QStringLiteral("Error termination"), Qt::CaseInsensitive) ||
                   trimmed.contains(QStringLiteral("Erroneous termination"), Qt::CaseInsensitive)) {
            result.data.status = CalculationStatus::ErrorTermination;
            result.data.terminationMessage = trimmed;
        }
    }

    if (!lastAtoms.isEmpty()) {
        applyCharges(lastAtoms, mullikenCharges);
        applySpins(lastAtoms, spinDensities);
        result.data.molecule.atoms = lastAtoms;
        result.data.molecule.rebuildBonds();
    }
    if (result.data.frames.isEmpty() && !lastAtoms.isEmpty()) {
        result.data.frames.push_back(frameFromAtoms(lastAtoms, jobIndex, currentMetrics));
    }
    for (int i = 0; i < result.data.frames.size(); ++i) {
        if (result.data.frames[i].metrics.step < 0) {
            result.data.frames[i].metrics.step = i + 1;
        }
    }
    buildScanData(result.data, completedScanFrames, pointFrames);
    buildIrcData(result.data, ircDetected, ircDirection);

    if (result.data.status == CalculationStatus::Unknown) {
        result.data.status = (sawAnyContent && !result.data.frames.isEmpty())
                                  ? CalculationStatus::Incomplete
                                  : CalculationStatus::Unknown;
        if (result.data.status == CalculationStatus::Incomplete) {
            result.data.warningMessage = QStringLiteral("출력이 정상 종료 문구 없이 끝났습니다.");
        }
    }
    if (result.data.route.endsWith(QLatin1Char(' '))) result.data.route.chop(1);
    result.success = !result.data.molecule.atoms.isEmpty() || !result.data.frames.isEmpty() ||
                     !result.data.excitedStates.isEmpty() || result.data.scanDetected;
    if (!result.success && result.error.isEmpty()) {
        result.error = QStringLiteral("Gaussian 구조 또는 계산 결과를 찾지 못했습니다.");
    }
    result.data.nbo = NboParser::parse(filePath);
    result.data.hasNbo = result.data.hasNbo || !result.data.nbo.charges.isEmpty() || !result.data.nbo.wiberg.isEmpty();
    return result;
}

} // namespace youngcraft
