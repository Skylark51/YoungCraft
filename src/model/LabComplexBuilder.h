#pragma once
#include "model/FragmentLibrary.h"
#include "model/MoleculeEditController.h"
#include <cmath>
#include <algorithm>
#include <QFileInfo>
namespace youngcraft {
class LabComplexBuilder {
public:
    static const OptimizedGeometry *referenceFor(const Fragment &ligand,const Fragment &adduct,int metal,
            int oxidation,int charge,int multiplicity,const Fragment *coligand=nullptr) {
        const auto adductKey=QFileInfo(adduct.resource).completeBaseName();
        const auto coligandKey=coligand?QFileInfo(coligand->resource).completeBaseName():QString();
        for(const auto &geometry:ligand.optimizedGeometries)
            if(geometry.metal==metal && geometry.oxidation==oxidation && geometry.charge==charge
               && geometry.multiplicity==multiplicity && geometry.adduct==adductKey && geometry.coligand==coligandKey)return &geometry;
        return nullptr;
    }
    static bool assemble(const Fragment &ligand, const Fragment &adduct, int metal,
                         int oxidation, int charge, int multiplicity,
                         Molecule &result, QString *error=nullptr, const Fragment *coligand=nullptr) {
        Molecule m;
        Atom center;center.atomicNumber=metal;center.element=elementSymbol(metal);
        center.index=1;center.formalCharge=oxidation;center.autoHydrogenLocked=true;
        m.atoms.append(center);
        for(const auto *fragment:{&ligand,&adduct}) {
            if(fragment->dummyAtoms.size()!=1||fragment->attachments.isEmpty()) {
                if(error)*error=QStringLiteral("fragment에는 기준 금속 위치 1개와 주개 원자 정보가 필요합니다.");
                return false;
            }
            QVector<int> mapping(fragment->atoms.size(),-1);
            for(int i=0;i<fragment->atoms.size();++i) {
                if(fragment->atoms[i].atomicNumber==0)continue;
                Atom a=fragment->atoms[i];a.position-=fragment->anchor;
                for(const auto &other:m.atoms)if((a.position-other.position).length()<0.45f){
                    if(error)*error=QStringLiteral("fragment 원자가 겹칩니다. 같은 금속 기준 좌표계로 준비하세요.");
                    return false;
                }
                a.index=m.atoms.size()+1;a.autoHydrogenLocked=true;
                mapping[i]=m.atoms.size();m.atoms.append(a);
            }
            for(const auto &bond:fragment->bonds)
                if(mapping[bond.first]>=0&&mapping[bond.second]>=0)
                    m.setBond(mapping[bond.first],mapping[bond.second],bond.order,false,
                              bond.order==1.5?BondStyle::Dashed:BondStyle::Solid,true);
            for(int donor:fragment->attachments)
                m.setBond(0,mapping[donor],1,true,BondStyle::Dashed,true);
        }
        if(coligand) {
            if(coligand->dummyAtoms.size()!=1 || coligand->attachments.size()!=1) {
                if(error)*error=QStringLiteral("보조 리간드는 기준 금속 1개와 주개 원자 1개가 필요합니다.");
                return false;
            }
            // Find an open coordination direction from the existing donor vectors.
            QVector<QVector3D> donors;
            for(const auto &b:m.bonds)if(b.first==0||b.second==0)
                donors << m.atoms[b.first==0?b.second:b.first].position.normalized();
            QVector<QVector3D> candidates;
            for(const auto &d:donors)candidates << -d;
            for(int i=0;i<256;++i) {
                const double z=1.0-2.0*(i+0.5)/256.0, r=std::sqrt(1.0-z*z), phi=i*2.399963229728653;
                candidates << QVector3D(float(r*std::cos(phi)),float(r*std::sin(phi)),float(z));
            }
            QVector<QPair<float,QVector3D>> ranked;
            for(const auto &candidate:candidates) {
                float closest=-1;
                for(const auto &d:donors)closest=qMax(closest,QVector3D::dotProduct(candidate,d));
                ranked.append({closest,candidate});
            }
            std::sort(ranked.begin(),ranked.end(),[](const auto &a,const auto &b){return a.first<b.first;});
            bool placed=false;
            for(const auto &candidate:ranked) {
                Molecule trial=m;
                if(!FragmentLibrary::insert(trial,*coligand,0,{},candidate.second,error))return false;
                bool overlap=false;
                for(int i=m.atoms.size();i<trial.atoms.size();++i)for(int j=0;j<i;++j)
                    if((trial.atoms[i].position-trial.atoms[j].position).length()<0.85f)overlap=true;
                if(!overlap){m=std::move(trial);placed=true;break;}
            }
            if(!placed) {
                if(error)*error=QStringLiteral("보조 리간드를 놓을 공간이 없습니다. 다른 조합을 선택하세요.");
                return false;
            }
        }
        m.charge=charge;m.multiplicity=multiplicity;
        m.title=elementSymbol(metal)+" + "+ligand.name+" + "+adduct.name;
        if(coligand)m.title+=" + "+coligand->name;
        if(const auto *reference=referenceFor(ligand,adduct,metal,oxidation,charge,multiplicity,coligand)) {
            if(reference->positions.size()!=m.atoms.size()) {
                if(error)*error=QStringLiteral("최적화 구조 템플릿의 원자 수가 조합과 다릅니다.");return false;
            }
            for(int i=0;i<m.atoms.size();++i){
                if(m.atoms[i].atomicNumber!=reference->atomicNumbers[i]) {
                    if(error)*error=QStringLiteral("최적화 구조 템플릿의 원자 순서가 조합과 다릅니다.");return false;
                }
                m.atoms[i].position=reference->positions[i];
            }
            m.title+=QStringLiteral(" · optimized M=%1 · %2").arg(multiplicity).arg(reference->source);
        }
        if(!MoleculeEditController::validate(m,error))return false;
        result=std::move(m);return true;
    }
};
}
