#include "model/GeometryEditor.h"

#include <QQueue>
#include <QSet>
#include <QQuaternion>

#include <algorithm>
#include <cmath>
#include <limits>

namespace youngcraft {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1.0e-12;
const QString kRingWarning = QStringLiteral("고리 구조로 인해 fragment 분리가 불가능하여 선택 원자 기준으로 조정했습니다.");

struct Vec {
    double x{}, y{}, z{};
    Vec operator+(const Vec &v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec operator-(const Vec &v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec operator*(double s) const { return {x * s, y * s, z * s}; }
};

Vec vec(const QVector3D &v) { return {v.x(), v.y(), v.z()}; }
QVector3D qvec(const Vec &v) { return {float(v.x), float(v.y), float(v.z)}; }
double dot(const Vec &a, const Vec &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
Vec cross(const Vec &a, const Vec &b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }
double norm(const Vec &v) { return std::sqrt(dot(v,v)); }
Vec normalized(const Vec &v) { const double n=norm(v);return n>kEps?v*(1.0/n):Vec{}; }
bool valid(const Molecule &m,int i){return i>=0&&i<m.atoms.size();}
double wrap(double value){while(value<=-180.0)value+=360.0;while(value>180.0)value-=360.0;return value;}
double periodicError(double a,double b){return std::abs(wrap(a-b));}

QSet<int> component(const Molecule &m,int start,int cutA,int cutB){
    QSet<int> seen;if(!valid(m,start))return seen;QQueue<int> queue;queue.enqueue(start);seen.insert(start);
    while(!queue.isEmpty()){const int current=queue.dequeue();for(const Bond &bond:m.bonds){if((bond.first==cutA&&bond.second==cutB)||(bond.first==cutB&&bond.second==cutA))continue;int next=-1;if(bond.first==current)next=bond.second;else if(bond.second==current)next=bond.first;if(next>=0&&!seen.contains(next)){seen.insert(next);queue.enqueue(next);}}}
    return seen;
}

bool overlaps(const QSet<int>&a,const QSet<int>&b){for(int value:a)if(b.contains(value))return true;return false;}
void translate(Molecule&m,const QSet<int>&ids,const Vec&delta){for(int i:ids)m.atoms[i].position=qvec(vec(m.atoms[i].position)+delta);}
Vec rotateVector(const Vec&v,const Vec&axis,double radians){const Vec u=normalized(axis);const double c=std::cos(radians),s=std::sin(radians);return v*c+cross(u,v)*s+u*(dot(u,v)*(1.0-c));}
void rotate(Molecule&m,const QSet<int>&ids,const Vec&origin,const Vec&axis,double degrees){const double radians=degrees*kPi/180.0;for(int i:ids){const Vec relative=vec(m.atoms[i].position)-origin;m.atoms[i].position=qvec(origin+rotateVector(relative,axis,radians));}}
Vec safePerpendicular(const Vec&axis){Vec candidate=cross(axis,{1,0,0});if(norm(candidate)<kEps)candidate=cross(axis,{0,1,0});return normalized(candidate);}
bool finiteCoordinates(const Molecule&m){for(const Atom&a:m.atoms)if(!std::isfinite(a.position.x())||!std::isfinite(a.position.y())||!std::isfinite(a.position.z()))return false;return true;}

template<typename Measure,typename Transform>
bool chooseCandidate(Molecule&m,double target,Measure measure,Transform transform,bool periodic=false){
    Molecule plus=m,minus=m;transform(plus,1.0);transform(minus,-1.0);const double p=measure(plus),n=measure(minus);const double pe=periodic?periodicError(p,target):std::abs(p-target);const double ne=periodic?periodicError(n,target):std::abs(n-target);m=pe<=ne?std::move(plus):std::move(minus);return std::isfinite(pe)&&std::isfinite(ne)&&finiteCoordinates(m);
}

} // namespace

bool GeometryEditor::arrangeApproach(Molecule &m, int a, int b, double separation, QString *error) {
    auto fail = [&](const QString &message) { if (error) *error = message; return false; };
    if (!valid(m,a) || !valid(m,b) || a == b || !std::isfinite(separation) || separation < 0.5)
        return fail(QStringLiteral("서로 다른 반응 원자 두 개와 0.5 Å 이상의 거리를 지정하세요."));
    if (!finiteCoordinates(m)) return fail(QStringLiteral("유효하지 않은 좌표입니다."));
    const auto left = component(m,a,a,b), right = component(m,b,a,b);
    if (overlaps(left,right))
        return fail(QStringLiteral("두 원자가 고리 또는 다른 결합 경로로 연결되어 있습니다. 분리된 반응물 또는 분리 가능한 결합을 선택하세요."));
    Molecule candidate = m;
    const auto origin = m.atoms[a].position;
    QVector3D axis = (m.atoms[b].position-origin).normalized();
    if (axis.isNull()) axis = {1,0,0};
    auto orient = [&](const QSet<int> &ids, int center, const QVector3D &destination, const QVector3D &outward) {
        QVector3D neighbors;
        for (const auto &bond : m.bonds) {
            int other = bond.first == center ? bond.second : bond.second == center ? bond.first : -1;
            if (other >= 0 && ids.contains(other)) neighbors += m.atoms[other].position-m.atoms[center].position;
        }
        if (neighbors.lengthSquared()<1e-8f)
            for (int i : ids) if (i != center) neighbors += m.atoms[i].position-m.atoms[center].position;
        const auto q = neighbors.lengthSquared()>1e-8f ? QQuaternion::rotationTo(-neighbors.normalized(),outward) : QQuaternion();
        for (int i : ids) candidate.atoms[i].position = destination + q.rotatedVector(m.atoms[i].position-m.atoms[center].position);
        for (int i=0;i<candidate.coordinationSites.size();++i)
            if(ids.contains(candidate.coordinationSites[i].center))candidate.coordinationSites[i].direction=q.rotatedVector(m.coordinationSites[i].direction);
    };
    orient(left,a,origin,axis);
    const auto destination = origin+axis*float(separation);
    orient(right,b,destination,-axis);
    const Molecule aligned=candidate;
    double best=std::numeric_limits<double>::infinity();
    for (int step=0; step<72; ++step) {
        const auto roll=QQuaternion::fromAxisAndAngle(axis,float(step*5));
        for (int i:right) candidate.atoms[i].position=destination+roll.rotatedVector(aligned.atoms[i].position-destination);
        for (int i=0;i<candidate.coordinationSites.size();++i)
            if(right.contains(candidate.coordinationSites[i].center))candidate.coordinationSites[i].direction=roll.rotatedVector(aligned.coordinationSites[i].direction);
        double penalty=0;
        for (int i:right) for (int j=0;j<candidate.atoms.size();++j) {
            if (right.contains(j) || (i==b && j==a)) continue;
            const double d=(candidate.atoms[i].position-candidate.atoms[j].position).length();
            const double cutoff=0.80*(vanDerWaalsRadius(candidate.atoms[i].atomicNumber)+vanDerWaalsRadius(candidate.atoms[j].atomicNumber));
            if(d<cutoff) penalty+=(cutoff-d)*(cutoff-d);
        }
        if(penalty<best) { best=penalty; m=candidate; }
    }
    if(error) *error=best>0.05 ? QStringLiteral("배치 완료. 가까운 비결합 접촉이 남아 있으므로 확인 후 최적화하세요.") : QString();
    return true;
}

double GeometryEditor::distance(const Molecule&m,int a,int b){if(!valid(m,a)||!valid(m,b))return std::numeric_limits<double>::quiet_NaN();return norm(vec(m.atoms[b].position)-vec(m.atoms[a].position));}

double GeometryEditor::angleDegrees(const Molecule&m,int a,int b,int c){if(!valid(m,a)||!valid(m,b)||!valid(m,c))return std::numeric_limits<double>::quiet_NaN();const Vec u=vec(m.atoms[a].position)-vec(m.atoms[b].position),v=vec(m.atoms[c].position)-vec(m.atoms[b].position);const double denom=norm(u)*norm(v);if(denom<kEps)return std::numeric_limits<double>::quiet_NaN();return std::acos(std::clamp(dot(u,v)/denom,-1.0,1.0))*180.0/kPi;}

double GeometryEditor::dihedralDegrees(const Molecule&m,int a,int b,int c,int d){if(!valid(m,a)||!valid(m,b)||!valid(m,c)||!valid(m,d))return std::numeric_limits<double>::quiet_NaN();const Vec b0=vec(m.atoms[b].position)-vec(m.atoms[a].position),b1=vec(m.atoms[c].position)-vec(m.atoms[b].position),b2=vec(m.atoms[d].position)-vec(m.atoms[c].position);const Vec n1=cross(b0,b1),n2=cross(b1,b2);const double l1=norm(n1),l2=norm(n2),axis=norm(b1);if(l1<kEps||l2<kEps||axis<kEps)return std::numeric_limits<double>::quiet_NaN();const Vec u=normalized(b1),nn1=n1*(1.0/l1),nn2=n2*(1.0/l2);return wrap(std::atan2(dot(cross(nn1,nn2),u),dot(nn1,nn2))*180.0/kPi);}

bool GeometryEditor::setDistance(Molecule&m,int a,int b,double target,GeometryMoveMode mode,QString*warning){if(warning)warning->clear();const double current=distance(m,a,b);if(!std::isfinite(current)||current<kEps||target<0.1)return false;const Vec axis=normalized(vec(m.atoms[b].position)-vec(m.atoms[a].position));const double delta=target-current;QSet<int> sideB=component(m,b,a,b);const bool ring=sideB.contains(a);if(mode==GeometryMoveMode::OneSide){if(ring){sideB={b};if(warning)*warning=kRingWarning;}translate(m,sideB,axis*delta);}else{QSet<int> sideA=component(m,a,a,b);if(ring||sideA.contains(b)||overlaps(sideA,sideB)){sideA={a};sideB={b};if(warning)*warning=kRingWarning;}translate(m,sideA,axis*(-delta/2.0));translate(m,sideB,axis*(delta/2.0));}return finiteCoordinates(m)&&std::abs(distance(m,a,b)-target)<1.0e-4;}

bool GeometryEditor::setAngle(Molecule&m,int a,int b,int c,double target,GeometryMoveMode mode,QString*warning){if(warning)warning->clear();const double current=angleDegrees(m,a,b,c);if(!std::isfinite(current)||target<0.0||target>180.0)return false;const Vec origin=vec(m.atoms[b].position),u=vec(m.atoms[a].position)-origin,v=vec(m.atoms[c].position)-origin;Vec axis=normalized(cross(u,v));if(norm(axis)<kEps)axis=safePerpendicular(u);if(norm(axis)<kEps)return false;const double magnitude=std::abs(target-current);QSet<int> sideC=component(m,c,b,c);bool ring=sideC.contains(b);QSet<int> sideA;
    if(mode==GeometryMoveMode::BothSides){sideA=component(m,a,a,b);ring=ring||sideA.contains(b)||overlaps(sideA,sideC);}
    if(ring){sideA={a};sideC={c};if(warning)*warning=kRingWarning;}
    const Molecule original=m;auto measure=[a,b,c](const Molecule&x){return angleDegrees(x,a,b,c);};
    auto transform=[&](Molecule&x,double sign){if(mode==GeometryMoveMode::OneSide)rotate(x,sideC,origin,axis,sign*magnitude);else{rotate(x,sideA,origin,axis,sign*magnitude/2.0);rotate(x,sideC,origin,axis,-sign*magnitude/2.0);}};
    if(!chooseCandidate(m,target,measure,transform))return false;if(std::abs(angleDegrees(m,a,b,c)-target)>1.0e-3){m=original;return false;}return true;
}

bool GeometryEditor::setDihedral(Molecule&m,int a,int b,int c,int d,double target,GeometryMoveMode mode,QString*warning){if(warning)warning->clear();const double current=dihedralDegrees(m,a,b,c,d);if(!std::isfinite(current))return false;target=wrap(target);const double delta=wrap(target-current);const Vec origin=vec(m.atoms[b].position),axis=vec(m.atoms[c].position)-origin;if(norm(axis)<kEps)return false;QSet<int> sideD=component(m,d,b,c),sideA=component(m,a,b,c);bool ring=sideD.contains(b)||sideA.contains(c)||overlaps(sideA,sideD);if(ring){sideA={a};sideD={d};if(warning)*warning=kRingWarning;}const double magnitude=std::abs(delta);const Molecule original=m;auto measure=[a,b,c,d](const Molecule&x){return dihedralDegrees(x,a,b,c,d);};auto transform=[&](Molecule&x,double sign){if(mode==GeometryMoveMode::OneSide)rotate(x,sideD,origin,axis,sign*magnitude);else{rotate(x,sideA,origin,axis,sign*magnitude/2.0);rotate(x,sideD,origin,axis,-sign*magnitude/2.0);}};if(!chooseCandidate(m,target,measure,transform,true))return false;if(periodicError(dihedralDegrees(m,a,b,c,d),target)>1.0e-3){m=original;return false;}return true;}

} // namespace youngcraft
