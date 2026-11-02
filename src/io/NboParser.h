#pragma once
#include <QString>
#include <QVector>
namespace youngcraft {
struct NboRow { int section = 0; QStringList values; };
struct NboData {
    QVector<NboRow> charges, orbitals, interactions, wiberg;
    QString excerpt;
};
class NboParser {
public:
    static NboData parse(const QString &path);
};
}
