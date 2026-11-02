#include "model/FragmentLibrary.h"
#include "model/MoleculeEditController.h"
#include <QDirIterator>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QQuaternion>
#include <QStandardPaths>
#include <QSaveFile>
#include <cmath>
#include <algorithm>
namespace youngcraft {
bool FragmentLibrary::metalComplex(const Fragment &f,int metal,int oxidation,int totalCharge,
                                  int multiplicity,Molecule &result,QString *error) {
    if(!isMetal(metal)||(!f.ligand&&f.category!="lab-ligand")||multiplicity<1||oxidation < -4||oxidation>4) {
        if(error)*error=QStringLiteral("금속, 리간드, 형식 전하와 multiplicity를 확인하세요.");return false;
    }
    Molecule complex;DrawState state;state.element=metal;state.formalCharge=oxidation;
    const int center=MoleculeEditController::addAtom(complex,{},state);
    complex.atoms[center].autoHydrogenLocked=true;
    if(!insert(complex,f,center,{},f.direction,error))return false;
    complex.charge=totalCharge;complex.multiplicity=multiplicity;
    complex.title=elementSymbol(metal)+"–"+f.name;
    if(!MoleculeEditController::validate(complex,error))return false;
    result=std::move(complex);return true;
}
bool FragmentLibrary::saveLigand(const Molecule &m,int metal,const QSet<int> &included,
                                const QString &name,const QString &path,QString *error) {
    auto fail=[&](const QString &message){if(error)*error=message;return false;};
    if(!MoleculeEditController::validate(m,error))return false;
    if(metal<0||metal>=m.atoms.size()||!isMetal(m.atoms[metal].atomicNumber)||name.trimmed().isEmpty())
        return fail(QStringLiteral("유효한 금속 중심과 리간드 이름이 필요합니다."));
    QSet<int> ids=included;if(ids.isEmpty())for(int i=0;i<m.atoms.size();++i)ids.insert(i);
    if(!ids.contains(metal)||ids.size()<2)return fail(QStringLiteral("리간드와 기준 금속을 함께 선택하세요."));
    QVector<int> ordered{metal};for(int i=0;i<m.atoms.size();++i)if(i!=metal&&ids.contains(i))ordered<<i;
    QMap<int,int> mapping;QJsonArray numbers,coords,charges,ends,orders;int charge=0,donors=0;
    for(int i=0;i<ordered.size();++i){
        int old=ordered[i];mapping[old]=i;const auto &a=m.atoms[old];
        if(old!=metal&&isMetal(a.atomicNumber))return fail(QStringLiteral("기준 금속 외의 금속은 제외하세요."));
        numbers.append(old==metal?0:a.atomicNumber);charges.append(old==metal?0:a.formalCharge);
        const auto p=a.position-m.atoms[metal].position;coords.append(p.x());coords.append(p.y());coords.append(p.z());
        if(old!=metal)charge+=a.formalCharge;
    }
    for(const auto &b:m.bonds)if(mapping.contains(b.first)&&mapping.contains(b.second)){
        ends.append(mapping[b.first]);ends.append(mapping[b.second]);orders.append(b.first==metal||b.second==metal?1.0:b.order);
        if(b.first==metal||b.second==metal)++donors;
    }
    if(!donors)return fail(QStringLiteral("기준 금속과 주개 원자 사이의 결합을 먼저 지정하세요."));
    QJsonObject obj{{"chemicalJson",1},{"atoms",QJsonObject{{"elements",QJsonObject{{"number",numbers}}},
        {"coords",QJsonObject{{"3d",coords}}},{"formalCharges",charges}}},
        {"bonds",QJsonObject{{"connections",QJsonObject{{"index",ends}}},{"order",orders}}},
        {"properties",QJsonObject{{"name",name.trimmed()},{"category","lab-ligand"},{"totalCharge",charge},
            {"source",QJsonObject{{"kind","user-saved current geometry"},{"note","Retains metal-frame geometry; assemble only with compatible adduct frames."}}}}}};
    if(!QDir().mkpath(QFileInfo(path).absolutePath()))return fail(QStringLiteral("템플릿 폴더를 만들 수 없습니다."));
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly))return fail(file.errorString());
    if(file.write(QJsonDocument(obj).toJson())<0||!file.commit())return fail(file.errorString());
    return true;
}
bool FragmentLibrary::read(const QString &path, Fragment &f, QString *error) {
    auto fail=[&](const QString &s){if(error)*error=path+": "+s;return false;};
    QFile file(path);if(!file.open(QIODevice::ReadOnly))return fail(file.errorString());
    QJsonParseError parse;auto doc=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError)return fail(parse.errorString());
    auto obj=doc.object(),atoms=obj["atoms"].toObject(),bonds=obj["bonds"].toObject();
    auto numbers=atoms["elements"].toObject()["number"].toArray(),coords=atoms["coords"].toObject()["3d"].toArray(),charges=atoms["formalCharges"].toArray();
    if(numbers.isEmpty()||coords.size()!=numbers.size()*3)return fail("유효하지 않은 좌표");
    f={};f.resource=path;f.name=QFileInfo(path).completeBaseName();f.category=QFileInfo(path).dir().dirName();f.ligand=path.contains("/ligands/");
    const auto properties=obj["properties"].toObject();
    if(!properties["name"].toString().isEmpty())f.name=properties["name"].toString();
    if(!properties["category"].toString().isEmpty())f.category=properties["category"].toString();
    f.totalCharge=obj["properties"].toObject()["totalCharge"].toInt();
    for(const auto &entry:properties["optimizedGeometries"].toArray()) {
        const auto object=entry.toObject();OptimizedGeometry geometry;
        geometry.metal=object["metalAtomicNumber"].toInt();geometry.oxidation=object["oxidation"].toInt();
        geometry.charge=object["charge"].toInt();geometry.multiplicity=object["multiplicity"].toInt();
        geometry.adduct=object["adduct"].toString();geometry.coligand=object["coligand"].toString();
        geometry.source=object["source"].toObject()["file"].toString();
        const auto elements=object["atomicNumbers"].toArray(),positions=object["coordinates"].toArray();
        if(elements.isEmpty()||positions.size()!=elements.size()*3||geometry.multiplicity<1||!isMetal(geometry.metal))return fail("잘못된 최적화 구조 템플릿");
        for(int i=0;i<elements.size();++i){
            const int z=elements[i].toInt(-1);const double x=positions[3*i].toDouble(qQNaN()),y=positions[3*i+1].toDouble(qQNaN()),zpos=positions[3*i+2].toDouble(qQNaN());
            if(z<1||z>118||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(zpos))return fail("잘못된 최적화 구조 좌표");
            geometry.atomicNumbers<<z;geometry.positions<<QVector3D(float(x),float(y),float(zpos));
        }
        f.optimizedGeometries<<geometry;
    }

    for(int i=0;i<numbers.size();++i){Atom a;a.index=i+1;a.atomicNumber=numbers[i].toInt(-1);if(a.atomicNumber<0||a.atomicNumber>118)return fail("유효하지 않은 원소");a.element=elementSymbol(a.atomicNumber);a.position={float(coords[i*3].toDouble()),float(coords[i*3+1].toDouble()),float(coords[i*3+2].toDouble())};a.formalCharge=charges.size()>i?charges[i].toInt():0;f.atoms<<a;if(a.atomicNumber==0)f.dummyAtoms<<i;}
    auto ends=bonds["connections"].toObject()["index"].toArray(),orders=bonds["order"].toArray();
    if(ends.size()%2||orders.size()!=ends.size()/2)return fail("유효하지 않은 결합 배열");
    QSet<quint64> pairs;
    for(int i=0;i<orders.size();++i){int a=ends[i*2].toInt(-1),b=ends[i*2+1].toInt(-1);double o=orders[i].toDouble();if(a<0||b<0||a>=numbers.size()||b>=numbers.size()||a==b||o<1||o>3)return fail("유효하지 않은 결합");quint64 key=(quint64(qMin(a,b))<<32)|quint32(qMax(a,b));if(pairs.contains(key))return fail("중복된 결합");pairs.insert(key);f.bonds<<Bond{a,b,o};}
    for(const Bond &b:f.bonds){if(f.atoms[b.first].atomicNumber==0&&f.atoms[b.second].atomicNumber>0&&!f.attachments.contains(b.second))f.attachments<<b.second;if(f.atoms[b.second].atomicNumber==0&&f.atoms[b.first].atomicNumber>0&&!f.attachments.contains(b.first))f.attachments<<b.first;}
    if(f.category=="haptic" && f.dummyAtoms.size()>=2){
        int centroid=f.dummyAtoms[0],metal=f.dummyAtoms[1];
        auto meanDistance=[&](int d){double sum=0;int n=0;for(auto a:f.atoms)if(a.atomicNumber>1){sum+=(a.position-f.atoms[d].position).length();++n;}return sum/qMax(1,n);};
        if(meanDistance(metal)<meanDistance(centroid))std::swap(metal,centroid);
        f.anchor=f.atoms[metal].position;f.direction=(f.atoms[centroid].position-f.anchor).normalized();
        int eta=0;auto name=f.name;for(int i=3;i<name.size()&&name[i].isDigit();++i)eta=eta*10+name[i].digitValue();
        QVector<QPair<float,int>> nearest;for(int i=0;i<f.atoms.size();++i)if(f.atoms[i].atomicNumber>1)nearest<<qMakePair((f.atoms[i].position-f.atoms[centroid].position).length(),i);
        std::sort(nearest.begin(),nearest.end());for(int i=0;i<qMin(eta,int(nearest.size()));++i)f.hapticAtoms<<nearest[i].second;
    }else if(!f.dummyAtoms.isEmpty()){
        f.anchor=f.atoms[f.dummyAtoms.first()].position;QVector3D center;for(int i:f.attachments)center+=f.atoms[i].position;if(!f.attachments.isEmpty())f.direction=(center/f.attachments.size()-f.anchor).normalized();
    }
    if(f.direction.lengthSquared()<1e-8f)f.direction={1,0,0};
    return true;
}
QVector<Fragment> FragmentLibrary::load(QString *error,const QString &root){
    QVector<Fragment> result;QDirIterator it(root,{"*.cjson"},QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext()){Fragment f;QString e;if(read(it.next(),f,&e))result<<f;else if(error)*error+=e+"\n";}
    if(root==":/fragments") {
        result+=load(error,QCoreApplication::applicationDirPath()+"/lab_fragments");
        result+=load(error,QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/lab_fragments");
        QSet<QString> seen;
        for(int i=result.size()-1;i>=0;--i) {
            const QString key=result[i].category+"/"+result[i].name;
            if(seen.contains(key))result.removeAt(i);else seen.insert(key);
        }
    }
    std::sort(result.begin(),result.end(),[](const auto &a,const auto &b){return a.name<b.name;});return result;
}
bool FragmentLibrary::insert(Molecule &m,const Fragment&f,int target,const QVector3D&p,const QVector3D&dir,QString*error){
    if(target>=m.atoms.size()||target< -1)return false;
    Molecule edited=m;QVector3D origin=p,direction=dir.normalized();if(direction.isNull())direction={1,0,0};
    bool coordination=target>=0 && isMetal(edited.atoms[target].atomicNumber);
    if(target>=0){
        if(edited.atoms[target].atomicNumber==1){
            int neighbor=-1;for(auto b:edited.bonds)if(b.first==target||b.second==target)neighbor=b.first==target?b.second:b.first;
            if(neighbor<0)return false;
            direction=(edited.atoms[target].position-edited.atoms[neighbor].position).normalized();origin=edited.atoms[neighbor].position;
            edited.removeAtom(target);if(neighbor>target)--neighbor;target=neighbor;coordination=isMetal(edited.atoms[target].atomicNumber);
        }else{origin=edited.atoms[target].position;
            if(!coordination) for(int i=edited.atoms.size()-1;i>=0;--i)if(edited.atoms[i].atomicNumber==1&&edited.bondIndex(target,i)>=0){direction=(edited.atoms[i].position-origin).normalized();edited.removeAtom(i);if(i<target)--target;break;}
        }
        if(f.attachments.isEmpty()&&f.hapticAtoms.isEmpty()){if(error)*error="분자 조각에 연결 정보가 없습니다.";return false;}
        if(f.ligand&&!coordination){if(error)*error="이 리간드를 연결할 금속 배위 중심을 선택하세요.";return false;}
    }
    QVector3D anchor=f.anchor;
    auto align=QQuaternion::rotationTo(f.direction,direction);
    // Functional-group dummy positions represent replaceable H, not a full bond length.
    QVector3D shift;
    if(target>=0&&!coordination&&!f.attachments.isEmpty()){
        int donor=f.attachments.first();double length=MoleculeEditController::idealLength(edited.atoms[target].atomicNumber,f.atoms[donor].atomicNumber,1);
        if(length>0)shift=direction*float(length)-align.rotatedVector(f.atoms[donor].position-anchor);
    }
    double bestScore=std::numeric_limits<double>::infinity();QQuaternion best=align;
    for(int step=0;step<12;++step){auto q=QQuaternion::fromAxisAndAngle(direction,step*30)*align;double score=0;
        for(auto atom:f.atoms)if(atom.atomicNumber>0){auto pos=origin+shift+q.rotatedVector(atom.position-anchor);for(int j=0;j<edited.atoms.size();++j){if(j==target)continue;double d=(pos-edited.atoms[j].position).length();double cutoff=(covalentRadius(atom.atomicNumber)+covalentRadius(edited.atoms[j].atomicNumber))*0.75; if(d<cutoff)score+=(cutoff-d)*(cutoff-d);}}
        if(score<bestScore){bestScore=score;best=q;}
    }
    QVector<int> map(f.atoms.size(),-1);
    for(int i=0;i<f.atoms.size();++i)if(f.atoms[i].atomicNumber>0){auto a=f.atoms[i];a.position=origin+shift+best.rotatedVector(a.position-anchor);a.index=edited.atoms.size()+1;a.autoHydrogen=false;a.autoHydrogenLocked=true;map[i]=edited.atoms.size();edited.atoms<<a;}
    for(auto b:f.bonds)if(map[b.first]>=0&&map[b.second]>=0)edited.setBond(map[b.first],map[b.second],b.order,false,b.order==1.5?BondStyle::Dashed:BondStyle::Solid,true);
    if(target>=0){
        for(int donor:f.attachments)edited.setBond(target,map[donor],1,coordination,coordination?BondStyle::Dashed:BondStyle::Solid,true);
        if(!f.hapticAtoms.isEmpty()){HapticContact h;h.metal=target;h.name=f.name;for(int i:f.hapticAtoms)h.atoms<<map[i];edited.hapticContacts<<h;}
        // Consume the closest free direction for each donor; ghost sites stay out of export.
        QVector<int> donors=f.attachments;if(donors.isEmpty()&&!f.hapticAtoms.isEmpty())donors<<f.hapticAtoms.first();
        for(int donor:donors){QVector3D v=(edited.atoms[map[donor]].position-origin).normalized();int site=-1;float score=-2;for(int i=0;i<edited.coordinationSites.size();++i){const auto&s=edited.coordinationSites[i];if(s.center!=target||s.occupied)continue;float d=QVector3D::dotProduct(v,s.direction);if(d>score){score=d;site=i;}}if(site>=0){edited.coordinationSites[site].occupied=true;edited.coordinationSites[site].donor=map[donor];}}
    }
    edited.charge+=f.totalCharge;
    if(!MoleculeEditController::validate(edited,error))return false;m=std::move(edited);return true;
}
QStringList FragmentLibrary::geometries(){return {"1배위 선형","2배위 선형","3배위 평면 삼각형","4배위 정사면체","4배위 평면 사각형","5배위 사각뿔","5배위 삼각쌍뿔","6배위 정팔면체","6배위 삼각기둥","7배위 오각쌍뿔","8배위 사각 엇각기둥"};}
QVector<QVector3D> FragmentLibrary::directions(int g){
    QVector<QVector3D> v;auto ring=[&](int n,float z,float phase){for(int i=0;i<n;++i){double a=(i*360.0/n+phase)*3.141592653589793/180;v<<QVector3D(float(std::cos(a)),float(std::sin(a)),z).normalized();}};
    switch(g){case 0:v={{1,0,0}};break;case 1:v={{1,0,0},{-1,0,0}};break;case 2:ring(3,0,0);break;
    case 3:v={{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};break;case 4:ring(4,0,0);break;case 5:ring(4,0,0);v<<QVector3D(0,0,1);break;
    case 6:ring(3,0,0);v<<QVector3D(0,0,1)<<QVector3D(0,0,-1);break;case 7:ring(4,0,0);v<<QVector3D(0,0,1)<<QVector3D(0,0,-1);break;
    case 8:ring(3,0.75f,0);ring(3,-0.75f,0);break;case 9:ring(5,0,0);v<<QVector3D(0,0,1)<<QVector3D(0,0,-1);break;case 10:ring(4,0.6f,0);ring(4,-0.6f,45);break;default:break;}
    for(auto &d:v)d.normalize();return v;
}
int FragmentLibrary::coordinationCenter(Molecule&m,int z,int q,int g,const QVector3D&p){DrawState s;s.element=z;s.formalCharge=q;int i=MoleculeEditController::addAtom(m,p,s);for(auto d:directions(g))m.coordinationSites<<CoordinationSite{i,d,false};return i;}
}
