
constexpr double kHartreeToKcal = 627.509474;

bool matchNumber(const QString &text, const QRegularExpression &expression, double &value) {
    const auto match = expression.match(text);
    if (!match.hasMatch()) {
        return false;
    }
    bool ok = false;
    value = parser::toDouble(match.captured(1), &ok);
    return ok;
}

QVector<double> numbersAfterMarker(const QString &line, const QString &marker) {
    const int markerIndex = line.indexOf(marker);
    if (markerIndex < 0) {
        return {};
    }
    const QStringList fields = parser::columns(line.mid(markerIndex + marker.size()));
    QVector<double> values;
    values.reserve(fields.size());
    for (const QString &field : fields) {
        bool ok = false;
        const double value = parser::toDouble(field, &ok);
        if (ok) {
            values.push_back(value);
        }
    }
    return values;
}

QVector<Atom> readOrientationBlock(QFile &file) {
    // Four lines follow both "Input orientation:" and "Standard orientation:".
    for (int i = 0; i < 4 && !file.atEnd(); ++i) {
        file.readLine();
    }

    QVector<Atom> atoms;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if (line.contains(QStringLiteral("----"))) {
            break;
        }
        const QStringList fields = parser::columns(line);
        if (fields.size() < 6) {
            continue;
        }
        bool okIndex = false;
        bool okAtomic = false;
        const int index = fields[0].toInt(&okIndex);
        const int atomicNumber = fields[1].toInt(&okAtomic);
        if (!okIndex || !okAtomic) {
            continue;
        }
        bool okX = false;
        bool okY = false;
        bool okZ = false;
        const double x = parser::toDouble(fields[3], &okX);
        const double y = parser::toDouble(fields[4], &okY);
        const double z = parser::toDouble(fields[5], &okZ);
        if (!okX || !okY || !okZ) {
            continue;
        }
        Atom atom;
        atom.index = index;
        atom.atomicNumber = atomicNumber;
        atom.element = elementSymbol(atomicNumber);
        atom.position = QVector3D(float(x), float(y), float(z));
        atoms.push_back(atom);
    }
    return atoms;
}

Frame frameFromAtoms(const QVector<Atom> &atoms, int jobIndex, const FrameMetrics &metrics) {
    Frame frame;
    frame.jobIndex = jobIndex;
    frame.metrics = metrics;
    frame.coordinates.reserve(atoms.size());
    for (const Atom &atom : atoms) {
        frame.coordinates.push_back(atom.position);
    }
    return frame;
}

void applyCharges(QVector<Atom> &atoms, const QVector<double> &values) {
    for (int i = 0; i < atoms.size() && i < values.size(); ++i) {
        atoms[i].charge = values[i];
    }
}

void applySpins(QVector<Atom> &atoms, const QVector<double> &values) {
    for (int i = 0; i < atoms.size() && i < values.size(); ++i) {
        atoms[i].spinDensity = values[i];
    }
}

QVector<double> readAtomicValues(QFile &file, bool spin) {
    QVector<double> values;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if ((spin && line.contains(QStringLiteral("Sum of Mulliken"), Qt::CaseInsensitive)) ||
            (!spin && line.contains(QStringLiteral("Sum of Mulliken"), Qt::CaseInsensitive))) {
            break;
        }
        const QStringList fields = parser::columns(line);
        if (fields.size() < 3) {
            if (!values.isEmpty() && line.trimmed().isEmpty()) {
                break;
            }
            continue;
        }
        bool okIndex = false;
        fields[0].toInt(&okIndex);
        if (!okIndex) {
            continue;
        }
        bool ok = false;
        const double value = parser::toDouble(fields.last(), &ok);
        if (ok) {
            values.push_back(value);
        }
    }
    return values;
}

void readForceBlock(QFile &file, QVector<Atom> &atoms) {
    QVector<QVector3D> forces;
    int headerLines = 0;
    while (!file.atEnd()) {
        const qint64 position = file.pos();
        const QString line = QString::fromUtf8(file.readLine());
        if (line.contains(QStringLiteral("----")) && !forces.isEmpty()) break;
        if (forces.isEmpty() && ++headerLines > 4) { file.seek(position); break; }
        if (line.trimmed().isEmpty()) {
            if (!forces.isEmpty()) {
                break;
            }
            continue;
        }
        const QStringList fields = parser::columns(line);
        if (fields.size() < 5) {
            if (forces.isEmpty() && line.contains(QStringLiteral("----"))) continue;
            file.seek(position);
            break;
        }
        bool okIndex = false;
        fields[0].toInt(&okIndex);
        if (!okIndex) {
            if (forces.isEmpty() && line.contains(QStringLiteral("Number"), Qt::CaseInsensitive)) continue;
            file.seek(position);
            break;
        }
        bool okX = false;
        bool okY = false;
        bool okZ = false;
        const int last = fields.size() - 1;
        const double x = parser::toDouble(fields[last - 2], &okX);
        const double y = parser::toDouble(fields[last - 1], &okY);
        const double z = parser::toDouble(fields[last], &okZ);
        if (okX && okY && okZ) {
            forces.push_back(QVector3D(float(x), float(y), float(z)));
        }
    }
    for (int i = 0; i < atoms.size() && i < forces.size(); ++i) {
        atoms[i].force = forces[i];
        atoms[i].hasForce = true;
    }
}

bool parseExcitedState(const QString &line, ExcitedState &state) {
    static const QRegularExpression re(
        QStringLiteral(R"(Excited State\s+(\d+)\s*:.*?([+-]?[0-9]+(?:\.[0-9]*)?)\s*eV\s+([+-]?[0-9]+(?:\.[0-9]*)?)\s*nm\s+f\s*=\s*([+-]?[0-9.EeDd]+))"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = re.match(line);
    if (!match.hasMatch()) {
        return false;
    }
    bool okState = false;
    bool okEv = false;
    bool okNm = false;
    bool okF = false;
    state.state = match.captured(1).toInt(&okState);
    state.energyEv = parser::toDouble(match.captured(2), &okEv);
    state.wavelengthNm = parser::toDouble(match.captured(3), &okNm);
    state.oscillatorStrength = parser::toDouble(match.captured(4), &okF);
    return okState && okEv;
}

bool parseTransition(const QString &line, OrbitalTransition &transition) {
    static const QRegularExpression re(
        QStringLiteral(R"((\d+[A-Za-z]?\s*->\s*\d+[A-Za-z]?)\s+([+-]?[0-9.EeDd]+))"));
    const auto match = re.match(line);
    if (!match.hasMatch()) {
        return false;
    }
    const QStringList orbitals = match.captured(1).split(QStringLiteral("->"));
    if (orbitals.size() != 2) {
        return false;
    }
    transition.from = orbitals[0].trimmed();
    transition.to = orbitals[1].trimmed();
    bool ok = false;
    transition.coefficient = parser::toDouble(match.captured(2), &ok);
    return ok;
}

struct CompletedScanFrame {
    qint64 offset;
    int frameIndex;
    double energy;
    int jobIndex;
    QVector<int> optimizationFrames;
};

bool isJobBoundary(const QString &line) {
    return line == QStringLiteral("--Link1--") ||
           (line.startsWith(QStringLiteral("Link1:"), Qt::CaseInsensitive) &&
            line.contains(QStringLiteral("Proceeding to internal job step number"), Qt::CaseInsensitive)) ||
           line.contains(QStringLiteral("Entering Gaussian System"), Qt::CaseInsensitive);
}

struct ScanDefinition {
    QString parameter;
    QString type;
    QVector<int> atoms;
};

struct OptimizedScanSummary {
    qint64 offset = -1;
    int jobIndex = -1;
    bool detected = false;
    QString parameter;
    QString type;
    QVector<int> atoms;
    QVector<ScanDefinition> definitions;
    double energyOffset = 0.0;
    QVector<double> energies;
    QVector<double> coordinates;
};

QVector<double> scanNumbers(const QString &text) {
    static const QRegularExpression numberRe(
        QStringLiteral(R"([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[EeDd][+-]?\d+)?|\*+|NaN|N/A)"), QRegularExpression::CaseInsensitiveOption);
    QVector<double> values;
    auto matches = numberRe.globalMatch(text);
    while (matches.hasNext()) {
        bool ok = false;
        const double value = parser::toDouble(matches.next().captured(), &ok);
        values.push_back(ok && std::isfinite(value) ? value : kNaN);
    }
    return values;
}

OptimizedScanSummary readOptimizedScanSummary(const QString &path) {
    QFile file(path);
    OptimizedScanSummary summary;
    if (!file.open(QIODevice::ReadOnly)) return summary;
    const QRegularExpression scanRe(QStringLiteral(R"(!\s*([RAD]\d+)\s+([RAD])\(([^)]*)\).*\bScan\b)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression directiveRe(QStringLiteral(R"(^\s*([BAD])\s+((?:\d+\s+){2,4})S\s+\d+\s+[+\-0-9.EeDd]+\s*$)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression offsetRe(QStringLiteral(R"(add\s+([+\-0-9.EeDd]+)\s+to energies)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression rowRe(QStringLiteral(R"(^\s*([RAD]\d+)\s+(.+)$)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression columnsRe(QStringLiteral(R"(^\s*\d+(?:\s+\d+)*\s*$)"));
    QVector<ScanDefinition> definitions;
    QVector<ScanDefinition> directiveDefinitions;
    QString parameter;
    bool reading = false;
    bool coordinateFound = false;
    int blockStart = 0;
    int blockSize = 0;
    int jobIndex = 0;
    QVector<int> columns;
    QVector<int> columnPositions;
    while (!file.atEnd()) {
        const qint64 offset = file.pos();
        const QString line = QString::fromUtf8(file.readLine());
        if (isJobBoundary(line.trimmed())) {
            ++jobIndex;
            definitions.clear();
            directiveDefinitions.clear();
            reading = false;
        }
        const auto scan = scanRe.match(line);
        const auto directive = directiveRe.match(line);
        if (scan.hasMatch() || directive.hasMatch()) {
            if (summary.jobIndex != jobIndex) summary = {};
            summary.jobIndex = jobIndex;
            summary.detected = true;
            ScanDefinition definition;
            if (scan.hasMatch()) definition.parameter = scan.captured(1).toUpper();
            definition.type = scan.hasMatch() ? scan.captured(2).toUpper() : directive.captured(1).toUpper();
            if (definition.type == QStringLiteral("B")) definition.type = QStringLiteral("R");
            const auto values = scanNumbers(scan.hasMatch() ? scan.captured(3) : directive.captured(2));
            for (double value : values) definition.atoms.push_back(int(value) - 1);
            auto &target = scan.hasMatch() ? definitions : directiveDefinitions;
            const bool duplicate = std::any_of(target.cbegin(), target.cend(), [&](const ScanDefinition &existing) {
                return (!definition.parameter.isEmpty() && existing.parameter == definition.parameter) ||
                       (existing.type == definition.type && existing.atoms == definition.atoms);
            });
            if (!duplicate) target.push_back(definition);
            summary.definitions = definitions.isEmpty() ? directiveDefinitions : definitions;
            if (summary.definitions.size() == 1) {
                summary.parameter = summary.definitions.first().parameter;
                summary.type = summary.definitions.first().type;
                summary.atoms = summary.definitions.first().atoms;
            } else {
                summary.parameter.clear(); summary.type.clear(); summary.atoms.clear();
            }
        }
        if (line.contains(QStringLiteral("Summary of Optimized Potential Surface Scan"), Qt::CaseInsensitive)) {
            if (summary.jobIndex != jobIndex) summary = {};
            summary.energies.clear(); summary.coordinates.clear();
            summary.jobIndex = jobIndex;
            summary.detected = true;
            summary.offset = offset;
            summary.definitions = definitions.isEmpty() ? directiveDefinitions : definitions;
            parameter = summary.definitions.size() == 1 ? summary.definitions.first().parameter : QString();
            if (summary.definitions.size() == 1) {
                summary.parameter = summary.definitions.first().parameter;
                summary.type = summary.definitions.first().type;
                summary.atoms = summary.definitions.first().atoms;
            } else {
                summary.parameter.clear(); summary.type.clear(); summary.atoms.clear();
            }
            const auto energyOffset = offsetRe.match(line);
            summary.energyOffset = energyOffset.hasMatch() ? parser::toDouble(energyOffset.captured(1)) : 0.0;
            reading = true;
            coordinateFound = false;
            blockSize = 0;
            columns.clear(); columnPositions.clear();
            continue;
        }
        if (!reading) continue;
        if (columnsRe.match(line).hasMatch()) {
            columns.clear();
            columnPositions.clear();
            for (const auto &value : parser::columns(line)) columns.push_back(value.toInt() - 1);
            static const QRegularExpression integerRe(QStringLiteral("\\d+"));
            auto matches = integerRe.globalMatch(line);
            while (matches.hasNext()) { const auto match = matches.next(); columnPositions.push_back(match.capturedEnd() - 1); }
            continue;
        }
        const int marker = line.indexOf(QStringLiteral("Eigenvalues --"), 0, Qt::CaseInsensitive);
        if (marker >= 0) {
            auto energies = scanNumbers(line.mid(marker + 14));
            // Gaussian's fixed-width columns may contain entirely blank slots.
            // Use column centres only for an aligned, wide Gaussian table, not
            // compact fixtures or variable-width D/E exponent output.
            if (energies.size() < columns.size() && columnPositions.size() > 1) {
                const int width = columnPositions[1] - columnPositions[0];
                const int start = columnPositions[0] - width / 2;
                if (width >= 10 && start >= marker + 14) {
                    QVector<double> aligned;
                    for (int i = 0; i < columns.size(); ++i) {
                        const auto slot = scanNumbers(line.mid(start + i * width, width));
                        aligned.push_back(slot.size() == 1 ? slot[0] : kNaN);
                    }
                    energies = aligned;
                }
            }
            blockStart = summary.energies.size();
            blockSize = columns.isEmpty() ? energies.size() : columns.size();
            if (!columns.isEmpty()) blockStart = columns.first();
            while (summary.energies.size() < blockStart + blockSize) {
                summary.energies.push_back(kNaN); summary.coordinates.push_back(kNaN);
            }
            for (int i = 0; i < blockSize && i < energies.size(); ++i)
                summary.energies[blockStart + i] = energies[i] + summary.energyOffset;
            continue;
        }
        const auto row = rowRe.match(line);
        if (row.hasMatch()) {
            if (!parameter.isEmpty() && row.captured(1).compare(parameter, Qt::CaseInsensitive) == 0) {
                coordinateFound = true;
                const auto values = scanNumbers(row.captured(2));
                for (int i = 0; i < blockSize && i < values.size(); ++i)
                    summary.coordinates[blockStart + i] = values[i];
            }
            continue;
        }
        if (!line.trimmed().isEmpty() && !columnsRe.match(line).hasMatch()) reading = false;
    }
    // Unknown or ambiguous coordinates use ordinal positions; invalid known rows stay invalid.
    if (parameter.isEmpty() || !coordinateFound) {
        for (int i = 0; i < summary.coordinates.size(); ++i) summary.coordinates[i] = i + 1;
        if (summary.offset >= 0) { summary.type.clear(); summary.atoms.clear(); }
    }
    return summary;
}

double scanCoordinate(const Frame &frame, const ScanDefinition &definition) {
    const auto &a = definition.atoms;
    for (int index : a) if (index < 0 || index >= frame.coordinates.size()) return kNaN;
    const auto &p = frame.coordinates;
    if (definition.type == QStringLiteral("R") && a.size() == 2) return double((p[a[0]] - p[a[1]]).length());
    constexpr double degrees = 180.0 / 3.14159265358979323846;
    if (definition.type == QStringLiteral("A") && a.size() == 3) {
        const auto u = p[a[0]] - p[a[1]], v = p[a[2]] - p[a[1]];
        if (u.lengthSquared() == 0 || v.lengthSquared() == 0) return kNaN;
        return std::acos(std::clamp(double(QVector3D::dotProduct(u.normalized(), v.normalized())), -1.0, 1.0)) * degrees;
    }
    if (definition.type == QStringLiteral("D") && a.size() == 4) {
        const auto b0 = p[a[0]] - p[a[1]], b1 = p[a[2]] - p[a[1]], b2 = p[a[3]] - p[a[2]];
        if (b1.lengthSquared() == 0) return kNaN;
        const auto axis = b1.normalized();
        const auto v = b0 - QVector3D::dotProduct(b0, axis) * axis;
        const auto w = b2 - QVector3D::dotProduct(b2, axis) * axis;
        if (v.lengthSquared() == 0 || w.lengthSquared() == 0) return kNaN;
        return std::atan2(double(QVector3D::dotProduct(QVector3D::crossProduct(axis, v), w)), double(QVector3D::dotProduct(v, w))) * degrees;
    }
    return kNaN;
}

double scanCoordinate(const Frame &frame, const OptimizedScanSummary &definition) {
    return scanCoordinate(frame, ScanDefinition{definition.parameter, definition.type, definition.atoms});
}

void buildScanData(CalculationData &data,
                   const QVector<CompletedScanFrame> &completed,
                   QVector<int> pendingFrames) {
    const auto summary = readOptimizedScanSummary(data.filePath);
    data.scanDetected = summary.detected;
    if (!summary.detected) return;
    data.scanHasSummary = summary.offset >= 0;
    data.scanDimensions = summary.definitions.size() == 2 ? 2 : 1;
    data.scanCoordinateType = summary.type;
    data.scanAtoms = summary.definitions.isEmpty() ? summary.atoms : summary.definitions.first().atoms;
    auto axisLabel = [](const ScanDefinition &definition) {
        return QStringLiteral("%1 (%2)").arg(definition.parameter.isEmpty() ? definition.type : definition.parameter,
                                             definition.type == QStringLiteral("R") ? QStringLiteral("Å") : QStringLiteral("degree"));
    };
    if (data.scanDimensions == 2) {
        data.scanCoordinateType = summary.definitions[0].type;
        data.scanAxisLabel = axisLabel(summary.definitions[0]);
        data.scanAxisLabel2 = axisLabel(summary.definitions[1]);
    } else {
        data.scanAxisLabel = summary.type.isEmpty() ? QStringLiteral("Point index") :
            QStringLiteral("%1 (%2)").arg(summary.parameter.isEmpty() ? summary.type : summary.parameter,
                                         summary.type == QStringLiteral("R") ? QStringLiteral("Å") : QStringLiteral("degree"));
    }
    QVector<CompletedScanFrame> eligible;
    for (const auto &frame : completed)
        if (frame.jobIndex == summary.jobIndex && (summary.offset < 0 || frame.offset < summary.offset)) eligible.push_back(frame);
    if (data.scanDimensions == 2) {
        for (int i = 0; i < eligible.size(); ++i) {
            const auto &completedFrame = eligible[i];
            if (completedFrame.frameIndex < 0 || completedFrame.frameIndex >= data.frames.size() ||
                !std::isfinite(completedFrame.energy)) continue;
            const Frame &frame = data.frames[completedFrame.frameIndex];
            const double coordinate = scanCoordinate(frame, summary.definitions[0]);
            const double coordinate2 = scanCoordinate(frame, summary.definitions[1]);
            if (std::isfinite(coordinate) && std::isfinite(coordinate2))
                data.scanPoints.push_back({i + 1, coordinate, coordinate2, completedFrame.energy, 0.0,
                                           completedFrame.frameIndex, {}, false});
        }
    } else if (summary.offset >= 0) {
        const int count = summary.energies.size();
        QVector<bool> used(eligible.size(), false);
        for (int i = 0; i < count; ++i) {
            if (!std::isfinite(summary.energies[i]) || !std::isfinite(summary.coordinates[i])) continue;
            int frame = -1;
            // Match recorded completion energies, never align unrelated arrays by position.
            for (int j = 0; j < eligible.size(); ++j) {
                if (!used[j] && std::isfinite(eligible[j].energy) &&
                    std::abs(eligible[j].energy - summary.energies[i]) <= 0.000006) {
                    frame = eligible[j].frameIndex; used[j] = true; break;
                }
            }
            data.scanPoints.push_back({i + 1, summary.coordinates[i], kNaN, summary.energies[i], 0.0, frame, {}, false});
        }
    } else {
        for (int i = 0; i < eligible.size(); ++i) {
            const auto &frame = eligible[i];
            double coordinate = frame.frameIndex >= 0 && frame.frameIndex < data.frames.size()
                ? scanCoordinate(data.frames[frame.frameIndex], summary) : kNaN;
            if (!std::isfinite(coordinate)) coordinate = i + 1;
            if (std::isfinite(frame.energy))
                data.scanPoints.push_back({i + 1, coordinate, kNaN, frame.energy, 0.0, frame.frameIndex, {}, false});
        }
    }
    // Associate by the actual completion frame, including when summary rows are missing.
    for (auto &point : data.scanPoints) {
        for (const auto &completion : eligible) {
            if (point.frameIndex >= 0 && completion.frameIndex == point.frameIndex) {
                point.optimizationFrames = completion.optimizationFrames;
                break;
            }
        }
    }
    // Only the scan job contributes an unfinished point; a later Link1 job must not.
    // A new geometry may already be available while its first SCF is still running.
    if (!data.scanHasSummary && !data.frames.isEmpty()) {
        const int latest = data.frames.size() - 1;
        const auto &frame = data.frames[latest];
        const int lastCompleted = eligible.isEmpty() ? -1 : eligible.last().frameIndex;
        if (frame.jobIndex == summary.jobIndex && !std::isfinite(frame.metrics.energyHartree) &&
            latest > lastCompleted &&
            (!pendingFrames.isEmpty() || lastCompleted < 0 || frame.coordinates != data.frames[lastCompleted].coordinates))
            pendingFrames.push_back(latest);
    }
    if (!data.scanHasSummary && !pendingFrames.isEmpty() &&
        data.frames[pendingFrames.last()].jobIndex == summary.jobIndex) {
        ScanPoint point;
        point.index = eligible.size() + 1;
        point.frameIndex = pendingFrames.last();
        point.optimizationFrames = pendingFrames;
        point.inProgress = true;
        const auto &frame = data.frames[point.frameIndex];
        point.energyHartree = frame.metrics.energyHartree;
        point.coordinate = data.scanDimensions == 2
            ? scanCoordinate(frame, summary.definitions[0]) : scanCoordinate(frame, summary);
        if (!std::isfinite(point.coordinate)) point.coordinate = point.index;
        if (data.scanDimensions == 2) point.coordinate2 = scanCoordinate(frame, summary.definitions[1]);
        data.scanPoints.push_back(point);
    }
    if (data.scanPoints.isEmpty()) return;
    double reference = std::numeric_limits<double>::infinity();
    for (const auto &point : data.scanPoints)
        if (std::isfinite(point.energyHartree)) reference = std::min(reference, point.energyHartree);
    for (auto &point : data.scanPoints) point.relativeKcal = (point.energyHartree - reference) * kHartreeToKcal;
}

void buildIrcData(CalculationData &data, bool ircDetected,
                         const QString &ircDirection) {
    if (data.scfEnergies.isEmpty()) {
        return;
    }
    const int count = std::min(data.frames.size(), data.scfEnergies.size());
    if (count <= 0) {
        return;
    }
    QVector<double> energies;
    energies.reserve(count);
    const int energyOffset = data.scfEnergies.size() - count;
    for (int i = 0; i < count; ++i) {
        energies.push_back(data.scfEnergies[energyOffset + i]);
    }
    const double reference = *std::min_element(energies.cbegin(), energies.cend());
    if (ircDetected) {
        data.ircPoints.clear();
        for (int i = 0; i < energies.size(); ++i) {
            const double coordinate = ircDirection.compare(QStringLiteral("reverse"), Qt::CaseInsensitive) == 0
                                          ? double(energies.size() - i - 1)
                                          : double(i);
            data.ircPoints.push_back({i + 1, coordinate, energies[i],
                                      (energies[i] - reference) * kHartreeToKcal,
                                      int(data.frames.size() - count + i), ircDirection});
        }
    }
}
