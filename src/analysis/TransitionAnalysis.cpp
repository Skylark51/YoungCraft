#include "analysis/TransitionAnalysis.h"
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace youngcraft {
TransitionAnalysis TransitionAnalysis::calculate(const ExcitedState &state) {
    TransitionAnalysis result;
    static const QRegularExpression label(QStringLiteral("^\\d+[ab]?$"));
    for (const auto &t : state.transitions) {
        const auto from=t.from.toLower(), to=t.to.toLower();
        if (!label.match(from).hasMatch() || !label.match(to).hasMatch() || !std::isfinite(t.coefficient)) {
            result.error=QStringLiteral("유효하지 않은 전이 계수 또는 MO 번호입니다."); return result;
        }
        if (!result.occupied.contains(from)) result.occupied.append(from);
        if (!result.virtuals.contains(to)) result.virtuals.append(to);
    }
    const int m=int(result.occupied.size()), n=int(result.virtuals.size());
    if (!m || !n) { result.error=QStringLiteral("이 상태에 전이 계수가 없습니다. 전이 계수를 포함한 TD 출력이 필요합니다."); return result; }
    if (std::max(m,n)>256) { result.error=QStringLiteral("전이 공간이 너무 큽니다 (각 방향 최대 256 MO)."); return result; }
    // One-sided Jacobi SVD of T, retaining signs and interference between MOs.
    QVector<double> a(m*n,0), v(n*n,0);
    for (int j=0;j<n;++j) v[j*n+j]=1;
    for (const auto &t:state.transitions)
        a[result.occupied.indexOf(t.from.toLower())*n+result.virtuals.indexOf(t.to.toLower())]+=t.coefficient;
    for (double x:a) result.printedNorm+=x*x;
    if (!std::isfinite(result.printedNorm) || result.printedNorm<=1e-24) {
        result.error=QStringLiteral("전이 계수의 크기가 0이거나 계산할 수 없습니다."); return result;
    }
    result.holePopulation.fill(0,m); result.particlePopulation.fill(0,n);
    for(int i=0;i<m;++i) for(int j=0;j<n;++j) {
        const double w=a[i*n+j]*a[i*n+j]/result.printedNorm;
        result.holePopulation[i]+=w; result.particlePopulation[j]+=w;
    }
    bool converged=false;
    for(int sweep=0;sweep<100;++sweep) {
        bool changed=false;
        for(int p=0;p<n;++p) for(int q=p+1;q<n;++q) {
            double aa=0,bb=0,ab=0;
            for(int i=0;i<m;++i) { aa+=a[i*n+p]*a[i*n+p]; bb+=a[i*n+q]*a[i*n+q]; ab+=a[i*n+p]*a[i*n+q]; }
            if(aa<result.printedNorm*1e-28 || bb<result.printedNorm*1e-28 || std::abs(ab)<=1e-12*std::sqrt(aa*bb)) continue;
            changed=true;
            const double tau=(bb-aa)/(2*ab);
            const double t=std::copysign(1.0,tau)/(std::abs(tau)+std::hypot(1.0,tau));
            const double c=1/std::sqrt(1+t*t), s=c*t;
            for(int i=0;i<m;++i) { const double x=a[i*n+p],y=a[i*n+q]; a[i*n+p]=c*x-s*y; a[i*n+q]=s*x+c*y; }
            for(int i=0;i<n;++i) { const double x=v[i*n+p],y=v[i*n+q]; v[i*n+p]=c*x-s*y; v[i*n+q]=s*x+c*y; }
        }
        if(!changed) { converged=true; break; }
    }
    if(!converged) { result.error=QStringLiteral("NTO 분해가 수렴하지 않았습니다."); return result; }
    for(int j=0;j<n;++j) {
        double norm=0; for(int i=0;i<m;++i) norm+=a[i*n+j]*a[i*n+j];
        if(norm<=result.printedNorm*1e-14) continue;
        NtoPair pair; pair.weight=norm/result.printedNorm;
        for(int i=0;i<m;++i) pair.hole.append(a[i*n+j]/std::sqrt(norm));
        for(int i=0;i<n;++i) pair.particle.append(v[i*n+j]);
        result.pairs.append(pair);
    }
    std::stable_sort(result.pairs.begin(),result.pairs.end(),[](const auto &a,const auto &b){return a.weight>b.weight;});
    return result;
}

std::shared_ptr<CubeGrid> TransitionAnalysis::grid(const QVector<std::shared_ptr<const CubeGrid>> &mos,
                                                bool particle, int pair, QString *error) const {
    if(error) error->clear();
    auto fail=[&](const QString &message)->std::shared_ptr<CubeGrid>{if(error)*error=message;return {};};
    const int count=int(particle?virtuals.size():occupied.size());
    if(!this->error.isEmpty() || pairs.isEmpty() || mos.size()!=count || !count || pair>=pairs.size() || pair< -1)
        return fail(QStringLiteral("MO 그리드 또는 NTO 쌍이 부족합니다."));
    const auto &base=mos.first();
    if(!base || !base->valid || base->nx<=0 || base->ny<=0 || base->nz<=0 || base->values.size()!=qint64(base->nx)*base->ny*base->nz)
        return fail(QStringLiteral("유효하지 않은 MO 그리드입니다."));
    for(const auto &g:mos) {
        if(!g || !g->valid || g->nx!=base->nx || g->ny!=base->ny || g->nz!=base->nz || g->values.size()!=base->values.size()
           || (g->origin-base->origin).length()>1e-6f || (g->axisX-base->axisX).length()>1e-6f
           || (g->axisY-base->axisY).length()>1e-6f || (g->axisZ-base->axisZ).length()>1e-6f)
            return fail(QStringLiteral("MO 그리드의 좌표계가 일치하지 않습니다."));
        if(g->molecule.atoms.size()!=base->molecule.atoms.size()) return fail(QStringLiteral("MO 그리드의 분자가 다릅니다."));
        for(int i=0;i<g->molecule.atoms.size();++i)
            if(g->molecule.atoms[i].atomicNumber!=base->molecule.atoms[i].atomicNumber ||
               (g->molecule.atoms[i].position-base->molecule.atoms[i].position).length()>1e-4f)
                return fail(QStringLiteral("MO 그리드의 분자 구조가 다릅니다."));
        for(float value:g->values) if(!std::isfinite(value)) return fail(QStringLiteral("MO 그리드에 유효하지 않은 값이 있습니다."));
    }
    auto out=std::make_shared<CubeGrid>(*base); out->filePath.clear(); out->values.fill(0);
    const int first=pair<0?0:pair, last=pair<0?int(pairs.size()):pair+1;
    QVector<double> amplitude(base->values.size());
    for(int p=first;p<last;++p) {
        amplitude.fill(0);
        const auto &coeff=particle?pairs[p].particle:pairs[p].hole;
        for(int i=0;i<count;++i) for(qsizetype k=0;k<amplitude.size();++k) amplitude[k]+=coeff[i]*mos[i]->values[k];
        for(qsizetype k=0;k<amplitude.size();++k)
            out->values[k]+=float(pair<0?pairs[p].weight*amplitude[k]*amplitude[k]:amplitude[k]);
    }
    const auto bounds=std::minmax_element(out->values.cbegin(),out->values.cend());
    out->minValue=*bounds.first; out->maxValue=*bounds.second;
    return out;
}
}
