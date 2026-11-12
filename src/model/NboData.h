#pragma once

#include <QStringList>
#include <QVector>

namespace youngcraft {
struct NboRow { int section = 0; QStringList values; };
struct NboData {
    QVector<NboRow> charges, orbitals, interactions, wiberg;
    QString excerpt;
};
}
