// Read the saved NTO basis separately from canonical excitation amplitudes.
struct NativeNtoCollector {
    int state = -1;
    qint64 offset = 0;
    QString spin;
    QVector<double> occupied, virtuals;
    void finish(CalculationData &data) {
        if(state<0 || occupied.isEmpty() || virtuals.size()<occupied.size()) {state=-1;return;}
        QVector<ExcitedState::NativeNtoPair> pairs;
        for(int rank=0;rank<occupied.size();++rank) {
            const int hole=int(occupied.size())-rank;
            const double weight=occupied[hole-1];
            if(!std::isfinite(weight) || weight<0 || std::abs(weight-virtuals[rank])>1e-4*std::max(1.0,weight)) {state=-1;return;}
            if(weight>0) pairs.append({QString::number(hole)+spin,QString::number(occupied.size()+rank+1)+spin,weight});
        }
        if(!pairs.isEmpty()) {
            auto found=std::find_if(data.excitedStates.begin(),data.excitedStates.end(),[this](const auto &s){return s.state==state;});
            if(found==data.excitedStates.end()) {
                ExcitedState entry;entry.state=state;entry.label=QStringLiteral("NTO S%1").arg(state);entry.frameIndex=data.finalFrameIndex();
                data.excitedStates.append(entry);found=data.excitedStates.end()-1;
                data.index.sectionOffsets[QStringLiteral("excited_states")].append(offset);
            }
            auto &saved=found->nativeNtoPairs;
            saved.erase(std::remove_if(saved.begin(),saved.end(),[this](const auto &p){return p.hole.endsWith(spin);}),saved.end());
            saved+=pairs;
        }
        state=-1;
    }
    void observe(const QString &line,qint64 position,CalculationData &data) {
        static const QRegularExpression start(QStringLiteral(R"(Saving\s+(Alpha|Beta)\s+spin\s+NTOs\s+for\s+state\s+(\d+))"));
        static const QRegularExpression values(QStringLiteral(R"(^\s*(Alpha|Beta)\s+(occ\.|virt\.)\s+eigenvalues\s*--)"));
        const auto begin=start.match(line);
        if(begin.hasMatch()) {
            finish(data);state=begin.captured(2).toInt();spin=begin.captured(1)=="Beta"?"b":"a";
            offset=position;occupied.clear();virtuals.clear();return;
        }
        if(state<0)return;
        const auto eigen=values.match(line);
        if(eigen.hasMatch() && (eigen.captured(1)=="Beta"?"b":"a")==spin) {
            auto &target=eigen.captured(2).startsWith("occ")?occupied:virtuals;
            target+=numbersAfterMarker(line,QStringLiteral("--"));
        }
        if(line.contains(QStringLiteral("Natural Transition Orbital Coefficients")))finish(data);
    }
};
