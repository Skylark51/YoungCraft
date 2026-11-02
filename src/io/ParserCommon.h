#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <cmath>

namespace youngcraft::parser {

inline double toDouble(QString value, bool *ok = nullptr) {
    value = value.trimmed();
    value.replace('D', 'E', Qt::CaseInsensitive);
    double parsed = value.toDouble(ok);
    if (ok && !*ok) {
        *ok = false;
    }
    return parsed;
}

inline QStringList columns(const QString &line) {
    return line.simplified().split(' ', Qt::SkipEmptyParts);
}

inline bool nearlyValid(double value) {
    return std::isfinite(value);
}

inline bool parseLeadingInt(const QString &line, int &value) {
    static const QRegularExpression re(QStringLiteral(R"(^\s*(\d+)\b)"));
    const auto match = re.match(line);
    if (!match.hasMatch()) {
        return false;
    }
    bool ok = false;
    value = match.captured(1).toInt(&ok);
    return ok;
}

inline bool containsAny(const QString &line, std::initializer_list<QLatin1String> needles) {
    for (const auto needle : needles) {
        if (line.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

} // namespace youngcraft::parser
