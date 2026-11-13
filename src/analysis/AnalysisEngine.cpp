#include "analysis/AnalysisEngine.h"

#include <QtMath>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace youngcraft {

namespace {
constexpr double kHartreeToKcal = 627.509474;
constexpr double kEvNm = 1239.841984;
}

QString AnalysisEngine::molecularFormula(const Molecule &molecule) {
    QMap<QString,int> counts;
    for(const auto &atom:molecule.atoms) {
        const auto symbol=elementSymbol(atom.atomicNumber);
        if(!symbol.isEmpty())++counts[symbol];
    }
    QString formula;
    const auto append=[&](const QString &symbol) {
        const int count=counts.take(symbol);
        if(count>0)formula+=symbol+(count>1?QString::number(count):QString());
    };
    if(counts.contains(QStringLiteral("C"))){append(QStringLiteral("C"));append(QStringLiteral("H"));}
    const auto remaining=counts.keys();
    for(const auto &symbol:remaining)append(symbol);
    return formula;
}

double AnalysisEngine::molecularMass(const Molecule &molecule) {
    double total = 0.0;
    for (const Atom &atom : molecule.atoms) total += atomicMass(atom.atomicNumber);
    return total;
}

QVector3D AnalysisEngine::principalMomentApproximation(const Molecule &molecule) {
    if (molecule.atoms.isEmpty()) return {};
    double totalMass = 0.0;
    QVector3D center;
    for (const Atom &atom : molecule.atoms) {
        const double mass = atomicMass(atom.atomicNumber);
        totalMass += mass;
        center += atom.position * float(mass);
    }
    if (totalMass > 0.0) center /= float(totalMass);
    double ixx = 0.0, iyy = 0.0, izz = 0.0;
    for (const Atom &atom : molecule.atoms) {
        const double mass = atomicMass(atom.atomicNumber);
        const QVector3D r = atom.position - center;
        ixx += mass * (r.y() * r.y() + r.z() * r.z());
        iyy += mass * (r.x() * r.x() + r.z() * r.z());
        izz += mass * (r.x() * r.x() + r.y() * r.y());
    }
    return QVector3D(float(ixx), float(iyy), float(izz));
}

double AnalysisEngine::collisionDiameter(const Molecule &molecule) {
    double maxDiameter = 0.0;
    for (const Atom &atom : molecule.atoms) maxDiameter = std::max(maxDiameter, 2.0 * vanDerWaalsRadius(atom.atomicNumber));
    return maxDiameter;
}

double AnalysisEngine::molecularAreaApproximation(const Molecule &molecule) {
    double area = 0.0;
    for (const Atom &atom : molecule.atoms) {
        const double radius = vanDerWaalsRadius(atom.atomicNumber);
        area += 4.0 * 3.14159265358979323846 * radius * radius;
    }
    return area;
}

QVector<QPointF> AnalysisEngine::uvVisSpectrum(const QVector<ExcitedState> &states,
                                               double broadeningNm, double startNm,
                                               double endNm, double stepNm,
                                               SpectrumLineShape lineShape) {
    QVector<QPointF> spectrum;
    if (stepNm <= 0.0 || endNm <= startNm) return spectrum;
    const double sigma = std::max(1.0, broadeningNm);
    for (double wavelength = startNm; wavelength <= endNm; wavelength += stepNm) {
        double intensity = 0.0;
        for (const ExcitedState &state : states) {
            if (!std::isfinite(state.wavelengthNm) || !std::isfinite(state.oscillatorStrength)) continue;
            const double delta = (wavelength - state.wavelengthNm) / sigma;
            const double profile = lineShape == SpectrumLineShape::Lorentzian
                                       ? 1.0 / (1.0 + delta * delta)
                                       : std::exp(-0.5 * delta * delta);
            intensity += state.oscillatorStrength * profile;
        }
        spectrum.push_back(QPointF(wavelength, intensity));
    }
    return spectrum;
}

QString AnalysisEngine::formatEnergy(double hartree) {
    if (!std::isfinite(hartree)) return QStringLiteral("—");
    return QStringLiteral("%1 Eh (%2 kcal/mol)").arg(hartree, 0, 'f', 8)
        .arg(hartree * kHartreeToKcal, 0, 'f', 3);
}

QString AnalysisEngine::summary(const CalculationData &data) {
    QStringList lines;
    lines << QStringLiteral("파일: %1").arg(data.filePath)
          << QStringLiteral("형식: %1").arg(data.format)
          << QStringLiteral("원자 수: %1 / 결합 수: %2").arg(data.molecule.atoms.size()).arg(data.molecule.bonds.size())
          << QStringLiteral("전하 / 다중도: %1 / %2").arg(data.molecule.charge).arg(data.molecule.multiplicity)
          << QStringLiteral("구조 프레임: %1").arg(data.frames.size())
          << QStringLiteral("진동 모드: %1 (허수 %2)").arg(data.frequencies.size())
          .arg(std::count_if(data.frequencies.cbegin(), data.frequencies.cend(), [](const FrequencyMode &m) { return m.imaginary; }))
          << QStringLiteral("SCF 에너지: %1개 / TD-DFT 상태: %2개").arg(data.scfEnergies.size()).arg(data.excitedStates.size())
          << QStringLiteral("molecular mass: %1 Da").arg(molecularMass(data.molecule), 0, 'f', 4);
    if (data.hasPcm) lines << QStringLiteral("용매 모델: %1").arg(data.solvent.isEmpty() ? QStringLiteral("PCM/CPCM") : data.solvent);
    if (data.hasNbo) lines << QStringLiteral("NBO section: 발견됨");
    if (data.hasDipole) lines << QStringLiteral("쌍극자: (%1, %2, %3) D").arg(data.dipoleDebye.x(), 0, 'f', 4).arg(data.dipoleDebye.y(), 0, 'f', 4).arg(data.dipoleDebye.z(), 0, 'f', 4);
    return lines.join(QLatin1Char('\n'));
}

} // namespace youngcraft
