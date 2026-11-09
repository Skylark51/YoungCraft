#include "export/ImageExporter.h"
#include <QtMath>

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QColorSpace>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QHash>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>
#include <QUrl>

namespace youngcraft {

namespace {

void appendWord(QByteArray &bytes, int value) {
    bytes.append(char(value & 0xff));
    bytes.append(char((value >> 8) & 0xff));
}

QVector<QRgb> gifPalette() {
    QVector<QRgb> palette;
    palette.reserve(256);
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                palette.push_back(qRgb(r * 255 / 5, g * 255 / 5, b * 255 / 5));
            }
        }
    }
    for (int i = 0; i < 40; ++i) {
        const int gray = i * 255 / 39;
        palette.push_back(qRgb(gray, gray, gray));
    }
    return palette;
}

QByteArray gifLzw(const QVector<quint8> &pixels) {
    constexpr int clearCode = 256;
    constexpr int endCode = 257;
    int codeSize = 9;
    int nextCode = 258;
    quint32 bitBuffer = 0;
    int bitCount = 0;
    QByteArray bytes;

    auto writeCode = [&](int code) {
        bitBuffer |= quint32(code) << bitCount;
        bitCount += codeSize;
        while (bitCount >= 8) {
            bytes.append(char(bitBuffer & 0xff));
            bitBuffer >>= 8;
            bitCount -= 8;
        }
    };

    writeCode(clearCode);
    if (pixels.isEmpty()) {
        writeCode(endCode);
    } else {
        QHash<quint32, int> dictionary;
        int prefix = pixels.first();
        for (int i = 1; i < pixels.size(); ++i) {
            const int symbol = pixels[i];
            const quint32 key = (quint32(prefix) << 8) | quint32(symbol);
            const auto found = dictionary.constFind(key);
            if (found != dictionary.cend()) {
                prefix = found.value();
                continue;
            }

            writeCode(prefix);
            if (nextCode < 4096) {
                dictionary.insert(key, nextCode++);
                // The decoder adds a dictionary entry one emitted code later.
                if (nextCode > (1 << codeSize) && codeSize < 12) ++codeSize;
            } else {
                writeCode(clearCode);
                dictionary.clear();
                codeSize = 9;
                nextCode = 258;
            }
            prefix = symbol;
        }
        writeCode(prefix);
        // Reading the final prefix catches the decoder up before its end code.
        if (nextCode == (1 << codeSize) && codeSize < 12) ++codeSize;
        writeCode(endCode);
    }
    if (bitCount > 0) bytes.append(char(bitBuffer & 0xff));
    return bytes;
}

QVector<quint8> quantizeGifFrame(const QImage &source, const QSize &size, bool transparent,
                                bool ditherTransparency) {
    // GIF stores one transparent index. Distribute coverage across neighboring pixels.
    static constexpr int bayer[8][8] = {
        { 0,48,12,60, 3,51,15,63}, {32,16,44,28,35,19,47,31},
        { 8,56, 4,52,11,59, 7,55}, {40,24,36,20,43,27,39,23},
        { 2,50,14,62, 1,49,13,61}, {34,18,46,30,33,17,45,29},
        {10,58, 6,54, 9,57, 5,53}, {42,26,38,22,41,25,37,21}
    };
    const bool dither = transparent && ditherTransparency;
    const QImage image = source.size() == size
                             ? source.convertToFormat(QImage::Format_ARGB32)
                             : source.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                   .convertToFormat(QImage::Format_ARGB32);
    QVector<quint8> pixels;
    pixels.resize(image.width() * image.height());
    for (int y = 0; y < image.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            const int threshold = dither ? bayer[y % 8][x % 8] * 4 + 2 : 128;
            if (transparent && alpha < threshold) {
                pixels[y * image.width() + x] = 255;
                continue;
            }
            const int red = dither ? qRed(pixel) : (qRed(pixel) * alpha + 255 * (255 - alpha)) / 255;
            const int green = dither ? qGreen(pixel) : (qGreen(pixel) * alpha + 255 * (255 - alpha)) / 255;
            const int blue = dither ? qBlue(pixel) : (qBlue(pixel) * alpha + 255 * (255 - alpha)) / 255;
            const int r = (red * 5 + 127) / 255;
            const int g = (green * 5 + 127) / 255;
            const int b = (blue * 5 + 127) / 255;
            pixels[y * image.width() + x] = quint8(r * 36 + g * 6 + b);
        }
    }
    return pixels;
}

} // namespace

QImage ImageExporter::supersample(const QImage &image, int factor, bool transparent) {
    // Legacy bitmap resizing only. Fresh high-detail exports must be rendered
    // with MoleculeView::imageAtSize before reaching the encoder.
    if (image.isNull()) return image;
    QImage result = image;
    if (factor > 1) result = image.scaled(image.size() * factor, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (transparent) {
        QImage alpha(result.size(), QImage::Format_ARGB32_Premultiplied);
        alpha.fill(Qt::transparent);
        alpha.setDotsPerMeterX(result.dotsPerMeterX());
        alpha.setDotsPerMeterY(result.dotsPerMeterY());
        alpha.setColorSpace(result.colorSpace());
        QPainter painter(&alpha);
        painter.drawImage(0, 0, result);
        painter.end();
        return alpha;
    }
    return result;
}

bool ImageExporter::saveImage(const QImage &image, const QString &path, int supersampleFactor,
                              bool transparent, QString *error) {
    const QImage output = supersample(image, supersampleFactor, transparent);
    if (output.save(path)) return true;
    if (error) *error = QStringLiteral("이미지를 저장할 수 없습니다: %1").arg(path);
    return false;
}

QRect ImageExporter::contentBounds(const QImage &source) {
    if (source.isNull()) return {};
    if (!source.hasAlphaChannel()) return source.rect();
    const QImage image = source.convertToFormat(QImage::Format_ARGB32);
    int left = image.width(), right = -1, top = image.height(), bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        const auto *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            // Keep even faint antialiased edges and translucent labels.
            if (qAlpha(line[x]) == 0) continue;
            left = qMin(left, x); right = qMax(right, x);
            top = qMin(top, y); bottom = y;
        }
    }
    return right < left ? QRect() : QRect(QPoint(left, top), QPoint(right, bottom));
}

QImage ImageExporter::cropToContent(const QImage &image, int padding) {
    const QRect bounds = contentBounds(image);
    if (bounds.isEmpty()) return image;
    padding = qMax(0, padding);
    // QImage::copy fills areas beyond the source with transparent pixels.
    return image.copy(bounds.adjusted(-padding, -padding, padding, padding));
}

QVector<QImage> ImageExporter::cropAnimation(const QVector<QImage> &frames) {
    QRect bounds;
    for (const QImage &frame : frames) bounds = bounds.united(contentBounds(frame));
    if (bounds.isEmpty()) return frames;
    const int padding = qMax(2, qCeil(qMax(bounds.width(), bounds.height()) * 0.02));
    bounds.adjust(-padding, -padding, padding, padding);
    QVector<QImage> cropped;
    cropped.reserve(frames.size());
    // One shared rectangle preserves alignment and covers the entire motion.
    for (const QImage &frame : frames) cropped.push_back(frame.copy(bounds));
    return cropped;
}

void ImageExporter::copyToClipboard(const QImage &image) {
    QClipboard *clipboard = QApplication::clipboard();
    if (!clipboard || image.isNull()) return;
    auto *mime = new QMimeData;
#ifndef Q_OS_WIN
    mime->setImageData(image);
#endif
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (image.save(&buffer, "PNG")) {
#ifdef Q_OS_WIN
        // Word requests the registered Windows "PNG" format, not "image/png".
        // Exclude bitmap formats so Word cannot select a copy without alpha.
        mime->setData(QStringLiteral("application/x-qt-windows-mime;value=\"PNG\""), png);
#endif
        mime->setData(QStringLiteral("image/png"), png);
    }
    clipboard->setMimeData(mime);
}

bool ImageExporter::copyAnimationToClipboard(const QVector<QImage> &frames, int delayMs, QString *error) {
    if (frames.isEmpty() || !QApplication::clipboard()) {
        if (error) *error = QStringLiteral("복사할 애니메이션 프레임이 없습니다.");
        return false;
    }
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/clipboard-animations");
    if (!QDir().mkpath(directory)) {
        if (error) *error = QStringLiteral("애니메이션 복사 폴더를 만들 수 없습니다.");
        return false;
    }
    // Keep the backing file after shutdown: file-based paste may happen later.
    const QString path = directory + QStringLiteral("/Youngcraft-TS-%1.gif")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!saveGif(frames, path, delayMs, true, true, error)) {
        QFile::remove(path);
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        QFile::remove(path);
        return false;
    }
    const QByteArray gif = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (error) *error = file.errorString();
        file.close(); QFile::remove(path);
        return false;
    }
    file.close();
    auto *mime = new QMimeData;
    mime->setData(QStringLiteral("image/gif"), gif);
#ifdef Q_OS_WIN
    mime->setData(QStringLiteral("application/x-qt-windows-mime;value=\"GIF\""), gif);
#endif
    mime->setUrls({QUrl::fromLocalFile(path)});
    // Omit a still-image fallback so animated data is never silently flattened.
    QApplication::clipboard()->setMimeData(mime);
    return true;
}

bool ImageExporter::saveGif(const QVector<QImage> &frames, const QString &path, int delayMs,
                            bool transparent, bool loop, QString *error) {
    return saveGifFrames(frames.size(),frames.isEmpty()?QSize():frames.first().size(),path,
        [&](int index){return frames[index];},delayMs,transparent,loop,error);
}

bool ImageExporter::saveGifFrames(int count, const QSize &size, const QString &path,
                                  const std::function<QImage(int)> &frameAt, int delayMs,
                                  bool transparent, bool loop, QString *error, bool ditherTransparency) {
    if (count<=0) {
        if (error) *error = QStringLiteral("내보낼 애니메이션 프레임이 없습니다.");
        return false;
    }
    if (size.isEmpty() || size.width() > 65535 || size.height() > 65535) {
        if (error) *error = QStringLiteral("GIF 크기가 지원 범위를 벗어났습니다.");
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = QStringLiteral("GIF 파일을 열 수 없습니다: %1").arg(file.errorString());
        return false;
    }

    QByteArray header("GIF89a");
    appendWord(header, size.width());
    appendWord(header, size.height());
    header.append(char(0xf7)); // global table, 256 colors
    header.append(char(transparent ? 255 : 0)); // transparent disposal background
    header.append(char(0));
    if (file.write(header) != header.size()) {
        if (error) *error = file.errorString();
        return false;
    }
    const QVector<QRgb> palette = gifPalette();
    QByteArray paletteBytes;
    paletteBytes.reserve(palette.size() * 3);
    for (const QRgb color : palette) {
        paletteBytes.append(char(qRed(color)));
        paletteBytes.append(char(qGreen(color)));
        paletteBytes.append(char(qBlue(color)));
    }
    if (file.write(paletteBytes) != paletteBytes.size()) {
        if (error) *error = file.errorString();
        return false;
    }

    if (loop) {
        const QByteArray loopExtension("\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00", 19);
        if (file.write(loopExtension) != loopExtension.size()) {
            if (error) *error = file.errorString();
            return false;
        }
    }

    const int centiseconds = qBound(1, (delayMs + 5) / 10, 65535);
    for (int frameIndex=0;frameIndex<count;++frameIndex) {
        const QImage frame=frameAt(frameIndex);
        if(frame.isNull()) {if(error)*error=QStringLiteral("GIF 프레임 렌더링 실패 또는 저장 취소");return false;}
        QByteArray graphicsControl("\x21\xf9\x04", 3);
        graphicsControl.append(char(transparent ? 0x09 : 0x04));
        appendWord(graphicsControl, centiseconds);
        graphicsControl.append(char(transparent ? 255 : 0));
        graphicsControl.append(char(0));
        if (file.write(graphicsControl) != graphicsControl.size()) {
            if (error) *error = file.errorString();
            return false;
        }

        QByteArray descriptor("\x2c", 1);
        appendWord(descriptor, 0); appendWord(descriptor, 0);
        appendWord(descriptor, size.width()); appendWord(descriptor, size.height());
        descriptor.append(char(0));
        if (file.write(descriptor) != descriptor.size()) {
            if (error) *error = file.errorString();
            return false;
        }
        const QVector<quint8> pixels = quantizeGifFrame(frame, size, transparent, ditherTransparency);
        const QByteArray compressed = gifLzw(pixels);
        if (!file.putChar(char(8))) { // minimum LZW code size
            if (error) *error = file.errorString();
            return false;
        }
        for (int offset = 0; offset < compressed.size();) {
            const int count = qMin(255, compressed.size() - offset);
            if (!file.putChar(char(count)) || file.write(compressed.constData() + offset, count) != count) {
                if (error) *error = file.errorString();
                return false;
            }
            offset += count;
        }
        if (!file.putChar(char(0))) {
            if (error) *error = file.errorString();
            return false;
        }
    }
    if (!file.putChar(char(0x3b))) {
        if (error) *error = file.errorString();
        return false;
    }
    if(!file.commit()){if(error)*error=file.errorString();return false;}
    return true;
}

bool ImageExporter::ffmpegAvailable() {
    return !QStandardPaths::findExecutable(QStringLiteral("ffmpeg")).isEmpty();
}

bool ImageExporter::saveMp4(const QVector<QImage> &frames, const QString &path, int fps, QString *error) {
    if (!ffmpegAvailable()) {
        if (error) *error = QStringLiteral("ffmpeg를 찾지 못했습니다. ffmpeg 설치 후 다시 시도하세요.");
        return false;
    }
    QTemporaryDir temp;
    if (!temp.isValid()) {
        if (error) *error = QStringLiteral("임시 폴더를 만들 수 없습니다.");
        return false;
    }
    for (int i = 0; i < frames.size(); ++i) {
        const QString framePath = temp.path() + QStringLiteral("/frame_%1.png").arg(i, 5, 10, QLatin1Char('0'));
        if (!frames[i].save(framePath)) {
            if (error) *error = QStringLiteral("애니메이션 임시 프레임을 저장할 수 없습니다.");
            return false;
        }
    }
    QProcess process;
    QStringList args{QStringLiteral("-y"), QStringLiteral("-framerate"), QString::number(fps),
                     QStringLiteral("-i"), temp.path() + QStringLiteral("/frame_%05d.png"),
                     QStringLiteral("-pix_fmt"), QStringLiteral("yuv420p"), path};
    process.start(QStringLiteral("ffmpeg"), args);
    if (!process.waitForFinished(120000) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) *error = QString::fromLocal8Bit(process.readAllStandardError());
        return false;
    }
    return true;
}

} // namespace youngcraft
