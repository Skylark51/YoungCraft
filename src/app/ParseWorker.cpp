#include "app/ParseWorker.h"

namespace youngcraft {

void ParseWorker::run() {
    emit finished(FileDispatcher::load(m_filePath, m_options));
}

} // namespace youngcraft
