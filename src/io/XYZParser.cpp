#include "io/XYZParser.h"

#include "io/ParserCommon.h"

#include <QFile>

namespace youngcraft {

ParseResult XYZParser::parseText(const QString &text) {
    ParseResult result;result.data.format="XYZ";
    QStringList lines=text.split('\n');
    while(!lines.isEmpty() && lines.first().trimmed().isEmpty())lines.removeFirst();
    while(!lines.isEmpty() && lines.last().trimmed().isEmpty())lines.removeLast();
    if(lines.isEmpty()){result.error=QStringLiteral("렌더링할 XYZ 좌표를 붙여넣으세요.");return result;}
    bool header=false;int count=lines.first().trimmed().toInt(&header);
    int start=0;
    if(header) {
        if(count<=0 || count>1000000 || lines.size()!=count+2){result.error=QStringLiteral("XYZ 원자 수와 좌표 행 수가 다릅니다. 원자 수 다음에 제목 행이 필요합니다 (한 구조씩 입력). ");return result;}
        start=2;result.data.molecule.title=lines[1].trimmed();
    } else count=lines.size();
    if(result.data.molecule.title.isEmpty())result.data.molecule.title=QStringLiteral("Pasted XYZ");
    for(int i=0;i<count;++i) {
        const auto fields=parser::columns(lines[start+i]);
        bool x=false,y=false,z=false;
        const int number=fields.size()==4?atomicNumberFor(fields[0]):0;
        const double px=parser::toDouble(fields.value(1),&x),py=parser::toDouble(fields.value(2),&y),pz=parser::toDouble(fields.value(3),&z);
        const QVector3D point{float(px),float(py),float(pz)};
        if(!number || !x || !y || !z || !std::isfinite(point.x()) || !std::isfinite(point.y()) || !std::isfinite(point.z())) {
            result.error=QStringLiteral("%1행: 올바른 원소와 유한한 X Y Z 좌표 3개를 입력하세요.").arg(start+i+1);return result;
        }
        Atom atom;atom.index=i+1;atom.atomicNumber=number;atom.element=elementSymbol(number);atom.position=point;result.data.molecule.atoms.push_back(atom);
    }
    result.data.molecule.rebuildBonds();result.data.status=CalculationStatus::NormalTermination;result.success=true;return result;
}

ParseResult XYZParser::parse(const QString &filePath) {
    ParseResult result;
    result.data.filePath = filePath;
    result.data.format = QStringLiteral("XYZ");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("파일을 열 수 없습니다: %1").arg(file.errorString());
        return result;
    }

    bool firstFrame = true;
    int frameNumber = 0;
    while (!file.atEnd()) {
        const QString countLine = QString::fromUtf8(file.readLine()).trimmed();
        if (countLine.isEmpty()) continue;
        bool okCount = false;
        const int count = countLine.toInt(&okCount);
        if (!okCount || count <= 0 || count > 1000000) {
            result.error = QStringLiteral("XYZ 원자 수를 읽을 수 없습니다.");
            break;
        }
        if (file.atEnd()) break;
        const QString comment = QString::fromUtf8(file.readLine()).trimmed();
        QVector<Atom> atoms;
        atoms.reserve(count);
        for (int i = 0; i < count && !file.atEnd(); ++i) {
            const QStringList fields = parser::columns(QString::fromUtf8(file.readLine()));
            if (fields.size() < 4) continue;
            bool okX = false, okY = false, okZ = false;
            const int atomicNumber = atomicNumberFor(fields[0]);
            const double x = parser::toDouble(fields[1], &okX);
            const double y = parser::toDouble(fields[2], &okY);
            const double z = parser::toDouble(fields[3], &okZ);
            if (!atomicNumber || !okX || !okY || !okZ) continue;
            Atom atom;
            atom.index = i + 1;
            atom.atomicNumber = atomicNumber;
            atom.element = elementSymbol(atomicNumber);
            atom.position = QVector3D(float(x), float(y), float(z));
            atoms.push_back(atom);
        }
        if (atoms.size() != count) continue;
        if (firstFrame) {
            result.data.molecule.atoms = atoms;
            result.data.molecule.title = comment;
            firstFrame = false;
        }
        Frame frame;
        frame.jobIndex = 0;
        frame.metrics.step = ++frameNumber;
        frame.metrics.label = comment;
        for (const Atom &atom : atoms) frame.coordinates.push_back(atom.position);
        result.data.frames.push_back(frame);
    }
    result.data.molecule.rebuildBonds();
    result.data.status = CalculationStatus::NormalTermination;
    result.success = !result.data.molecule.atoms.isEmpty();
    if (!result.success && result.error.isEmpty()) result.error = QStringLiteral("XYZ 구조를 찾지 못했습니다.");
    return result;
}

} // namespace youngcraft
