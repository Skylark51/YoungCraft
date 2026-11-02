#include "model/MoleculeEditController.h"
#include "model/GeometryEditor.h"
#include <QQuaternion>
#include <cmath>
#include <algorithm>

namespace youngcraft {
void SelectionModel::click(int i, bool add, bool toggle) {
    if (!add && !toggle) clear();
    if (i < 0) return;
    if (toggle && atoms.contains(i)) atoms.remove(i); else atoms.insert(i);
}
QSet<int> MoleculeEditController::side(const Molecule &m, int start, int a, int b) {
    QSet<int> result; QVector<int> todo{start};
    while (!todo.isEmpty()) {
        int i = todo.takeLast();
        if (i < 0 || i >= m.atoms.size() || result.contains(i)) continue;
        result.insert(i);
        for (const Bond &bond : m.bonds) {
            if ((bond.first == a && bond.second == b) || (bond.first == b && bond.second == a)) continue;
            if (bond.first == i) todo << bond.second;
            if (bond.second == i) todo << bond.first;
        }
    }
    return result;
}
void SelectionModel::connected(const Molecule &m, int start) { clear(); atoms = MoleculeEditController::side(m,start,-1,-1); }
bool MoleculeEditController::rotatableBranch(const Molecule &m, int picked, QSet<int> &branch,
                                             QVector3D &pivot, QVector3D &axis) {
    branch.clear();
    if (picked < 0 || picked >= m.atoms.size()) return false;
    int root=picked;
    // A hydrogen click chooses the attached heavy group (e.g. CH3), not one H.
    if(m.atoms[root].atomicNumber==1){
        for(const Bond &bond:m.bonds){
            const int n=bond.first==root?bond.second:bond.second==root?bond.first:-1;
            if(n>=0&&m.atoms[n].atomicNumber!=1){root=n;break;}
        }
    }
    int parent=-1;
    QSet<int> candidate;
    for(const Bond &bond:m.bonds){
        const int neighbor=bond.first==root?bond.second:bond.second==root?bond.first:-1;
        if(neighbor<0||m.atoms[neighbor].atomicNumber==1)continue;
        QSet<int> sideAtoms=side(m,root,root,neighbor);
        if(sideAtoms.contains(neighbor))continue; // closed ring; no safe cut
        if(parent<0||sideAtoms.size()<candidate.size()){parent=neighbor;candidate=sideAtoms;}
    }
    if(parent<0)return false;
    const QVector3D direction=m.atoms[root].position-m.atoms[parent].position;
    if(direction.lengthSquared()<1e-8f)return false;
    branch=candidate;
    pivot=m.atoms[parent].position;
    axis=direction.normalized();
    return true;
}
bool MoleculeEditController::validate(const Molecule &m, QString *error) {
    auto fail = [&](const QString &s) { if (error) *error = s; return false; };
    QSet<quint64> pairs;
    for (const Atom &a : m.atoms) {
        if (a.atomicNumber < 1 || a.atomicNumber > 118) return fail("유효하지 않은 원소 / 더미 원자");
        if (!std::isfinite(a.position.x()) || !std::isfinite(a.position.y()) || !std::isfinite(a.position.z())) return fail("좌표가 유한한 값이 아닙니다.");
        if (a.formalCharge < -4 || a.formalCharge > 4) return fail("유효하지 않은 형식 전하");
    }
    for (const Bond &b : m.bonds) {
        if (b.first < 0 || b.second < 0 || b.first >= m.atoms.size() || b.second >= m.atoms.size() || b.first == b.second) return fail("유효하지 않은 결합 끝점");
        if (!std::isfinite(b.order) || b.order < 1 || b.order > 3) return fail("유효하지 않은 결합 차수");
        quint64 key = (quint64(qMin(b.first,b.second)) << 32) | quint32(qMax(b.first,b.second));
        if (pairs.contains(key)) return fail("중복된 결합"); pairs.insert(key);
    }
    for (const auto &s : m.coordinationSites) if (s.center < 0 || s.center >= m.atoms.size() || !std::isfinite(s.direction.length()) || s.direction.lengthSquared() < 0.5f || (s.occupied && (s.donor<0 || s.donor>=m.atoms.size()))) return fail("유효하지 않은 연결 자리");
    for (const auto &h : m.hapticContacts) {
        if (h.metal < 0 || h.metal >= m.atoms.size()) return fail("유효하지 않은 다중 원자 배위 중심");
        for (int i : h.atoms) if (i < 0 || i >= m.atoms.size()) return fail("유효하지 않은 다중 원자 배위 연결");
    }
    return true;
}
double MoleculeEditController::idealLength(int a,int b,double order) {
    if (a > b) std::swap(a,b);
    // Representative equilibrium lengths (angstrom); uncommon pairs use radii.
    struct Entry { int a,b; double single,dbl,triple; };
    static const Entry table[]={{6,6,1.54,1.34,1.20},{6,7,1.47,1.28,1.16},{6,8,1.43,1.23,1.13},
        {7,7,1.45,1.25,1.10},{7,8,1.40,1.21,0},{8,8,1.48,1.21,0},{1,6,1.09,0,0},{1,7,1.01,0,0},{1,8,0.96,0,0}};
    for (auto e : table) if (e.a==a && e.b==b) { double v=order>=2.5?e.triple:order>=1.75?e.dbl:e.single; if (v>0) return v; }
    if (a<1 || b>118) return 0;
    // Radius data currently covers these elements; avoid invented fallback values.
    auto known=[](int z){return z<=36 || z==53 || z==78 || z==79 || z==80;};
    if (!known(a) || !known(b)) return 0;
    return (covalentRadius(a)+covalentRadius(b)) * (isMetal(a)||isMetal(b)?1.0:order>=2.5?0.80:order>=1.75?0.88:1.0);
}
double MoleculeEditController::predictOrder(int a,int b,double d) {
    if (isMetal(a)||isMetal(b)||!std::isfinite(d)) return 1;
    // Conservative: distance inference only for common multiple-bond partners.
    if ((a!=6 && a!=7 && a!=8) || (b!=6 && b!=7 && b!=8)) return 1;
    double best=1, error=0.065;
    for (int o=2;o<=3;++o) {
        if(o==3 && !((a==6 && (b==6||b==7||b==8)) || (b==6 && (a==7||a==8)) || (a==7&&b==7)))continue;
        double l=idealLength(a,b,o); double e=std::abs(d-l); if(l>0 && e<error) {best=o;error=e;} }
    return best;
}
QVector3D MoleculeEditController::snappedPosition(const QVector3D &o,const QVector3D &p,int a,int b,double order,bool alt) {
    if (alt) return p;
    double l=idealLength(a,b,order==0?predictOrder(a,b,(p-o).length()):order);
    if(l<=0) return p;
    QVector3D v=p-o; if(v.lengthSquared()<1e-8f) v={1,0,0};
    return o+v.normalized()*float(l);
}
int MoleculeEditController::addAtom(Molecule &m,const QVector3D &p,const DrawState &s) {
    Atom a; a.index=m.atoms.size()+1; a.atomicNumber=s.element; a.element=elementSymbol(s.element); a.position=p;a.formalCharge=s.formalCharge;
    m.charge+=s.formalCharge;m.atoms<<a;return m.atoms.size()-1;
}
bool MoleculeEditController::replace(Molecule &m,const QSet<int>&ids,int z,int charge) {
    if(z<1||z>118||charge<-4||charge>4) return false;
    for(int i:ids) if(i>=0&&i<m.atoms.size()){auto &a=m.atoms[i];m.charge+=charge-a.formalCharge;a.atomicNumber=z;a.element=elementSymbol(z);a.formalCharge=charge;a.autoHydrogen=false;a.autoHydrogenLocked=false;a.colorOverride=QColor();a.radiusScale=1;}
    return true;
}
bool MoleculeEditController::drawBond(Molecule &m,int a,int b,const DrawState&s) {
    if(a<0||b<0||a==b||a>=m.atoms.size()||b>=m.atoms.size()) return false;
    if(s.bondOrder==0 && m.bondIndex(a,b)>=0) return true;
    double order=s.bondOrder==0?predictOrder(m.atoms[a].atomicNumber,m.atoms[b].atomicNumber,(m.atoms[a].position-m.atoms[b].position).length()):s.bondOrder;
    bool metal=isMetal(m.atoms[a].atomicNumber)||isMetal(m.atoms[b].atomicNumber);
    m.atoms[a].autoHydrogen=false;m.atoms[b].autoHydrogen=false;
    m.setBond(a,b,order,metal,metal||order==1.5?BondStyle::Dashed:BondStyle::Solid,true);return true;
}
void MoleculeEditController::hydrogens(Molecule &m,HydrogenAction action) {
    if(action==HydrogenAction::RemoveAll) { for(int i=m.atoms.size()-1;i>=0;--i)if(m.atoms[i].atomicNumber==1)m.removeAtom(i);return; }
    if(action==HydrogenAction::Add){m.reconcileAutoHydrogens(true,false);return;}
    // Explicit adjustment can replace singly bound main-group hydrogen.
    for(int i=0;i<m.atoms.size();++i)if(m.atoms[i].atomicNumber==1) {
        int count=0;bool metal=false,locked=false;for(auto b:m.bonds)if(b.first==i||b.second==i){++count;auto &neighbor=m.atoms[b.first==i?b.second:b.first];metal|=isMetal(neighbor.atomicNumber);locked|=neighbor.autoHydrogenLocked;}
        if(count==1&&!metal&&(!locked||action==HydrogenAction::RemoveExtra)) m.atoms[i].autoHydrogen=true;
    }
    m.reconcileAutoHydrogens(action==HydrogenAction::Add,action==HydrogenAction::RemoveExtra);
}
void MoleculeEditController::transform(Molecule &m,const QSet<int>&ids,const QVector3D&t,const QVector3D&r,const QVector3D&c) {
    auto q=QQuaternion::fromEulerAngles(r);
    for(int i:ids)if(i>=0&&i<m.atoms.size())m.atoms[i].position=c+q.rotatedVector(m.atoms[i].position-c)+t;
    for(auto &s:m.coordinationSites)if(ids.contains(s.center))s.direction=q.rotatedVector(s.direction);
}
bool MoleculeEditController::bondGeometry(Molecule&m,int a,int b,int kind,double value,bool opposite,QString*error) {
    if(m.bondIndex(a,b)<0)return false;
    auto sa=side(m,a,a,b),sb=side(m,b,a,b);
    if(sa.contains(b)){if(error)*error="고리 결합: 분자 조각을 안전하게 분리할 수 없습니다.";return false;}
    bool moveA=sa.size()<sb.size();if(opposite)moveA=!moveA;if(moveA)std::swap(a,b);
    if(kind==0)return GeometryEditor::setDistance(m,a,b,value,GeometryMoveMode::OneSide,error);
    int outerA=-1,outerB=-1;
    for(auto bond:m.bonds){int n=bond.first==a?bond.second:bond.second==a?bond.first:-1;if(n>=0&&n!=b)outerA=n;n=bond.first==b?bond.second:bond.second==b?bond.first:-1;if(n>=0&&n!=a)outerB=n;}
    if(kind==1&&outerA>=0)return GeometryEditor::setAngle(m,outerA,a,b,value,GeometryMoveMode::OneSide,error);
    if(kind==2&&outerA>=0&&outerB>=0)return GeometryEditor::setDihedral(m,outerA,a,b,outerB,value,GeometryMoveMode::OneSide,error);
    if(error)*error="각도 / 비틀림을 조절하려면 인접 원자가 있는 결합을 선택하세요.";return false;
}
}
