#pragma once

#include "io/CubeParser.h"
#include "io/GaussianParser.h"
#include "io/StructureReader.h"

#include <QMetaType>

#include <memory>

namespace youngcraft {

struct LoadedFile {
    ParseResult parseResult;
    std::shared_ptr<CubeGrid> cube;
    QString format;
    bool isCube = false;
    std::shared_ptr<CrystalData> crystal;
};

class FileDispatcher {
public:
    static QString formatForPath(const QString &filePath);
    static QString openFilter();
    static LoadedFile load(const QString &filePath, const ParseOptions &options = {});
};

} // namespace youngcraft

Q_DECLARE_METATYPE(youngcraft::LoadedFile)
