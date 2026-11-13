#include "model/StructureTools.h"
#include "model/GeometryEditor.h"
#include <QDataStream>
#include <QIODevice>
#include <QtMath>
#include <QRegularExpression>
#include <cmath>

namespace youngcraft {
QSet<int> StructureTools::selection(const Molecule &m, const QSet<int> &ids) {
    QSet<int> result;
    for (int i : ids) if (i >= 0 && i < m.atoms.size()) result.insert(i);
    return result;
}
QSet<int> StructureTools::expand(const Molecule &m, const QSet<int> &ids, bool connected) {
    auto result = selection(m, ids);
    QVector<QVector<int>> neighbors(m.atoms.size());
    auto link = [&](int a, int b) {
        if(a<0||b<0||a>=neighbors.size()||b>=neighbors.size())return;
        neighbors[a]<<b;neighbors[b]<<a;
    };
    for(const auto &b:m.bonds)link(b.first,b.second);
    // Haptic contacts connect an entire ligand to its metal without artificial bonds.
    for(const auto &h:m.hapticContacts)for(int i:h.atoms)link(h.metal,i);
    QVector<int> queue;for(int i:result)queue<<i;
    const int seeds=queue.size();
    for(int head=0;head<(connected?queue.size():seeds);++head) {
        for(int i:neighbors[queue[head]])if(!result.contains(i)){result.insert(i);queue<<i;}
    }
    return result;
}
QSet<int> StructureTools::invert(const Molecule &m, const QSet<int> &ids) {
    QSet<int> result;
    for (int i = 0; i < m.atoms.size(); ++i) if (!ids.contains(i)) result.insert(i);
    return result;
}
Molecule StructureTools::fragment(const Molecule &m, const QSet<int> &ids) {
    Molecule result; result.title = m.title;
    QVector<int> map(m.atoms.size(), -1);
    for (int i = 0; i < m.atoms.size(); ++i) if (ids.contains(i)) {
        map[i] = result.atoms.size();
        auto a = m.atoms[i]; a.index = result.atoms.size() + 1;
        result.charge += a.formalCharge; result.atoms << a;
    }
    auto mapped = [&](int i) { return i >= 0 && i < map.size() ? map[i] : -1; };
    for (auto b : m.bonds) if (mapped(b.first) >= 0 && mapped(b.second) >= 0) {
        b.first = mapped(b.first); b.second = mapped(b.second); result.bonds << b;
    }
    for (auto s : m.coordinationSites) if (mapped(s.center) >= 0) {
        s.center = mapped(s.center); s.donor = mapped(s.donor);
        if (s.donor < 0) s.occupied = false;
        result.coordinationSites << s;
    }
    for (auto h : m.hapticContacts) {
        bool complete = mapped(h.metal) >= 0;
        for (int i : h.atoms) complete = complete && mapped(i) >= 0;
        if (!complete) continue;
        h.metal = mapped(h.metal);
        for (int &i : h.atoms) i = mapped(i);
        result.hapticContacts << h;
    }
    if (result.atoms.size() == m.atoms.size()) { result.charge = m.charge; result.multiplicity = m.multiplicity; }
    return result;
}
QVector<QSet<int>> StructureTools::components(const Molecule &m) {
    QVector<QSet<int>> result; QSet<int> visited;
    for (int i = 0; i < m.atoms.size(); ++i) if (!visited.contains(i)) {
        auto group = expand(m, {i}, true);
        visited.unite(group); result << group;
    }
    return result;
}
QByteArray StructureTools::encode(const Molecule &m) {
    QByteArray bytes; QDataStream s(&bytes, QIODevice::WriteOnly); s.setVersion(QDataStream::Qt_6_4);
    s << quint32(0x59434631) << m.title << m.charge << m.multiplicity << qint32(m.atoms.size());
    for (const auto &a : m.atoms)
        s << a.atomicNumber << a.position << a.charge << a.spinDensity << a.force << a.hasForce
          << a.visible << a.label << a.colorOverride << a.radiusScale << a.autoHydrogen
          << a.occupancy << a.disorderGroup << a.formalCharge << a.autoHydrogenLocked;
    s << qint32(m.bonds.size());
    for (const auto &b : m.bonds) s << b.first << b.second << b.order << b.coordination << int(b.style) << b.manual;
    s << qint32(m.coordinationSites.size());
    for (const auto &site : m.coordinationSites) s << site.center << site.direction << site.occupied << site.donor;
    s << qint32(m.hapticContacts.size());
    for (const auto &h : m.hapticContacts) s << h.metal << h.atoms << h.name;
    return bytes;
}
bool StructureTools::decode(const QByteArray &bytes, Molecule &m, QString *error) {
    auto fail = [&] { if (error) *error = QStringLiteral("유효한 Youngcraft molecule fragment이 아닙니다."); return false; };
    if (bytes.size() > 16 * 1024 * 1024 || bytes.isEmpty()) return fail();
    QDataStream s(bytes); s.setVersion(QDataStream::Qt_6_4);
    Molecule result; quint32 magic; qint32 count;
    s >> magic; if (magic != 0x59434631) return fail();
    s >> result.title >> result.charge >> result.multiplicity >> count;
    if (count < 1 || count > 100000) return fail();
    for (int i = 0; i < count; ++i) {
        Atom a; a.index = i + 1;
        s >> a.atomicNumber >> a.position >> a.charge >> a.spinDensity >> a.force >> a.hasForce
          >> a.visible >> a.label >> a.colorOverride >> a.radiusScale >> a.autoHydrogen
          >> a.occupancy >> a.disorderGroup >> a.formalCharge >> a.autoHydrogenLocked;
        a.element = elementSymbol(a.atomicNumber); result.atoms << a;
        if (s.status() != QDataStream::Ok) return fail();
    }
    s >> count; if (count < 0 || count > 1000000) return fail();
    for (int i = 0; i < count; ++i) {
        Bond b; int style;
        s >> b.first >> b.second >> b.order >> b.coordination >> style >> b.manual;
        if (style < 0 || style > 1 || s.status() != QDataStream::Ok) return fail();
        b.style = BondStyle(style); result.bonds << b;
    }
    s >> count; if (count < 0 || count > 100000) return fail();
    for (int i = 0; i < count; ++i) { CoordinationSite site; s >> site.center >> site.direction >> site.occupied >> site.donor; result.coordinationSites << site; }
    s >> count; if (count < 0 || count > 100000) return fail();
    for (int i = 0; i < count; ++i) { HapticContact h; s >> h.metal >> h.atoms >> h.name; result.hapticContacts << h; }
    if (s.status() != QDataStream::Ok || !s.atEnd() || result.multiplicity < 1 || !MoleculeEditController::validate(result)) return fail();
    m = result; return true;
}
bool StructureTools::append(Molecule &target, const Molecule &f, const QVector3D &offset, QString *error) {
    if (f.isEmpty() || !MoleculeEditController::validate(f, error)) return false;
    Molecule result = target; const int base = result.atoms.size();
    for (auto a : f.atoms) { a.index = result.atoms.size() + 1; a.position += offset; result.atoms << a; }
    for (auto b : f.bonds) { b.first += base; b.second += base; result.bonds << b; }
    for (auto site : f.coordinationSites) { site.center += base; if (site.donor >= 0) site.donor += base; result.coordinationSites << site; }
    for (auto h : f.hapticContacts) { h.metal += base; for (int &i : h.atoms) i += base; result.hapticContacts << h; }
    result.charge += f.charge;
    if (target.isEmpty()) result.multiplicity = f.multiplicity;
    if (!MoleculeEditController::validate(result, error)) return false;
    target = result; return true;
}
QString StructureTools::xyz(const Molecule &m) {
    QString result = QString::number(m.atoms.size()) + '\n' + QString(m.title).replace('\n', ' ').replace('\r', ' ') + '\n';
    for (const auto &a : m.atoms) result += QStringLiteral("%1  %2  %3  %4\n").arg(elementSymbol(a.atomicNumber)).arg(a.position.x(),0,'f',8).arg(a.position.y(),0,'f',8).arg(a.position.z(),0,'f',8);
    return result;
}
QString StructureTools::zMatrix(const Molecule &m, QString *error) {
    auto fail = [&](const QString &message) { if (error) *error = message; return QString(); };
    if (error) error->clear();
    if (m.isEmpty() || !MoleculeEditController::validate(m, error)) return {};
    QStringList lines;
    for (int i = 0; i < m.atoms.size(); ++i) {
        QString line = elementSymbol(m.atoms[i].atomicNumber);
        if (i == 0) { lines << line; continue; }
        int a = -1, b = -1, c = -1; double best = 1e100;
        // Prefer bonded preceding atoms, but avoid zero-length references.
        for (int j = 0; j < i; ++j) {
            double d = GeometryEditor::distance(m,i,j);
            double score = d + (m.bondIndex(i,j) < 0 ? 10000 : 0);
            if (d > 1e-6 && score < best) { a = j; best = score; }
        }
        if (a < 0) return fail(QStringLiteral("원자 %1: 겹친 좌표로 Z-matrix를 생성할 수 없습니다.").arg(i+1));
        line += QString("  %1  %2").arg(a+1).arg(GeometryEditor::distance(m,i,a),0,'f',8);
        if (i >= 2) {
            double bestScore = -1;
            for (int j = 0; j < i; ++j) if (j != a) {
                double angle = GeometryEditor::angleDegrees(m,i,a,j);
                if (!std::isfinite(angle)) continue;
                double score = std::abs(std::sin(qDegreesToRadians(angle)));
                if (i == 2) { b = j; break; }
                for (int k = 0; k < i; ++k) if (k != a && k != j) {
                    double referenceAngle = GeometryEditor::angleDegrees(m,a,j,k);
                    double dihedral = GeometryEditor::dihedralDegrees(m,i,a,j,k);
                    double quality = score * std::abs(std::sin(qDegreesToRadians(referenceAngle)));
                    if (std::isfinite(dihedral) && quality > 1e-6 && quality > bestScore) { b = j; c = k; bestScore = quality; }
                }
            }
            if (b < 0) return fail(QStringLiteral("원자 %1: 일직선 기준 원자에서는 이면각을 정의할 수 없습니다. Cartesian 입력을 사용하세요.").arg(i+1));
            line += QString("  %1  %2").arg(b+1).arg(GeometryEditor::angleDegrees(m,i,a,b),0,'f',8);
        }
        if (i >= 3) line += QString("  %1  %2").arg(c+1).arg(GeometryEditor::dihedralDegrees(m,i,a,b,c),0,'f',8);
        lines << line;
    }
    return lines.join('\n') + '\n';
}
QString StructureTools::gaussianInput(const Molecule &m, const QString &route, int charge, int multiplicity, QString *error) {
    if (error) error->clear();
    auto fail = [&](const QString &message) { if (error) *error = message; return QString(); };
    if (m.isEmpty() || !MoleculeEditController::validate(m, error)) return fail(QStringLiteral("유효한 molecule 구조가 필요합니다."));
    QString normalized = route.simplified();
    if (!normalized.startsWith('#') || normalized.contains('%') || normalized.contains(QRegularExpression("--\\s*link1\\s*--", QRegularExpression::CaseInsensitiveOption))) return fail(QStringLiteral("#으로 시작하는 단일 계산 경로를 입력하세요."));
    if (normalized.contains(QRegularExpression("\\b(geom\\s*=|gen(ecp)?\\b|chkbasis|pseudo\\s*=\\s*read|guess\\s*=\\s*\\(?\\s*read|opt\\s*=\\s*\\(?\\s*modredundant|units\\s*=\\s*\\(?\\s*(bohr|au))",QRegularExpression::CaseInsensitiveOption))) return fail(QStringLiteral("추가 입력 구역/체크포인트 또는 다른 좌표 단위가 필요한 경로입니다. 독립적인 Cartesian(Å) 계산 경로를 지정하세요."));
    int electrons = -charge; for (const auto &a : m.atoms) electrons += a.atomicNumber;
    if (multiplicity < 1 || electrons < 0 || multiplicity-1 > electrons || (electrons-(multiplicity-1))%2 != 0) return fail(QStringLiteral("총전하와 스핀 다중도가 전자 수와 맞지 않습니다."));
    QString title = m.title.simplified(); if (title.isEmpty()) title = "Youngcraft structure";
    QString result = normalized + "\n\n" + title + "\n\n" + QString("%1 %2\n").arg(charge).arg(multiplicity);
    for (const auto &a : m.atoms) result += QStringLiteral("%1  %2  %3  %4\n").arg(elementSymbol(a.atomicNumber)).arg(a.position.x(),0,'f',8).arg(a.position.y(),0,'f',8).arg(a.position.z(),0,'f',8);
    return result + '\n';
}
}
