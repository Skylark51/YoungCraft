#pragma once

#include <QImage>
#include <QString>
#include <functional>

namespace youngcraft {
bool writePowerPointVideo(const QString &path, QSize size, int fps, int frames,
                          const std::function<QImage(int)> &render, QString &error);
}
