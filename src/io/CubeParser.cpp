#include "io/CubeParser.h"

#include "io/ParserCommon.h"

#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>

namespace youngcraft {

CubeGrid CubeParser::parse(const QString &filePath) {
    CubeGrid grid;
    grid.filePath = filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        grid.error = QStringLiteral("CUBE 파일을 열 수 없습니다: %1").arg(file.errorString());
        return grid;
    }
    QTextStream stream(&file);
    grid.comment1 = stream.readLine();
    grid.comment2 = stream.readLine();
    const QStringList atomHeader = parser::columns(stream.readLine());
    if (atomHeader.size() < 4) {
        grid.error = QStringLiteral("CUBE 헤더가 올바르지 않습니다.");
        return grid;
    }
    bool okAtoms = false, okX = false, okY = false, okZ = false;
    int atomCountSigned = atomHeader[0].toInt(&okAtoms);
    const double ox = parser::toDouble(atomHeader[1], &okX);
    const double oy = parser::toDouble(atomHeader[2], &okY);
    const double oz = parser::toDouble(atomHeader[3], &okZ);
    if (!okAtoms || !okX || !okY || !okZ) {
        grid.error = QStringLiteral("CUBE 원자/원점 정보를 읽을 수 없습니다.");
        return grid;
    }
    double unit = 0.529177210903;
    int axisSign = 0;
    const int atomCount = std::abs(atomCountSigned);
    grid.origin = QVector3D(float(ox * unit), float(oy * unit), float(oz * unit));

    auto readAxis = [&](QVector3D &axis, int &count) -> bool {
        const QStringList fields = parser::columns(stream.readLine());
        if (fields.size() < 4) return false;
        bool okCount = false, okA = false, okB = false, okC = false;
        const int signedCount = fields[0].toInt(&okCount);
        const double a = parser::toDouble(fields[1], &okA);
        const double b = parser::toDouble(fields[2], &okB);
        const double c = parser::toDouble(fields[3], &okC);
        if (!okCount || !okA || !okB || !okC) return false;
        const int sign = signedCount < 0 ? -1 : 1;
        if (axisSign && sign != axisSign) return false;
        axisSign = sign;
        unit = sign < 0 ? 1.0 : 0.529177210903;
        count = std::abs(signedCount);
        axis = QVector3D(float(a * unit), float(b * unit), float(c * unit));
        return count > 0;
    };
    if (!readAxis(grid.axisX, grid.nx) || !readAxis(grid.axisY, grid.ny) ||
        !readAxis(grid.axisZ, grid.nz)) {
        grid.error = QStringLiteral("CUBE 격자 축을 읽을 수 없습니다.");
        return grid;
    }

    grid.origin = QVector3D(float(ox * unit), float(oy * unit), float(oz * unit));
    if (atomHeader.size() > 4 && atomHeader[4].toInt() > 1) {
        grid.error = QStringLiteral("다중 데이터 cube는 단일 MO cube로 나누어 주세요."); return grid;
    }
    if (atomCount > 100000) {
        grid.error = QStringLiteral("CUBE 원자 수가 너무 큽니다.");
        return grid;
    }
    grid.molecule.atoms.reserve(atomCount);
    for (int i = 0; i < atomCount; ++i) {
        const QStringList fields = parser::columns(stream.readLine());
        if (fields.size() < 5) {
            grid.error = QStringLiteral("CUBE 원자 행을 읽을 수 없습니다.");
            return grid;
        }
        bool okNumber = false, okCharge = false, okA = false, okB = false, okC = false;
        const int number = fields[0].toInt(&okNumber);
        const double charge = parser::toDouble(fields[1], &okCharge);
        const double x = parser::toDouble(fields[2], &okA);
        const double y = parser::toDouble(fields[3], &okB);
        const double z = parser::toDouble(fields[4], &okC);
        if (!okNumber || !okA || !okB || !okC) continue;
        Atom atom;
        atom.index = i + 1;
        atom.atomicNumber = number;
        atom.element = elementSymbol(number);
        atom.charge = okCharge ? charge : kNaN;
        atom.position = QVector3D(float(x * unit), float(y * unit), float(z * unit));
        grid.molecule.atoms.push_back(atom);
    }

    const qint64 total = qint64(grid.nx) * grid.ny * grid.nz;
    if (total <= 0 || total > 50000000) {
        grid.error = QStringLiteral("CUBE 격자 크기가 지원 범위를 벗어났습니다.");
        return grid;
    }
    if (atomCountSigned < 0) {
        int orbitals = 0, orbitalId = 0; stream >> orbitals;
        if (orbitals != 1) { grid.error = QStringLiteral("단일 MO cube만 지원합니다."); return grid; }
        stream >> orbitalId;
    }
    grid.values.reserve(int(total));
    float minimum = std::numeric_limits<float>::max();
    float maximum = std::numeric_limits<float>::lowest();
    while (!stream.atEnd() && grid.values.size() < total) {
        QString token;
        stream >> token;
        if (token.isEmpty()) break;
        bool ok = false;
        const float value = float(parser::toDouble(token, &ok));
        if (!ok || !std::isfinite(value)) { grid.error = QStringLiteral("유효하지 않은 cube 값입니다."); return grid; }
        grid.values.push_back(value);
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    // Cube serialization has Z fastest; the renderer stores X fastest.
    if (grid.values.size() == total) {
        QVector<float> ordered(int(total), 0.0f);
        for (int x=0; x<grid.nx; ++x) for (int y=0; y<grid.ny; ++y) for (int z=0; z<grid.nz; ++z)
            ordered[(z*grid.ny+y)*grid.nx+x] = grid.values[(x*grid.ny+y)*grid.nz+z];
        grid.values = std::move(ordered);
    }
    grid.minValue = grid.values.isEmpty() ? 0.0f : minimum;
    grid.maxValue = grid.values.isEmpty() ? 0.0f : maximum;
    grid.molecule.rebuildBonds();
    grid.valid = grid.values.size() == total;
    if (!grid.valid) grid.error = QStringLiteral("CUBE 격자 값이 부족합니다.");
    return grid;
}

} // namespace youngcraft
