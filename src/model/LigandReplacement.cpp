#include "model/LigandReplacement.h"
#include "model/MoleculeEditController.h"
#include <QQuaternion>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace youngcraft {
namespace {
// Match heavy-atom connectivity, including which atoms coordinate the metal.
// Explicit hydrogen counts and inferred bond orders may differ in calculation files.
bool matches(const Molecule &m, const LigandCandidate &c, const Fragment &f) {
    QVector<int> a, b;
    for(int i=0;i<m.atoms.size();++i)if(c.atoms.contains(i)&&m.atoms[i].atomicNumber>1)a<<i;
    for(int i=0;i<f.atoms.size();++i)if(f.atoms[i].atomicNumber>1)b<<i;
    if(a.size()!=b.size()||c.donors.size()!=f.attachments.size())return false;
    const int n=a.size();QVector<QVector<bool>> ga(n,QVector<bool>(n)),gb=ga;
    QVector<int> da(n),db(n),mapping(n,-1);QVector<bool> used(n);
    for(int i=0;i<n;++i)for(int j=0;j<i;++j){
        ga[i][j]=ga[j][i]=m.bondIndex(a[i],a[j])>=0;
        bool edge=false;for(const auto &bond:f.bonds)if((bond.first==b[i]&&bond.second==b[j])||(bond.second==b[i]&&bond.first==b[j])){edge=true;break;}
        gb[i][j]=gb[j][i]=edge;
        if(ga[i][j]){++da[i];++da[j];}if(edge){++db[i];++db[j];}
    }
    int budget=100000;
    std::function<bool(int)> search=[&](int depth){
        if(--budget<0)return false;if(depth==n)return true;
        for(int j=0;j<n;++j){
            if(used[j]||m.atoms[a[depth]].atomicNumber!=f.atoms[b[j]].atomicNumber||da[depth]!=db[j]||
                c.donors.contains(a[depth])!=f.attachments.contains(b[j]))continue;
            bool valid=true;for(int k=0;k<depth;++k)if(ga[depth][k]!=gb[j][mapping[k]]){valid=false;break;}
            if(!valid)continue;mapping[depth]=j;used[j]=true;
            if(search(depth+1))return true;used[j]=false;
        }return false;
    };
    return search(0);
}

QQuaternion fitRotation(const QVector<QVector3D> &from,const QVector<QVector3D> &to) {
    if(from.size()==1)return QQuaternion::rotationTo(from[0],to[0]);
    double s[3][3]={};
    for(int k=0;k<from.size();++k)for(int i=0;i<3;++i)for(int j=0;j<3;++j)s[i][j]+=from[k][i]*to[k][j];
    double n[4][4]={{s[0][0]+s[1][1]+s[2][2],s[1][2]-s[2][1],s[2][0]-s[0][2],s[0][1]-s[1][0]},
        {s[1][2]-s[2][1],s[0][0]-s[1][1]-s[2][2],s[0][1]+s[1][0],s[2][0]+s[0][2]},
        {s[2][0]-s[0][2],s[0][1]+s[1][0],-s[0][0]+s[1][1]-s[2][2],s[1][2]+s[2][1]},
        {s[0][1]-s[1][0],s[2][0]+s[0][2],s[1][2]+s[2][1],-s[0][0]-s[1][1]+s[2][2]}};
    double v[4][4]={};for(int i=0;i<4;++i)v[i][i]=1;
    for(int iter=0;iter<60;++iter){
        int p=0,q=1;for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)if(std::abs(n[i][j])>std::abs(n[p][q])){p=i;q=j;}
        if(std::abs(n[p][q])<1e-12)break;
        const double theta=0.5*std::atan2(2*n[p][q],n[q][q]-n[p][p]);
        const double c=std::cos(theta),sine=std::sin(theta);
        const double pp=n[p][p],qq=n[q][q],pq=n[p][q];
        for(int i=0;i<4;++i)if(i!=p&&i!=q){const double ip=n[i][p],iq=n[i][q];n[i][p]=n[p][i]=c*ip-sine*iq;n[i][q]=n[q][i]=sine*ip+c*iq;}
        n[p][p]=c*c*pp-2*c*sine*pq+sine*sine*qq;n[q][q]=sine*sine*pp+2*c*sine*pq+c*c*qq;n[p][q]=n[q][p]=0;
        for(int i=0;i<4;++i){const double ip=v[i][p],iq=v[i][q];v[i][p]=c*ip-sine*iq;v[i][q]=sine*ip+c*iq;}
    }
    int best=0;for(int i=1;i<4;++i)if(n[i][i]>n[best][best])best=i;
    return QQuaternion(float(v[0][best]),float(v[1][best]),float(v[2][best]),float(v[3][best])).normalized();
}
}

QVector<LigandCandidate> LigandReplacement::recognize(const Molecule &m,int metal,
    const QVector<Fragment> &library,const QSet<int> &protectedAtoms) {
    QVector<LigandCandidate> result;
    if(metal<0||metal>=m.atoms.size()||!isMetal(m.atoms[metal].atomicNumber))return result;
    QVector<QVector<int>> adjacency(m.atoms.size());QSet<int> donors;
    for(const auto &b:m.bonds){
        if(b.first<0||b.second<0||b.first>=m.atoms.size()||b.second>=m.atoms.size())return {};
        if(b.first==metal)donors.insert(b.second);else if(b.second==metal)donors.insert(b.first);
        if(!isMetal(m.atoms[b.first].atomicNumber)&&!isMetal(m.atoms[b.second].atomicNumber)){
            adjacency[b.first]<<b.second;adjacency[b.second]<<b.first;
        }
    }
    // Haptic/bridging ligands need a separate replacement rule; never partly remove them.
    QSet<int> visited;
    for(int seed=0;seed<m.atoms.size();++seed){
        if(!donors.contains(seed)||visited.contains(seed)||isMetal(m.atoms[seed].atomicNumber))continue;
        LigandCandidate c;c.metal=metal;QVector<int> queue{seed};visited.insert(seed);
        for(int k=0;k<queue.size();++k){int i=queue[k];c.atoms.insert(i);
            if(protectedAtoms.contains(i)||m.atoms[i].optimizationFixed)c.protectedCore=true;
            for(int j:adjacency[i])if(!visited.contains(j)){visited.insert(j);queue<<j;}
        }
        bool unsupported=false;
        for(const auto &bond:m.bonds){int other=-1;
            if(c.atoms.contains(bond.first)&&!c.atoms.contains(bond.second))other=bond.second;
            if(c.atoms.contains(bond.second)&&!c.atoms.contains(bond.first))other=bond.first;
            if(other>=0&&other!=metal)unsupported=true;
        }
        for(const auto &h:m.hapticContacts)for(int i:h.atoms)if(c.atoms.contains(i))unsupported=true;
        if(unsupported)continue;
        for(int i=0;i<m.atoms.size();++i)if(c.atoms.contains(i)&&donors.contains(i))c.donors<<i;
        QStringList names;for(const auto &f:library)if((f.ligand||f.category=="lab-ligand")&&matches(m,c,f))names<<f.name;
        c.knownTemplate=!names.isEmpty();
        c.name=names.isEmpty()?QStringLiteral("미등록 리간드"):names.join(" / ");result<<c;
    }
    return result;
}

bool LigandReplacement::replace(const Molecule &source,const LigandCandidate &candidate,
    const Fragment &f,const QSet<int> &protectedAtoms,Molecule &result,QString *error) {
    auto fail=[&](const QString &message){if(error)*error=message;return false;};
    if(!MoleculeEditController::validate(source,error))return false;
    LigandCandidate c;bool found=false;
    for(const auto &actual:recognize(source,candidate.metal,{},protectedAtoms))if(actual.atoms==candidate.atoms){c=actual;found=true;break;}
    if(!found)return fail(QStringLiteral("금속에서 분리 가능한 리간드를 다시 선택하세요. 가교·haptic 리간드는 지원하지 않습니다."));
    if(c.protectedCore)return fail(QStringLiteral("반응 중심 또는 고정 원자를 포함한 리간드는 교체할 수 없습니다."));
    if(!(f.ligand||f.category=="lab-ligand")||f.dummyAtoms.size()!=1||!f.hapticAtoms.isEmpty()||
        f.attachments.isEmpty()||f.attachments.size()!=c.donors.size()||f.attachments.size()>6)
        return fail(QStringLiteral("동일한 주개 수(1–6개)와 금속 기준점 하나를 가진 리간드 템플릿이 필요합니다."));
    const int dummy=f.dummyAtoms[0];
    if(dummy<0||dummy>=f.atoms.size())return fail(QStringLiteral("잘못된 템플릿 기준점입니다."));
    Molecule templateMolecule;templateMolecule.atoms=f.atoms;templateMolecule.bonds=f.bonds;
    templateMolecule.atoms[dummy].atomicNumber=source.atoms[c.metal].atomicNumber;
    if(!MoleculeEditController::validate(templateMolecule,error))return false;
    const auto templateParts=recognize(templateMolecule,dummy);
    if(templateParts.size()!=1||templateParts[0].atoms.size()!=f.atoms.size()-1||
        QSet<int>(templateParts[0].donors.begin(),templateParts[0].donors.end())!=QSet<int>(f.attachments.begin(),f.attachments.end()))
        return fail(QStringLiteral("템플릿은 금속 기준점에 연결된 리간드 하나만 포함해야 합니다."));
    const auto anchor=f.atoms[dummy].position,origin=source.atoms[c.metal].position;
    for(const auto &a:f.atoms)if(!std::isfinite(a.position.x())||!std::isfinite(a.position.y())||!std::isfinite(a.position.z())||isMetal(a.atomicNumber))
        return fail(QStringLiteral("템플릿 좌표와 원소를 확인하세요."));
    QVector<QVector3D> from;for(int i:f.attachments){
        if(i<0||i>=f.atoms.size()||f.atoms[i].atomicNumber<1||(f.atoms[i].position-anchor).lengthSquared()<1e-8f)
            return fail(QStringLiteral("잘못된 템플릿 연결점입니다."));
        from<<(f.atoms[i].position-anchor).normalized();
    }
    QVector<int> permutation=c.donors;std::sort(permutation.begin(),permutation.end());
    double bestScore=std::numeric_limits<double>::infinity();QQuaternion best;
    do {
        QVector<QVector3D> to;bool compatible=true;double elementPenalty=0;
        for(int k=0;k<permutation.size();++k){
            if(f.atoms[f.attachments[k]].atomicNumber!=source.atoms[permutation[k]].atomicNumber)elementPenalty+=0.05;
            const auto direction=source.atoms[permutation[k]].position-origin;
            if(direction.lengthSquared()<1e-8f)compatible=false;to<<direction.normalized();
        }
        if(!compatible)continue;const auto alignment=fitRotation(from,to);
        // A single donor leaves a free torsion: search it without moving the core.
        for(int step=0;step<(from.size()==1?24:1);++step){
            const auto rotation=QQuaternion::fromAxisAndAngle(to[0],float(step*15))*alignment;
            double score=elementPenalty;for(int k=0;k<from.size();++k)score+=100*(rotation.rotatedVector(from[k])-to[k]).lengthSquared();
            for(const auto &atom:f.atoms)if(atom.atomicNumber>0){const auto p=origin+rotation.rotatedVector(atom.position-anchor);
                for(int i=0;i<source.atoms.size();++i)if(i!=c.metal&&!c.atoms.contains(i)){
                    double cutoff=0.75*(covalentRadius(atom.atomicNumber)+covalentRadius(source.atoms[i].atomicNumber));
                    double overlap=cutoff-(p-source.atoms[i].position).length();if(overlap>0)score+=overlap*overlap;
                }
            }
            if(score<bestScore){bestScore=score;best=rotation;}
        }
    }while(std::next_permutation(permutation.begin(),permutation.end()));
    if(!std::isfinite(bestScore))return fail(QStringLiteral("기존 리간드의 금속–주개 방향을 정의할 수 없습니다."));
    Molecule edited=source;int metal=c.metal;
    for(int i=source.atoms.size()-1;i>=0;--i)if(c.atoms.contains(i)){edited.removeAtom(i);if(i<c.metal)--metal;}
    QVector<int> map(f.atoms.size(),-1);
    for(int i=0;i<f.atoms.size();++i)if(f.atoms[i].atomicNumber>0){
        auto a=f.atoms[i];a.position=origin+best.rotatedVector(a.position-anchor);a.index=edited.atoms.size()+1;
        a.autoHydrogen=false;a.autoHydrogenLocked=true;a.optimizationFixed=false;map[i]=edited.atoms.size();edited.atoms<<a;
    }
    for(const auto &b:f.bonds){
        if(b.first<0||b.second<0||b.first>=map.size()||b.second>=map.size())return fail(QStringLiteral("잘못된 템플릿 결합입니다."));
        if(map[b.first]>=0&&map[b.second]>=0){auto bond=b;bond.first=map[b.first];bond.second=map[b.second];bond.manual=true;edited.bonds<<bond;}
    }
    for(int donor:f.attachments){const int added=map[donor];edited.setBond(metal,added,1,true,BondStyle::Dashed,true);
        int site=-1;float score=-2;const auto direction=(edited.atoms[added].position-origin).normalized();
        for(int i=0;i<edited.coordinationSites.size();++i){const auto &s=edited.coordinationSites[i];if(s.center!=metal||s.occupied)continue;
            const float dot=QVector3D::dotProduct(s.direction,direction);if(dot>score){score=dot;site=i;}}
        if(site>=0){edited.coordinationSites[site].occupied=true;edited.coordinationSites[site].donor=added;}
    }
    // Partial/formal charges in parsed files are not enough to infer a ligand's net charge.
    // Preserve total charge and spin; the dialog lets the user set both explicitly.
    edited.charge=source.charge;edited.multiplicity=source.multiplicity;
    edited.title=source.title+QStringLiteral(" · ")+f.name;
    if(!MoleculeEditController::validate(edited,error))return false;
    result=std::move(edited);return true;
}
}
