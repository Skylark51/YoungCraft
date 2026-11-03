#include "compute/GaussianTools.h"
#include "io/FileDispatcher.h"

#include "io/FchkParser.h"
#include "io/OrcaParser.h"
#include "io/XYZParser.h"

#include <QFileInfo>

namespace youngcraft {

QString FileDispatcher::openFilter() {
    return QStringLiteral(
        "지원되는 모든 구조/계산 파일 (*.out *.log *.gjf *.com *.xyz *.cube *.cub *.fchk *.fch *.chk *.cif *.mmcif *.cmf *.mol *.sdf *.sd *.mdl *.mol2 *.pdb *.ent *.res *.ins *.cssr);;"
        "Gaussian / ORCA 출력 (*.out *.log);;Gaussian 입력 (*.gjf *.com);;XYZ 파일 (*.xyz);;CIF / 결정 구조 (*.cif *.mmcif *.cmf);;"
        "MOL / SDF (*.mol *.sdf *.sd *.mdl);;MOL2 (*.mol2);;PDB (*.pdb *.ent);;"
        "SHELX (*.res *.ins);;CSSR (*.cssr);;CUBE (*.cube *.cub);;FCHK (*.fchk *.fch *.chk);;모든 파일 (*.*)");
}

QString FileDispatcher::formatForPath(const QString &filePath) {
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("out") || suffix == QStringLiteral("log") ||
        suffix == QStringLiteral("gjf") || suffix == QStringLiteral("com")) return QStringLiteral("Gaussian");
    if (suffix == QStringLiteral("fchk") || suffix == QStringLiteral("fch")) return QStringLiteral("FCHK");
    if (suffix == QStringLiteral("chk")) return QStringLiteral("CHK");
    if (suffix == QStringLiteral("cube") || suffix == QStringLiteral("cub")) return QStringLiteral("CUBE");
    if (suffix == QStringLiteral("xyz")) return QStringLiteral("XYZ");
    if (suffix == QStringLiteral("cif") || suffix == QStringLiteral("mmcif") || suffix == QStringLiteral("cmf")) return QStringLiteral("CIF");
    if (suffix == QStringLiteral("mol") || suffix == QStringLiteral("mdl")) return QStringLiteral("MOL");
    if (suffix == QStringLiteral("sdf") || suffix == QStringLiteral("sd")) return QStringLiteral("SDF");
    if (suffix == QStringLiteral("mol2")) return QStringLiteral("MOL2");
    if (suffix == QStringLiteral("pdb") || suffix == QStringLiteral("ent")) return QStringLiteral("PDB");
    if (suffix == QStringLiteral("res") || suffix == QStringLiteral("ins")) return QStringLiteral("SHELX");
    if (suffix == QStringLiteral("cssr")) return QStringLiteral("CSSR");
    return suffix.toUpper();
}

LoadedFile FileDispatcher::load(const QString &filePath, const ParseOptions &options) {
    LoadedFile loaded;
    loaded.format = formatForPath(filePath);
    if (loaded.format == QStringLiteral("CUBE")) {
        loaded.cube = std::make_shared<CubeGrid>(CubeParser::parse(filePath));
        loaded.isCube = true;
        loaded.parseResult.data.filePath = filePath;
        loaded.parseResult.data.format = loaded.format;
        loaded.parseResult.data.molecule = loaded.cube->molecule;
        loaded.parseResult.data.hasCubeGrid = loaded.cube->valid;
        loaded.parseResult.success = loaded.cube->valid;
        loaded.parseResult.error = loaded.cube->error;
    } else if (loaded.format == QStringLiteral("XYZ")) {
        loaded.parseResult = XYZParser::parse(filePath);
    } else if (loaded.format == QStringLiteral("CHK")) {
        QString error;
        const QString formatted = GaussianTools::convertCheckpoint(filePath, &error);
        if (formatted.isEmpty()) loaded.parseResult.error = error;
        else loaded.parseResult = FchkParser::parse(formatted);
    } else if (loaded.format == QStringLiteral("FCHK")) {
        loaded.parseResult = FchkParser::parse(filePath);
    } else if (loaded.format == QStringLiteral("CIF") || loaded.format == QStringLiteral("MOL") ||
               loaded.format == QStringLiteral("SDF") || loaded.format == QStringLiteral("MOL2") ||
               loaded.format == QStringLiteral("PDB") || loaded.format == QStringLiteral("SHELX") ||
               loaded.format == QStringLiteral("CSSR")) {
        StructureLoad structure = StructureReader::read(filePath, loaded.format);
        loaded.parseResult = std::move(structure.result);
        loaded.crystal = std::move(structure.crystal);
    } else if (loaded.format == QStringLiteral("Gaussian") &&
               (QFileInfo(filePath).suffix().compare(QStringLiteral("out"), Qt::CaseInsensitive) == 0 ||
                QFileInfo(filePath).suffix().compare(QStringLiteral("log"), Qt::CaseInsensitive) == 0) &&
               OrcaParser::isOrcaOutput(filePath)) {
        loaded.format = QStringLiteral("ORCA");
        loaded.parseResult = OrcaParser::parse(filePath, options);
    } else {
        loaded.parseResult = GaussianParser::parse(filePath, options);
    }
    return loaded;
}

} // namespace youngcraft
