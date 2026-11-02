#include "io/FchkParser.h"

#include "io/ParserCommon.h"

#include <QFile>

namespace youngcraft {

namespace {

int arrayCount(const QString &header) {
    static const QRegularExpression re(QStringLiteral(R"(N\s*=\s*(\d+))"));
    const auto match = re.match(header);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

QVector<double> readRealArray(QFile &file, int count) {
    QVector<double> values;
    values.reserve(count);
    while (!file.atEnd() && values.size() < count) {
        const QStringList fields = parser::columns(QString::fromUtf8(file.readLine()));
        for (const QString &field : fields) {
            bool ok = false;
            const double value = parser::toDouble(field, &ok);
            if (ok) values.push_back(value);
            if (values.size() == count) break;
        }
    }
    return values;
}

QVector<int> readIntArray(QFile &file, int count) {
    QVector<int> values;
    values.reserve(count);
    while (!file.atEnd() && values.size() < count) {
        const QStringList fields = parser::columns(QString::fromUtf8(file.readLine()));
        for (const QString &field : fields) {
            bool ok = false;
            const int value = field.toInt(&ok);
            if (ok) values.push_back(value);
            if (values.size() == count) break;
        }
    }
    return values;
}

} // namespace

ParseResult FchkParser::parse(const QString &filePath) {
    ParseResult result;
    result.data.filePath = filePath;
    result.data.format = QStringLiteral("FCHK");
    result.data.warningMessage = QStringLiteral("FCHK의 구조와 MO 메타데이터를 읽었습니다. Orbitals에서 cubegen으로 FMO / MEP를 생성할 수 있습니다.");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("FCHK 파일을 열 수 없습니다: %1").arg(file.errorString());
        return result;
    }
    int atomCount = 0;
    QVector<int> atomicNumbers;
    QVector<double> coordinates;
    QVector<double> orbitalEnergies;
    QVector<double> orbitalOccupations;
    QVector<double> betaEnergies;
    int alphaCount = -1, betaCount = -1;
    QString title;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        if (line.startsWith(QStringLiteral("Number of alpha electrons"))) {
            alphaCount = parser::columns(line).last().toInt();
        } else if (line.startsWith(QStringLiteral("Number of beta electrons"))) {
            betaCount = parser::columns(line).last().toInt();
        } else if (line.startsWith(QStringLiteral("Beta Orbital Energies"))) {
            betaEnergies = readRealArray(file, arrayCount(line));
        } else if (line.startsWith(QStringLiteral("Number of atoms"))) {
            const QStringList fields = parser::columns(line);
            if (!fields.isEmpty()) atomCount = fields.last().toInt();
        } else if (line.startsWith(QStringLiteral("Charge"))) {
            const QStringList fields = parser::columns(line);
            if (!fields.isEmpty()) result.data.molecule.charge = fields.last().toInt();
        } else if (line.startsWith(QStringLiteral("Multiplicity"))) {
            const QStringList fields = parser::columns(line);
            if (!fields.isEmpty()) result.data.molecule.multiplicity = fields.last().toInt();
        } else if (line.startsWith(QStringLiteral("Atomic numbers"))) {
            atomicNumbers = readIntArray(file, arrayCount(line));
        } else if (line.startsWith(QStringLiteral("Current cartesian coordinates"))) {
            coordinates = readRealArray(file, arrayCount(line));
        } else if (line.startsWith(QStringLiteral("Alpha Orbital Energies"))) {
            orbitalEnergies = readRealArray(file, arrayCount(line));
        } else if (line.startsWith(QStringLiteral("Alpha Orbital Occupancies"))) {
            orbitalOccupations = readRealArray(file, arrayCount(line));
        } else if (title.isEmpty() && !line.trimmed().isEmpty()) {
            title = line.trimmed();
        }
    }
    if(atomCount<=0 || atomicNumbers.size()!=atomCount || coordinates.size()!=qint64(atomCount)*3) {
        result.error=QStringLiteral("FCHK 원자 또는 좌표 배열이 불완전합니다.");return result;
    }
    const int count = atomCount;
    for (int i = 0; i < count; ++i) {
        Atom atom;
        atom.index = i + 1;
        atom.atomicNumber = atomicNumbers[i];
        atom.element = elementSymbol(atom.atomicNumber);
        atom.position = QVector3D(float(coordinates[i * 3] * 0.529177210903),
                                  float(coordinates[i * 3 + 1] * 0.529177210903),
                                  float(coordinates[i * 3 + 2] * 0.529177210903));
        result.data.molecule.atoms.push_back(atom);
    }
    result.data.molecule.title = title;
    result.data.molecule.rebuildBonds();
    Frame frame;
    frame.metrics.step = 1;
    for (const Atom &atom : result.data.molecule.atoms) frame.coordinates.push_back(atom.position);
    if (!frame.coordinates.isEmpty()) result.data.frames.push_back(frame);
    for (int i = 0; i < orbitalEnergies.size(); ++i) {
        result.data.orbitals.push_back({i + 1, orbitalEnergies[i] * 27.211386245988,
                                        i < orbitalOccupations.size() ? orbitalOccupations[i] :
                                        alphaCount < 0 ? kNaN : (betaEnergies.isEmpty() ?
                                        double(i < alphaCount) + double(i < betaCount) : double(i < alphaCount)),
                                        QString()});
    }
    for (int i = 0; i < betaEnergies.size(); ++i)
        result.data.orbitals.push_back({i + 1, betaEnergies[i] * 27.211386245988,
            betaCount < 0 ? kNaN : double(i < betaCount), {}, QStringLiteral("Beta")});
    result.data.status = CalculationStatus::NormalTermination;
    result.success = !result.data.molecule.atoms.isEmpty();
    if (!result.success) result.error = QStringLiteral("FCHK 구조를 찾지 못했습니다.");
    return result;
}

} // namespace youngcraft
