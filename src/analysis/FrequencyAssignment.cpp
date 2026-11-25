#include "analysis/FrequencyAssignment.h"

#include <QStringList>
#include <algorithm>
#include <cmath>

namespace youngcraft {
namespace {
bool finite(const QVector3D &v) {
    return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
}

QString atomName(const Molecule &molecule, int i) {
    const auto &atom = molecule.atoms[i];
    const QString symbol = atom.element.isEmpty() ? elementSymbol(atom.atomicNumber) : atom.element;
    return symbol + QString::number(atom.index > 0 ? atom.index : i + 1);
}

struct BondDirection {
    int first, second;
    QVector3D unit;
    double length;
};

struct Contribution {
    QString label;
    double score;
};

QString describe(const Molecule &molecule, const QVector<BondDirection> &bonds,
                 const QVector<QVector<int>> &neighbors, const FrequencyMode &mode) {
    if (molecule.atoms.isEmpty() || mode.displacement.size() != molecule.atoms.size())
        return QStringLiteral("변위 정보 없음");
    double amplitude = 0;
    for (const auto &v : mode.displacement) {
        if (!finite(v)) return QStringLiteral("변위 정보 없음");
        amplitude = std::max(amplitude, double(v.length()));
    }
    if (amplitude == 0) return QStringLiteral("변위 없음");

    QVector<Contribution> contributions;
    for (const auto &bond : bonds) {
        const auto relative = mode.displacement[bond.second] - mode.displacement[bond.first];
        const double stretch = std::abs(QVector3D::dotProduct(relative, bond.unit));
        contributions.push_back({atomName(molecule, bond.first) + QStringLiteral("–")
                                 + atomName(molecule, bond.second), stretch});
    }
    // Bond-length derivatives miss pure bends. Compare the directions of bonds
    // sharing an atom; rigid translation and rotation leave these angles unchanged.
    for (int center = 0; center < neighbors.size(); ++center) {
        const auto &adjacent = neighbors[center];
        for (int a = 0; a < adjacent.size(); ++a) {
            for (int b = a + 1; b < adjacent.size(); ++b) {
                const auto &left = bonds[adjacent[a]];
                const auto &right = bonds[adjacent[b]];
                const int first = left.first == center ? left.second : left.first;
                const int last = right.first == center ? right.second : right.first;
                const QVector3D u = left.first == center ? left.unit : -left.unit;
                const QVector3D v = right.first == center ? right.unit : -right.unit;
                const auto du = mode.displacement[first] - mode.displacement[center];
                const auto dv = mode.displacement[last] - mode.displacement[center];
                const auto tangentU = (du - u * QVector3D::dotProduct(du, u)) / float(left.length);
                const auto tangentV = (dv - v * QVector3D::dotProduct(dv, v)) / float(right.length);
                const double cosine = std::clamp(double(QVector3D::dotProduct(u, v)), -1.0, 1.0);
                const double sine = QVector3D::crossProduct(u, v).length();
                const double angularRate = sine > 1.e-4
                    ? std::abs(QVector3D::dotProduct(tangentU, v) + QVector3D::dotProduct(u, tangentV)) / sine
                    : (tangentU - tangentV * float(cosine)).length();
                contributions.push_back({atomName(molecule, first) + QStringLiteral("–")
                    + atomName(molecule, center) + QStringLiteral("–") + atomName(molecule, last)
                    + QStringLiteral(" 굽힘"), angularRate * std::min(left.length, right.length)});
            }
        }
    }
    std::stable_sort(contributions.begin(), contributions.end(), [](const auto &a, const auto &b) {
        return a.score > b.score;
    });
    if (!contributions.isEmpty() && contributions.first().score > amplitude * 1.e-5) {
        QStringList labels;
        int significant = 0;
        for (const auto &entry : contributions) {
            if (entry.score < contributions.first().score * .55) break;
            if (significant++ < 2) labels.append(entry.label);
        }
        if (significant > 2) labels.append(QStringLiteral("외 %1개").arg(significant - 2));
        return labels.join(QStringLiteral(", "));
    }

    // Torsions and collective motions need not change a bond length or angle.
    // Identify their moving atoms without claiming a particular stretching bond.
    QVector<int> moving;
    for (int i = 0; i < mode.displacement.size(); ++i)
        if (mode.displacement[i].length() >= amplitude * .55) moving.push_back(i);
    std::stable_sort(moving.begin(), moving.end(), [&](int a, int b) {
        return mode.displacement[a].lengthSquared() > mode.displacement[b].lengthSquared();
    });
    QStringList labels;
    for (int i = 0; i < std::min(3, int(moving.size())); ++i) labels.append(atomName(molecule, moving[i]));
    if (moving.size() > 3) labels.append(QStringLiteral("외 %1개").arg(moving.size() - 3));
    return QStringLiteral("주요 원자: ") + labels.join(QStringLiteral(", "));
}
} // namespace

QVector<FrequencyAnnotation> frequencyAnnotations(const CalculationData &data) {
    if (data.frequencies.isEmpty()) return {};
    Molecule molecule = data.molecule;
    // Match the equilibrium coordinates used by frequency playback.
    if (!data.frames.isEmpty() && data.frames.last().coordinates.size() == molecule.atoms.size()) {
        for (int i = 0; i < molecule.atoms.size(); ++i)
            molecule.atoms[i].position = data.frames.last().coordinates[i];
    }
    if (molecule.bonds.isEmpty()) inferBonds(molecule);
    QVector<BondDirection> bonds;
    QVector<QVector<int>> neighbors(molecule.atoms.size());
    for (const auto &bond : molecule.bonds) {
        const int first = std::min(bond.first, bond.second), second = std::max(bond.first, bond.second);
        if (first < 0 || second >= molecule.atoms.size() || first == second) continue;
        const auto delta = molecule.atoms[second].position - molecule.atoms[first].position;
        const double length = delta.length();
        if (!finite(delta) || length < 1.e-6) continue;
        const int index = int(bonds.size());
        neighbors[first].append(index);
        neighbors[second].append(index);
        bonds.push_back({first, second, delta / float(length), length});
    }

    QVector<FrequencyAnnotation> result;
    result.reserve(data.frequencies.size());
    for (const auto &mode : data.frequencies) {
        FrequencyAnnotation entry;
        entry.atoms = describe(molecule, bonds, neighbors, mode);
        const QString value = std::isfinite(mode.wavenumber)
            ? QString::number(mode.wavenumber, 'f', 3) : QStringLiteral("—");
        entry.label = QStringLiteral("%1 cm⁻¹ (%2)%3").arg(value, entry.atoms,
            mode.imaginary || mode.wavenumber < 0 ? QStringLiteral(" [허수]") : QString());
        entry.tooltip = QStringLiteral("%1\n모드 %2 · 주요 진동 원자 자동 추정\n"
            "결합 길이·각도 변화와 원자 변위로 추정합니다. 여러 원자가 함께 움직이는 모드일 수 있습니다.\n"
            "원소 기호 뒤 숫자는 입력 구조의 원자 번호입니다.").arg(entry.label).arg(mode.number);
        result.push_back(entry);
    }
    return result;
}
} // namespace youngcraft
