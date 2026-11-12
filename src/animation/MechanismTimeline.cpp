#include "animation/MechanismTimeline.h"
#include "model/StructureTools.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QQuaternion>
#include <algorithm>
#include <cmath>

namespace youngcraft {
namespace {
bool finite(const QVector3D &p) { return std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()); }
quint64 key(const Bond &b) { return (quint64(qMin(b.first,b.second))<<32) | quint32(qMax(b.first,b.second)); }
float smooth(float t) { return t*t*(3-2*t); }
QVector<int> orderedMatch(const Molecule &previous, const Molecule &next) {
    const int n=previous.atoms.size(), m=next.atoms.size();
    QVector<int> result(m,-1), lengths((n+1)*(m+1));
    auto at=[&](int i,int j)->int& { return lengths[i*(m+1)+j]; };
    for(int i=n-1;i>=0;--i) for(int j=m-1;j>=0;--j)
        at(i,j)=previous.atoms[i].atomicNumber==next.atoms[j].atomicNumber
            ? 1+at(i+1,j+1) : qMax(at(i+1,j),at(i,j+1));
    for(int i=0,j=0;i<n&&j<m;) {
        if(previous.atoms[i].atomicNumber==next.atoms[j].atomicNumber) { result[j++]=i++; }
        else if(at(i+1,j)>at(i,j+1)) ++i;
        else ++j;
    }
    return result;
}

// Horn quaternion fit of common atoms, excluding added/removed fragments.
void alignFrames(QVector<QVector<QVector3D>> &frames, const Molecule &molecule,
                 const QVector<int> &mapping, const QVector<QVector3D> &reference,
                 const QVector<bool> &present) {
    Molecule moving, target;
    for(int i=0;i<mapping.size();++i) if(mapping[i]>=0 && mapping[i]<present.size() && present[mapping[i]]) {
        auto a=molecule.atoms[i]; a.position=frames.first()[i]; moving.atoms.push_back(a);
        a.position=reference[mapping[i]]; target.atoms.push_back(a);
    }
    if(moving.isEmpty()) return;
    QVector3D a,b; int count=0;
    bool heavy=false; for(const auto &atom:moving.atoms) if(atom.atomicNumber!=1) ++count;
    heavy=count>=3; count=0;
    for(int i=0;i<moving.atoms.size();++i) if(!heavy||moving.atoms[i].atomicNumber!=1) {
        a+=moving.atoms[i].position; b+=target.atoms[i].position; ++count;
    }
    a/=float(count); b/=float(count);
    QQuaternion rotation;
    if(count==2) {
        rotation=QQuaternion::rotationTo(moving.atoms[1].position-moving.atoms[0].position,
                                        target.atoms[1].position-target.atoms[0].position);
    } else if(count>2) {
        double s[3][3]{};
        for(int i=0;i<moving.atoms.size();++i) if(!heavy||moving.atoms[i].atomicNumber!=1) {
            const auto x=moving.atoms[i].position-a,y=target.atoms[i].position-b;
            for(int j=0;j<3;++j) for(int k=0;k<3;++k) s[j][k]+=double(x[j])*y[k];
        }
        const auto xx=s[0][0],xy=s[0][1],xz=s[0][2],yx=s[1][0],yy=s[1][1],yz=s[1][2],zx=s[2][0],zy=s[2][1],zz=s[2][2];
        double h[4][4]={{xx+yy+zz,yz-zy,zx-xz,xy-yx},{yz-zy,xx-yy-zz,xy+yx,zx+xz},
            {zx-xz,xy+yx,-xx+yy-zz,yz+zy},{xy-yx,zx+xz,yz+zy,-xx-yy+zz}};
        double v[4][4]{}; for(int i=0;i<4;++i)v[i][i]=1;
        for(int iter=0;iter<64;++iter) {
            int p=0,q=1; for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)if(std::abs(h[i][j])>std::abs(h[p][q])){p=i;q=j;}
            if(std::abs(h[p][q])<1e-12)break;
            const double angle=.5*std::atan2(2*h[p][q],h[q][q]-h[p][p]),c=std::cos(angle),t=std::sin(angle);
            const double pp=h[p][p],qq=h[q][q],pq=h[p][q];
            for(int k=0;k<4;++k)if(k!=p&&k!=q){const double kp=h[k][p],kq=h[k][q];h[k][p]=h[p][k]=c*kp-t*kq;h[k][q]=h[q][k]=t*kp+c*kq;}
            h[p][p]=c*c*pp-2*c*t*pq+t*t*qq;h[q][q]=t*t*pp+2*c*t*pq+c*c*qq;h[p][q]=h[q][p]=0;
            for(int k=0;k<4;++k){const double kp=v[k][p],kq=v[k][q];v[k][p]=c*kp-t*kq;v[k][q]=t*kp+c*kq;}
        }
        int best=0;for(int i=1;i<4;++i)if(h[i][i]>h[best][best])best=i;
        rotation=QQuaternion(float(v[0][best]),float(v[1][best]),float(v[2][best]),float(v[3][best])).normalized();
    }
    for(auto &frame:frames)for(auto &p:frame)p=b+rotation.rotatedVector(p-a);
}
float tangent(float a,float b,double da,double db) {
    if(a*b<=0)return 0;
    const double w1=2*db+da,w2=db+2*da;
    return float((w1+w2)/(w1/a+w2/b));
}
}

double MechanismStep::travelSeconds() const {
    return seconds/qBound(.1,presentation["speed"].toDouble(1),10.);
}

double MechanismStep::holdSeconds() const {
    return qBound(0.,presentation["hold"].toDouble(),30.);
}

MechanismStep MechanismStep::fromCalculation(const CalculationData &data,bool usePath) {
    MechanismStep step;step.path=data.filePath;step.name=QFileInfo(data.filePath).completeBaseName();step.molecule=data.molecule;
    QVector<int> indices;
    if(usePath&&!data.ircPoints.isEmpty()) {
        auto points=data.ircPoints;
        bool ordered=true;for(const auto &p:points)ordered=ordered&&std::isfinite(p.coordinate);
        if(ordered)std::stable_sort(points.begin(),points.end(),[](const auto&a,const auto&b){return a.coordinate<b.coordinate;});
        for(const auto &p:points)indices.push_back(p.frameIndex);
    } else if(usePath&&data.format=="GSM") {
        for(const auto &p:data.scanPoints)indices.push_back(p.frameIndex);
    } else if(usePath&&data.format=="XYZ") {
        for(int i=0;i<data.frames.size();++i)indices.push_back(i);
    }
    if(indices.isEmpty()&&!data.frames.isEmpty())indices.push_back(data.frames.size()-1);
    QHash<int,double> absolute,relative;
    auto collect=[&](const auto &points){
        for(const auto &point:points) {
            if(std::isfinite(point.energyHartree))absolute.insert(point.frameIndex,point.energyHartree);
            if(std::isfinite(point.relativeKcal))relative.insert(point.frameIndex,point.relativeKcal);
        }
    };
    collect(data.scanPoints);collect(data.ircPoints);
    QJsonArray energies,relativeEnergies;
    for(int index:indices)if(index>=0&&index<data.frames.size()&&data.frames[index].coordinates.size()==step.molecule.atoms.size()) {
        step.frames.push_back(data.frames[index].coordinates);
        double value=data.frames[index].metrics.energyHartree;
        if(!std::isfinite(value))value=absolute.value(index,kNaN);
        const double kcal=relative.value(index,kNaN);
        energies.append(std::isfinite(value)?QJsonValue(value):QJsonValue());
        relativeEnergies.append(std::isfinite(kcal)?QJsonValue(kcal):QJsonValue());
    }
    if(!energies.isEmpty())step.presentation["sourceEnergiesHartree"]=energies;
    if(!relative.isEmpty())step.presentation["sourceEnergiesKcal"]=relativeEnergies;
    if(step.frames.isEmpty()) { QVector<QVector3D> frame;for(const auto&a:step.molecule.atoms)frame.push_back(a.position);step.frames.push_back(frame); }
    if(!step.frames.isEmpty())for(int i=0;i<step.molecule.atoms.size();++i)step.molecule.atoms[i].position=step.frames.last()[i];
    return step;
}

bool MechanismTimeline::build(const QVector<MechanismStep> &steps,const MechanismOptions &options,QString *error) {
    auto fail=[&](const QString &message){if(error)*error=message;return false;};
    if(steps.isEmpty()||steps.size()>300)return fail(QStringLiteral("1–300개의 반응 단계를 넣어주세요."));
    MechanismTimeline built;built.m_options=options;
    if(!std::isfinite(options.lift)||options.lift<0||options.lift>100)return fail(QStringLiteral("조각 이동 거리는 0–100 Å입니다."));
    QVector<QVector<QVector<QVector3D>>> allFrames;
    QVector<QVector3D> reference;QVector<bool> present;
    qsizetype nodeCount=1;
    for(int stage=0;stage<steps.size();++stage) {
        const auto &step=steps[stage];
        if(step.molecule.isEmpty()||step.molecule.atoms.size()>2000||!std::isfinite(step.seconds)||step.seconds<.1||step.seconds>120)
            return fail(QStringLiteral("단계 %1: 원자는 1–2000개, 구간 시간은 0.1–120초로 지정하세요.").arg(stage+1));
        auto frames=step.frames;
        if(frames.isEmpty()){QVector<QVector3D> frame;for(const auto&a:step.molecule.atoms)frame.push_back(a.position);frames.push_back(frame);}
        for(const auto &frame:frames) {
            if(frame.size()!=step.molecule.atoms.size())return fail(QStringLiteral("단계 %1: 프레임의 원자 수가 다릅니다.").arg(stage+1));
            for(const auto&p:frame)if(!finite(p))return fail(QStringLiteral("유효하지 않은 원자 좌표입니다."));
        }
        if(step.reversed)std::reverse(frames.begin(),frames.end());
        const int before=built.m_tracks.size();QVector<int> mapping(step.molecule.atoms.size(),-1);
        if(!step.mapping.isEmpty()) {
            if(step.mapping.size()!=mapping.size()||stage==0)return fail(QStringLiteral("첫 단계는 기준 번호를 정의합니다. 이후 단계에서 원자 대응을 지정하세요."));
            mapping=step.mapping;
        } else if(stage>0) {
            const auto local=orderedMatch(steps[stage-1].molecule,step.molecule);
            for(int i=0;i<local.size();++i)if(local[i]>=0)mapping[i]=built.m_mappings.last()[local[i]];
        }
        QSet<int> used;int common=0;
        for(int i=0;i<mapping.size();++i) {
            int &id=mapping[i];
            if(id < -1 || id>=before)return fail(QStringLiteral("단계 %1: 존재하지 않는 원자 대응 번호입니다.").arg(stage+1));
            if(id>=0) {
                if(used.contains(id)||built.m_tracks[id].atomicNumber!=step.molecule.atoms[i].atomicNumber)
                    return fail(QStringLiteral("단계 %1: 원자 대응이 중복되거나 원소가 다릅니다.").arg(stage+1));
                ++common;
            } else {id=built.m_tracks.size();auto atom=step.molecule.atoms[i];atom.index=id+1;built.m_tracks.push_back(atom);}
            used.insert(id);
        }
        if(stage>0&&common==0)return fail(QStringLiteral("단계 %1: 공통 원자가 없습니다. 원자 대응을 확인하세요.").arg(stage+1));
        if(stage>0&&options.align)alignFrames(frames,step.molecule,mapping,reference,present);
        reference.resize(built.m_tracks.size());present.fill(false,built.m_tracks.size());
        for(int i=0;i<mapping.size();++i){reference[mapping[i]]=frames.last()[i];present[mapping[i]]=true;}
        built.m_mappings.push_back(mapping);built.m_names.push_back(step.name);
        built.m_summaries.push_back(stage==0 ? QStringLiteral("기준 · %1개").arg(mapping.size())
            : QStringLiteral("공통 %1 · 추가 %2 · 이전 단계 이탈 %3%4").arg(common).arg(mapping.size()-common)
                .arg(steps[stage-1].molecule.atoms.size()-[&]{int n=0;for(int id:built.m_mappings[stage-1])if(used.contains(id))++n;return n;}())
                .arg(step.mapping.isEmpty()?QStringLiteral(" · 자동 대응 확인"):QStringLiteral(" · 수동 대응")));
        nodeCount+=frames.size()+(step.holdSeconds()>0?1:0);allFrames.push_back(frames);
        if(nodeCount*built.m_tracks.size()>2000000)return fail(QStringLiteral("경로가 너무 큽니다. 파일을 나누거나 경로 프레임 수를 줄여주세요."));
    }
    double time=0;
    for(int stage=0;stage<steps.size();++stage) {
        const auto &frames=allFrames[stage];const auto &map=built.m_mappings[stage];
        for(int f=0;f<frames.size();++f) {
            if(stage>0||f>0)time+=steps[stage].travelSeconds()/qMax(1,int(frames.size())-(stage==0?1:0));
            Node node;node.time=time;node.step=stage;node.positions.resize(built.trackCount());node.presence.fill(0,built.trackCount());
            for(int i=0;i<map.size();++i){node.positions[map[i]]=frames[f][i];node.presence[map[i]]=1;}
            auto topology=steps[stage].molecule;
            if(frames.size()>1){for(int i=0;i<map.size();++i)topology.atoms[i].position=frames[f][i];topology.rebuildBonds(1.25,true,true,false);}
            for(auto bond:topology.bonds)if(bond.first>=0&&bond.second>=0&&bond.first<map.size()&&bond.second<map.size()) {
                bond.first=map[bond.first];bond.second=map[bond.second];node.bonds.push_back(bond);
            }
            built.m_nodes.push_back(node);
        }
        built.m_stageEnds.push_back(built.m_nodes.size()-1);
        if(const double hold=steps[stage].holdSeconds();hold>0) {
            auto node=built.m_nodes.last();
            time+=hold;node.time=time;built.m_nodes.push_back(node);
        }
    }
    // Hold the endpoints without a discontinuous wrap when presentation playback ends.
    if(built.m_nodes.size()==1){auto last=built.m_nodes.first();last.time=steps.first().travelSeconds();built.m_nodes.push_back(last);}
    for(int track=0;track<built.trackCount();++track) {
        int last=-1,first=-1;
        for(int n=0;n<built.m_nodes.size();++n)if(built.m_nodes[n].presence[track]>0){first=n;break;}
        for(int n=0;n<built.m_nodes.size();++n) {
            auto &node=built.m_nodes[n];
            if(node.presence[track]>0){last=n;continue;}
            const int anchor=last>=0?last:first;
            node.positions[track]=built.m_nodes[anchor].positions[track]+QVector3D(0,options.lift,0);
            node.presence[track]=last>=0&&options.keepDeparted?.18f:0.f;
        }
    }
    // Preserve internal bonds of departing fragments, never their link to the reacting core.
    QMap<quint64,Bond> previous;
    for(int i=0;i<built.m_nodes.size();++i) {
        auto &node=built.m_nodes[i];
        QMap<quint64,Bond> current;
        for(const auto &bond:node.bonds)current.insert(key(bond),bond);
        for(auto it=previous.cbegin();it!=previous.cend();++it) {
            const auto &b=it.value();
            if(node.presence[b.first]<1&&node.presence[b.second]<1&&node.presence[b.first]>0&&node.presence[b.second]>0&&!current.contains(it.key())) {
                node.bonds.push_back(b);
                current.insert(it.key(),b);
            }
        }
        bool changed=previous.size()!=current.size();
        for(auto a=previous.cbegin(),b=current.cbegin();!changed&&a!=previous.cend();++a,++b)
            changed=a.key()!=b.key()||std::abs(a->order-b->order)>.1||a->style!=b->style;
        if(i>0&&changed)built.m_topologyChanges.push_back((built.m_nodes[i-1].time+node.time)/2);
        previous=std::move(current);
    }
    for(int stage=0;stage+1<steps.size();++stage) {
        const auto marks=steps[stage].presentation["photoCleavages"].toArray();
        if(marks.isEmpty())continue;
        const auto nodes=built.cleavageNodes(stage);
        const auto &mapping=built.m_mappings[stage];
        for(const auto &mark:marks) {
            const auto pair=mark.toArray();
            if(pair.size()<2||pair.size()>3)continue;
            const int first=pair[0].toInt(-1),second=pair[1].toInt(-1);
            if(first<0||second<0||first>=mapping.size()||second>=mapping.size())continue;
            Bond bond;bond.first=mapping[first];bond.second=mapping[second];
            const auto id=key(bond);
            const auto occurrences=nodes.value(id);
            const int occurrence=pair.size()==3?pair[2].toInt(-1):0;
            if(occurrence<0||occurrence>=occurrences.size())continue;
            auto eventTime=[&](int node){return (built.m_nodes[node-1].time+built.m_nodes[node].time)/2;};
            const double midpoint=eventTime(occurrences[occurrence]);
            double start=qMax(built.m_nodes[built.m_stageEnds[stage]].time,midpoint-.28);
            double end=qMin(built.m_nodes[built.m_stageEnds[stage+1]].time,midpoint+.18);
            if(occurrence>0)start=qMax(start,(midpoint+eventTime(occurrences[occurrence-1]))/2);
            if(occurrence+1<occurrences.size())end=qMin(end,(midpoint+eventTime(occurrences[occurrence+1]))/2);
            built.m_photoCleavages.push_back({bond,start,end});
        }
    }
    built.emphasize({},2);
    for(int i=0;i<built.trackCount();++i){built.m_visible[i]=built.m_tracks[i].visible;built.m_opacity[i]=built.m_tracks[i].displayOpacity;}
    *this=std::move(built);return true;
}

QMap<quint64,QVector<int>> MechanismTimeline::cleavageNodes(int stage) const {
    QMap<quint64,QVector<int>> lost;
    if(stage<0||stage+1>=m_stageEnds.size())return lost;
    QSet<quint64> source;
    for(const auto &bond:m_nodes[m_stageEnds[stage]].bonds)source.insert(key(bond));
    auto previous=source;
    for(int n=m_stageEnds[stage]+1;n<=m_stageEnds[stage+1];++n) {
        QSet<quint64> current;
        for(const auto &bond:m_nodes[n].bonds) {
            const auto id=key(bond);
            if(source.contains(id))current.insert(id);
        }
        for(auto id:previous)if(!current.contains(id))lost[id].push_back(n);
        previous=std::move(current);
    }
    return lost;
}

QVector<double> MechanismTimeline::cleavageTimes(int stage,int first,int second) const {
    if(stage<0||stage>=m_mappings.size())return {};
    const auto &map=m_mappings[stage];
    if(first<0||second<0||first>=map.size()||second>=map.size())return {};
    Bond bond;bond.first=map[first];bond.second=map[second];
    QVector<double> times;
    for(int node:cleavageNodes(stage).value(key(bond)))
        times.push_back((m_nodes[node-1].time+m_nodes[node].time)/2);
    return times;
}

QVector<QPair<int,int>> MechanismTimeline::cleavableBonds(int stage) const {
    QVector<QPair<int,int>> pairs;
    const auto lost=cleavageNodes(stage);
    if(lost.isEmpty())return pairs;
    const auto &mapping=m_mappings[stage];
    QHash<int,int> local;
    for(int i=0;i<mapping.size();++i)local.insert(mapping[i],i);
    for(const auto &bond:m_nodes[m_stageEnds[stage]].bonds) {
        if(lost.contains(key(bond))&&local.contains(bond.first)&&local.contains(bond.second)) {
            const int first=local.value(bond.first),second=local.value(bond.second);
            pairs.push_back({qMin(first,second),qMax(first,second)});
        }
    }
    return pairs;
}

Molecule MechanismTimeline::sample(double time) const {
    Molecule result;if(m_nodes.isEmpty())return result;result.atoms=m_tracks;
    time=std::isfinite(time)?qBound(0.,time,duration()):0;
    auto it=std::upper_bound(m_nodes.cbegin(),m_nodes.cend(),time,[](double t,const Node&n){return t<n.time;});
    const int right=qBound(1,int(it-m_nodes.cbegin()),int(m_nodes.size())-1),left=right-1;
    const auto &a=m_nodes[left],&b=m_nodes[right];const double dt=b.time-a.time;
    const float u=float(qBound(0.,(time-a.time)/dt,1.)),e=smooth(u);
    const float h00=2*u*u*u-3*u*u+1,h10=u*u*u-2*u*u+u,h01=-2*u*u*u+3*u*u,h11=u*u*u-u*u;
    for(int i=0;i<result.atoms.size();++i) {
        QVector3D va,vb;
        for(int axis=0;axis<3;++axis) {
            const float delta=float((b.positions[i][axis]-a.positions[i][axis])/dt);
            if(left>0){const auto&p=m_nodes[left-1];va[axis]=tangent(float((a.positions[i][axis]-p.positions[i][axis])/(a.time-p.time)),delta,a.time-p.time,dt);}
            if(right+1<m_nodes.size()){const auto&n=m_nodes[right+1];vb[axis]=tangent(delta,float((n.positions[i][axis]-b.positions[i][axis])/(n.time-b.time)),dt,n.time-b.time);}
        }
        auto &atom=result.atoms[i];atom.position=a.positions[i]*h00+va*float(dt)*h10+b.positions[i]*h01+vb*float(dt)*h11;
        atom.displayOpacity=((1-e)*a.presence[i]+e*b.presence[i])*m_opacity[i];
        atom.visible=m_visible[i]&&atom.displayOpacity>.001f;
    }
    QHash<quint64,float> flashes;
    for(const auto &cleavage:m_photoCleavages) {
        if(time<=cleavage.start||time>=cleavage.end)continue;
        const double phase=3.141592653589793*(time-cleavage.start)/(cleavage.end-cleavage.start);
        const float pulse=float(std::sin(phase)*(.35+.65*std::pow(std::sin(3*phase),2)));
        flashes[key(cleavage.bond)]=pulse;
        for(int atom:{cleavage.bond.first,cleavage.bond.second})
            result.atoms[atom].displayFlash=qMax(result.atoms[atom].displayFlash,pulse);
    }
    QMap<quint64,Bond> oldBonds,newBonds;for(const auto&bond:a.bonds)oldBonds.insert(key(bond),bond);for(const auto&bond:b.bonds)newBonds.insert(key(bond),bond);
    auto add=[&](Bond bond,float opacity){
        bond.displayOpacity=opacity;
        if(!flashes.isEmpty())bond.displayFlash=flashes.value(key(bond));
        if(opacity>.001f)result.bonds.push_back(bond);
    };
    for(auto iter=oldBonds.cbegin();iter!=oldBonds.cend();++iter) {
        const auto newer=newBonds.constFind(iter.key());
        if(newer==newBonds.cend())add(iter.value(),1-e);
        else if(std::abs(iter->order-newer->order)>.1||iter->style!=newer->style) {add(iter.value(),1-e);add(newer.value(),e);}
        else add(iter.value(),1);
    }
    for(auto iter=newBonds.cbegin();iter!=newBonds.cend();++iter)if(!oldBonds.contains(iter.key()))add(iter.value(),e);
    if(m_options.preserveBonds&&u>0&&u<1)for(int iteration=0;iteration<12;++iteration)
        for(auto iter=oldBonds.cbegin();iter!=oldBonds.cend();++iter) {
            const auto newer=newBonds.constFind(iter.key());const auto &bond=iter.value();
            if(newer==newBonds.cend()||bond.coordination||std::abs(bond.order-newer->order)>.1)continue;
            const int i=bond.first,j=bond.second;
            if(a.presence[i]<.99f||a.presence[j]<.99f||b.presence[i]<.99f||b.presence[j]<.99f)continue;
            const float length=(1-u)*(a.positions[i]-a.positions[j]).length()+u*(b.positions[i]-b.positions[j]).length();
            const auto vector=result.atoms[j].position-result.atoms[i].position;const float distance=vector.length();
            if(distance<1e-6f)continue;
            const auto correction=vector*(.4f*(distance-length)/distance);
            result.atoms[i].position+=correction;result.atoms[j].position-=correction;
        }
    const float lift=smooth(float(qBound(0.,(time-m_parkStart)/qMax(.001,m_parkEnd-m_parkStart),1.)));
    for(int i:m_parked)if(i>=0&&i<result.atoms.size()){
        result.atoms[i].position+=QVector3D(0,m_options.lift*lift,0);
        result.atoms[i].displayOpacity*=1-.65f*lift;
    }
    for(auto &bond:result.bonds)if(m_parked.contains(bond.first)!=m_parked.contains(bond.second))bond.displayOpacity*=1-lift;
    return result;
}
void MechanismTimeline::park(const QSet<int>&tracks,double start,double seconds){
    m_parked=tracks;m_parkStart=qBound(0.,start,qMax(0.,duration()-.1));m_parkEnd=qMin(duration(),m_parkStart+qMax(.1,seconds));
}
Molecule MechanismTimeline::envelope() const {
    Molecule result;if(m_nodes.isEmpty())return result;
    QVector3D minimum,maximum;bool initialized=false;
    auto include=[&](const QVector3D &p){if(!initialized){minimum=maximum=p;initialized=true;}for(int axis=0;axis<3;++axis){minimum[axis]=qMin(minimum[axis],p[axis]);maximum[axis]=qMax(maximum[axis],p[axis]);}};
    for(const auto &node:m_nodes)for(int i=0;i<node.positions.size();++i)if(node.presence[i]>0&&m_visible[i])include(node.positions[i]);
    for(int i=0;i<=100;++i){const auto frame=sample(duration()*i/100.);for(const auto &a:frame.atoms)if(a.visible)include(a.position);}
    if(!initialized)return result;
    // Eight corners fit the entire movie without storing a duplicate atom cloud.
    Atom padding=m_tracks.first();
    for(const auto &a:m_tracks)if(vanDerWaalsRadius(a.atomicNumber)*a.radiusScale>vanDerWaalsRadius(padding.atomicNumber)*padding.radiusScale)padding=a;
    padding.visible=true;padding.displayOpacity=1;
    for(int corner=0;corner<8;++corner){auto a=padding;for(int axis=0;axis<3;++axis)a.position[axis]=(corner&(1<<axis))?maximum[axis]:minimum[axis];result.atoms.push_back(a);}
    return result;
}
QString MechanismTimeline::caption(double time) const {
    if(m_nodes.isEmpty())return {};
    auto it=std::upper_bound(m_nodes.cbegin(),m_nodes.cend(),time,[](double t,const Node&n){return t<n.time;});
    if(it==m_nodes.cbegin())return m_names.first();
    if(it==m_nodes.cend())return m_names.last();
    const int before=(it-1)->step,after=it->step;
    return before==after?m_names[after]:m_names[before]+QStringLiteral(" → ")+m_names[after];
}
void MechanismTimeline::emphasize(const QSet<int>&keep,int mode) {
    m_visible.resize(trackCount());m_opacity.resize(trackCount());
    for(int i=0;i<trackCount();++i){m_visible[i]=mode!=0||keep.contains(i);m_opacity[i]=mode==1&&!keep.contains(i)?.2f:1.f;}
}
QByteArray MechanismTimeline::saveProject(const QVector<MechanismStep>&steps,const MechanismOptions&options) {
    QJsonArray array;
    for(const auto &step:steps) {
        QJsonArray frames,mapping,opacity;for(int id:step.mapping)mapping.append(id);
        for(const auto &atom:step.molecule.atoms)opacity.append(atom.displayOpacity);
        for(const auto &frame:step.frames){QJsonArray coordinates;for(const auto&p:frame){coordinates.append(p.x());coordinates.append(p.y());coordinates.append(p.z());}frames.append(coordinates);}
        array.append(QJsonObject{{"name",step.name},{"path",step.path},{"molecule",QString::fromLatin1(StructureTools::encode(step.molecule).toBase64())},
            {"frames",frames},{"mapping",mapping},{"opacity",opacity},{"seconds",step.seconds},{"reversed",step.reversed},{"presentation",step.presentation}});
    }
    return QJsonDocument(QJsonObject{{"format","YoungcraftMechanism"},{"version",1},{"steps",array},
        {"align",options.align},{"preserveBonds",options.preserveBonds},{"keepDeparted",options.keepDeparted},{"lift",options.lift}}).toJson();
}
bool MechanismTimeline::loadProject(const QByteArray &bytes,QVector<MechanismStep>&steps,MechanismOptions&options,QString *error) {
    auto fail=[&]{if(error)*error=QStringLiteral("유효한 Youngcraft 메커니즘 프로젝트가 아닙니다.");return false;};
    if(bytes.size()>128*1024*1024)return fail();
    const auto doc=QJsonDocument::fromJson(bytes);const auto root=doc.object();
    if(root["format"]!="YoungcraftMechanism"||root["version"].toInt()!=1||!root["steps"].isArray())return fail();
    QVector<MechanismStep> loaded;
    for(const auto &value:root["steps"].toArray()) {
        const auto obj=value.toObject();MechanismStep step;step.name=obj["name"].toString();step.path=obj["path"].toString();
        if(!StructureTools::decode(QByteArray::fromBase64(obj["molecule"].toString().toLatin1()),step.molecule))return fail();
        const auto opacity=obj["opacity"].toArray();
        if(!opacity.isEmpty()){if(opacity.size()!=step.molecule.atoms.size())return fail();for(int i=0;i<opacity.size();++i){const double alpha=opacity[i].toDouble(-1);if(alpha<0||alpha>1||!std::isfinite(alpha))return fail();step.molecule.atoms[i].displayOpacity=float(alpha);}}
        step.seconds=obj["seconds"].toDouble(-1);step.reversed=obj["reversed"].toBool();step.presentation=obj["presentation"].toObject();
        for(const auto &value:obj["mapping"].toArray()){if(!value.isDouble()||value.toDouble()!=value.toInt())return fail();step.mapping.push_back(value.toInt());}
        for(const auto &value:obj["frames"].toArray()) {
            const auto coordinates=value.toArray();if(coordinates.size()!=step.molecule.atoms.size()*3)return fail();
            QVector<QVector3D> frame;
            for(int i=0;i<coordinates.size();i+=3){for(int j=0;j<3;++j)if(!coordinates[i+j].isDouble())return fail();frame.push_back(QVector3D(float(coordinates[i].toDouble()),float(coordinates[i+1].toDouble()),float(coordinates[i+2].toDouble())));}
            step.frames.push_back(frame);
        }
        loaded.push_back(step);
    }
    MechanismOptions settings;settings.align=root["align"].toBool(true);settings.preserveBonds=root["preserveBonds"].toBool(true);
    settings.keepDeparted=root["keepDeparted"].toBool(true);settings.lift=float(root["lift"].toDouble(6));
    MechanismTimeline check;if(!check.build(loaded,settings,error))return false;
    steps=std::move(loaded);options=settings;return true;
}
}
