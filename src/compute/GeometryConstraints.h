#pragma once
#include "model/Molecule.h"
#include "model/GeometryEditor.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <cmath>
#include <algorithm>

namespace youngcraft {
struct GeometryConstraint {
    QString type; // distance, angle, dihedral, fixed; zero-based atom indices
    QVector<int> atoms;
    double target = 0;
};
inline bool validateConstraints(const QVector<GeometryConstraint> &constraints, int count, QString *error) {
    QSet<QString> seen;
    for (const auto &c : constraints) {
        const int n = c.type == "fixed" ? 1 : c.type == "distance" ? 2 : c.type == "angle" ? 3 : c.type == "dihedral" ? 4 : 0;
        QSet<int> ids;
        QStringList key;
        for (int i : c.atoms) { ids.insert(i); key << QString::number(i); }
        QStringList reversed = key; std::reverse(reversed.begin(), reversed.end());
        const QString canonical = c.type + qMin(key.join(','), reversed.join(','));
        bool valid = n && c.atoms.size() == n && ids.size() == n && std::isfinite(c.target);
        for (int i : c.atoms) valid = valid && i >= 0 && i < count;
        valid = valid && (c.type == "fixed" ? true : c.type == "distance" ? c.target > 0 : c.type == "angle" ? c.target > 0 && c.target < 180 : c.target >= -180 && c.target <= 180);
        if (!valid || seen.contains(canonical)) {
            if (error) *error = QStringLiteral("Constraint의 원자 번호·목표값을 확인하세요. 중복 좌표, 같은 원자 반복, 0/180° 결합각은 사용할 수 없습니다.");
            return false;
        }
        seen.insert(canonical);
    }
    return true;
}
// Place the free side at the requested distance without moving any frozen atom.
inline bool prepareFixedDistances(Molecule &m, const QVector<GeometryConstraint> &entries, QString *error) {
    QSet<int> fixed;for(const auto &c:entries)if(c.type=="fixed")fixed.insert(c.atoms[0]);
    if(fixed.isEmpty())return true;
    Molecule prepared=m;
    for(const auto &c:entries)if(c.type=="distance") {
        if(std::abs(GeometryEditor::distance(prepared,c.atoms[0],c.atoms[1])-c.target)<0.0001)continue;
        bool accepted=false;
        for(int reverse=0;reverse<2&&!accepted;++reverse){
            Molecule candidate=prepared;QString warning;
            if(!GeometryEditor::setDistance(candidate,c.atoms[reverse],c.atoms[1-reverse],c.target,GeometryMoveMode::OneSide,&warning)||!warning.isEmpty())continue;
            bool unchanged=true;for(int i:fixed)if(candidate.atoms[i].position!=m.atoms[i].position)unchanged=false;
            if(unchanged){prepared=candidate;accepted=true;}
        }
        if(!accepted){if(error)*error=QStringLiteral("고정 구조를 유지하며 목표 거리로 배치할 수 없습니다. 움직일 쪽의 고정 또는 고리 연결을 확인하세요.");return false;}
    }
    for(const auto &c:entries)if(c.type=="distance"&&std::abs(GeometryEditor::distance(prepared,c.atoms[0],c.atoms[1])-c.target)>0.001){if(error)*error=QStringLiteral("거리 목표가 서로 충돌합니다. 시작 거리를 함께 만족할 수 없습니다.");return false;}
    m=prepared;return true;
}
inline QJsonArray constraintsJson(const QVector<GeometryConstraint> &constraints) {
    QJsonArray result;
    for (const auto &c : constraints) {
        QJsonArray ids; for (int i : c.atoms) ids.append(i);
        result.append(QJsonObject{{"type", c.type}, {"atoms", ids}, {"target", c.target}});
    }
    return result;
}
}
