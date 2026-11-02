#pragma once

#include "io/FileDispatcher.h"

#include <QObject>

namespace youngcraft {

class ParseWorker final : public QObject {
    Q_OBJECT
public:
    ParseWorker(QString filePath, ParseOptions options, QObject *parent = nullptr)
        : QObject(parent), m_filePath(std::move(filePath)), m_options(options) {}

public slots:
    void run();

signals:
    void finished(const youngcraft::LoadedFile &loaded);

private:
    QString m_filePath;
    ParseOptions m_options;
};

} // namespace youngcraft
