#pragma once

#include <QColor>
#include <QString>
#include <cmath>

namespace youngcraft {

inline QString styleColorButtonSheet(const QColor &background) {
    const auto linear = [](double channel) {
        return channel <= .04045 ? channel / 12.92 : std::pow((channel + .055) / 1.055, 2.4);
    };
    const double luminance = .2126 * linear(background.redF()) + .7152 * linear(background.greenF())
        + .0722 * linear(background.blueF());
    const QString text = luminance < .179 ? QStringLiteral("#ffffff") : QStringLiteral("#000000");
    return QStringLiteral("background-color: %1; color: %2;").arg(background.name(), text);
}

} // namespace youngcraft
