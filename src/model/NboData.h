#pragma once

#include <QStringList>
#include <QVector>

namespace youngcraft {
struct NboRow {
    int section = 0;
    QStringList values;
    qint64 sectionOffset = -1;
    bool spinResolved = false;
};
struct NboData {
    QVector<NboRow> charges, orbitals, interactions, wiberg;
    QString excerpt;
};
}
