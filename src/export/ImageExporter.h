#pragma once

#include <QImage>
#include <QString>
#include <QVector>
#include <functional>

namespace youngcraft {

class ImageExporter {
public:
    static QImage supersample(const QImage &image, int factor, bool transparent = false);
    static bool saveImage(const QImage &image, const QString &path, int supersampleFactor = 1,
                          bool transparent = false, QString *error = nullptr);
    static QRect contentBounds(const QImage &image);
    static QImage cropToContent(const QImage &image, int padding = 0);
    static QVector<QImage> cropAnimation(const QVector<QImage> &frames);
    static void copyToClipboard(const QImage &image);
    static bool saveGif(const QVector<QImage> &frames, const QString &path, int delayMs = 50,
                        bool transparent = false, bool loop = true, QString *error = nullptr);
    // One frame at a time, allowing long mechanism movies without retaining bitmaps.
    static bool saveGifFrames(int count, const QSize &size, const QString &path,
                             const std::function<QImage(int)> &frameAt, int delayMs = 50,
                             bool transparent = false, bool loop = true, QString *error = nullptr,
                             bool ditherTransparency = false);
    static bool copyAnimationToClipboard(const QVector<QImage> &frames, int delayMs, QString *error = nullptr);
    static bool ffmpegAvailable();
    static bool saveMp4(const QVector<QImage> &frames, const QString &path, int fps = 20,
                        QString *error = nullptr);
};

} // namespace youngcraft
