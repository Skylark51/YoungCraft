#include "app/MainWindow.h"

#include "analysis/AnalysisEngine.h"
#include "app/ParseWorker.h"
#include "animation/AnimationController.h"
#include "export/ImageExporter.h"
#include "render/MoleculeView.h"
#include "ui/AnalysisPanel.h"
#include "ui/StyleEditorDialog.h"

#include <QActionGroup>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QDockWidget>
#include <QDialog>
#include <QPainter>
#include <QScopeGuard>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <functional>

namespace youngcraft {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

void MainWindow::toggleFrameAnimation() {
    if (m_animation->isPlaying()) { m_animation->stop(); return; }
    if (m_data.frames.isEmpty()) { setStatus(QStringLiteral("재생할 구조 프레임이 없습니다")); return; }
    m_animation->setFrames(m_data.frames);
    m_animation->setFrameIndex(m_currentFrame);
    m_view->setFrequencyMode(nullptr);
    m_animation->startFrames();
}

void MainWindow::toggleFrequencyAnimation() {
    if (m_animation->isPlaying()) { m_animation->stop(); return; }
    if (m_currentFrequency < 0 && !m_data.frequencies.isEmpty()) showFrequency(0);
    if (m_currentFrequency < 0 || m_currentFrequency >= m_data.frequencies.size()) { setStatus(QStringLiteral("재생할 진동 모드가 없습니다")); return; }
    const QVector<QVector3D> base = m_data.frames.isEmpty() ? [&] { QVector<QVector3D> values; for (const Atom &a : m_data.molecule.atoms) values.push_back(a.position); return values; }() : m_data.frames.last().coordinates;
    if(m_animation->kind()!=AnimationController::Kind::Frequency)m_animation->setFrequency(base, m_data.frequencies[m_currentFrequency]);
    m_animation->startFrequency();
}

void MainWindow::restoreFrequencyGeometry() {
    if (m_currentFrequency < 0 || m_currentFrequency >= m_data.frequencies.size()) return;
    const QVector<QVector3D> base = m_data.frames.isEmpty() ? [&] { QVector<QVector3D> values; for (const Atom &a : m_data.molecule.atoms) values.push_back(a.position); return values; }() : m_data.frames.last().coordinates;
    m_animation->stop();
    m_animation->setFrequency(base, m_data.frequencies[m_currentFrequency]);
    m_view->setCoordinates(base);
    m_view->setBonds(m_data.molecule.bonds);
    setStatus(QStringLiteral("원래 구조를 표시했습니다"));
}

QVector<QImage> MainWindow::renderFrequencyAnimation(int frequency, const QSize &size,
                                                    int frameCount, bool recount, QString *error) {
    if (frequency < 0 || frequency >= m_data.frequencies.size() || frameCount < 4 || size.isEmpty()) {
        if (error) *error = QStringLiteral("Freq에서 복사할 진동/TS 모드를 먼저 선택하세요.");
        return {};
    }
    const auto &mode = m_data.frequencies[frequency];
    const QVector<QVector3D> base = m_data.frames.isEmpty() ? [&] {
        QVector<QVector3D> values;
        for (const auto &atom : m_data.molecule.atoms) values.push_back(atom.position);
        return values;
    }() : m_data.frames.last().coordinates;
    if (base.isEmpty() || mode.displacement.size() != base.size() || base.size() != m_view->molecule().atoms.size()) {
        if (error) *error = QStringLiteral("이 진동 모드에는 유효한 원자별 변위가 없습니다.");
        return {};
    }
    const bool wasPlaying = m_animation->isPlaying();
    const auto kind = m_animation->kind();
    QVector<QVector3D> original;
    for (const auto &atom : m_view->molecule().atoms) original.push_back(atom.position);
    const auto originalBonds = m_view->molecule().bonds;
    m_animation->stop();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const auto restore = qScopeGuard([&] {
        m_view->setCoordinates(original);
        m_view->setBonds(originalBonds);
        if (wasPlaying) {
            if (kind == AnimationController::Kind::Frames) m_animation->startFrames();
            else if (kind == AnimationController::Kind::Frequency) m_animation->startFrequency();
        }
        QApplication::restoreOverrideCursor();
    });
    QVector<QImage> frames;
    frames.reserve(frameCount);
    for (int i = 0; i < frameCount; ++i) {
        m_view->setCoordinates(AnimationController::coordinatesAtPhase(
            base, mode, m_analysis->frequencyAmplitude(), 2.0 * kPi * i / frameCount));
        if (recount) m_view->rebuildBonds(m_style.recountCutoffScale, m_style.recountHydrogen, m_style.recountMetalCoordination);
        // Render synchronously so a tab switch or late parser callback cannot change the document mid-copy.
        auto frame = m_view->imageAtSize(size, true, true);
        if (frame.isNull()) {
            if (error) *error = QStringLiteral("선택한 해상도로 프레임을 렌더링할 수 없습니다.");
            return {};
        }
        frames.push_back(frame);
    }
    return ImageExporter::cropAnimation(frames);
}

void MainWindow::copyAnimation() {
    int frequency = m_currentFrequency;
    if (frequency < 0 || frequency >= m_data.frequencies.size()) {
        // A TS file can be copied directly without having to select its imaginary mode first.
        frequency = -1;
        for (int i = 0; i < m_data.frequencies.size(); ++i) {
            if (m_data.frequencies[i].imaginary) { frequency = i; break; }
        }
    }
    const double cycle = m_analysis->frequencyCycleTime();
    const int count = qBound(4, qRound(cycle * 20), 120);
    const int delay = qMax(10, qRound(cycle * 1000.0 / count / 10.0) * 10);
    QString error;
    const auto frames = renderFrequencyAnimation(frequency, m_view->size().scaled(768, 768, Qt::KeepAspectRatio),
                                                 count, m_analysis->recountBondsEnabled(), &error);
    if (frames.isEmpty() || !ImageExporter::copyAnimationToClipboard(frames, delay, &error)) {
        setStatus(error, 8000);
        return;
    }
    setStatus(QStringLiteral("TS/진동 애니메이션을 투명 GIF로 복사했습니다 (%1프레임, %2 × %3 px)")
        .arg(frames.size()).arg(frames.first().width()).arg(frames.first().height()), 8000);
}

void MainWindow::exportAnimation() {
    if (m_currentFrequency < 0 || m_currentFrequency >= m_data.frequencies.size()) {
        setStatus(QStringLiteral("저장할 진동 모드를 먼저 선택하세요"));
        return;
    }

    QDialog options(this); options.setWindowTitle(QStringLiteral("애니메이션 저장"));
    auto *form = new QFormLayout(&options);
    QComboBox format;
    format.addItems({QStringLiteral("GIF 애니메이션"), QStringLiteral("PNG 프레임 여러 장"), QStringLiteral("JPG 프레임 여러 장 (흰 배경)")});
    QCheckBox proportional(QStringLiteral("비율 유지")); proportional.setChecked(true);
    QSpinBox width; width.setRange(64, 8192); width.setValue(m_view->width());
    QSpinBox height; height.setRange(64, 8192); height.setValue(m_view->height());
    QSpinBox interval; interval.setRange(10, 5000); interval.setValue(50); interval.setSuffix(QStringLiteral(" ms"));
    QSpinBox frameCount; frameCount.setRange(4, 240); frameCount.setValue(20);
    QCheckBox recount(QStringLiteral("결합 다시 계산")); recount.setChecked(m_analysis->recountBondsEnabled());
    form->addRow(QStringLiteral("저장 형식"), &format);
    form->addRow(&proportional);
    form->addRow(QStringLiteral("너비"), &width); form->addRow(QStringLiteral("높이"), &height);
    form->addRow(QStringLiteral("프레임 수"), &frameCount);
    form->addRow(QStringLiteral("프레임 간격"), &interval);
    form->addRow(&recount);
    const double aspect = m_view->height() > 0 ? double(m_view->width()) / m_view->height() : 1.0;
    bool updatingSize = false;
    connect(&width, qOverload<int>(&QSpinBox::valueChanged), &options, [&](int value) {
        if (!proportional.isChecked() || updatingSize) return;
        updatingSize = true; height.setValue(qMax(64, qRound(value / aspect))); updatingSize = false;
    });
    connect(&height, qOverload<int>(&QSpinBox::valueChanged), &options, [&](int value) {
        if (!proportional.isChecked() || updatingSize) return;
        updatingSize = true; width.setValue(qMax(64, qRound(value * aspect))); updatingSize = false;
    });
    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Save)->setText(QStringLiteral("저장"));
    buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    form->addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &options, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &options, &QDialog::reject);
    if (options.exec() != QDialog::Accepted) return;

    QString path;
    if (format.currentIndex() == 0)
        path = QFileDialog::getSaveFileName(this, QStringLiteral("GIF 애니메이션 저장"), {}, QStringLiteral("GIF 애니메이션 (*.gif)"));
    else
        path = QFileDialog::getExistingDirectory(this, QStringLiteral("프레임 저장 폴더 선택"));
    if (path.isEmpty()) return;
    if (format.currentIndex() == 0 && QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".gif");

    QString error;
    const auto images = renderFrequencyAnimation(m_currentFrequency, QSize(width.value(), height.value()),
                                                 frameCount.value(), recount.isChecked(), &error);
    if (images.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("애니메이션 저장 실패"), error);
        return;
    }
    bool ok = true;
    if (format.currentIndex() == 0) {
        ok = ImageExporter::saveGif(images, path, interval.value(), true, true, &error);
    } else {
        const QString extension = format.currentIndex() == 1 ? QStringLiteral("png") : QStringLiteral("jpg");
        QDir directory(path);
        for (int i = 0; i < images.size(); ++i) {
            const QString framePath = directory.filePath(QStringLiteral("frame_%1.%2").arg(i + 1, 3, 10, QLatin1Char('0')).arg(extension));
            QImage output = images[i];
            if (format.currentIndex() == 2) {
                // JPEG has no alpha channel; use an explicit white background.
                output = QImage(images[i].size(), QImage::Format_RGB32);
                output.fill(Qt::white);
                QPainter painter(&output);
                painter.drawImage(0, 0, images[i]);
            }
            if (!output.save(framePath)) {
                ok = false;
                error = QStringLiteral("프레임을 저장할 수 없습니다: %1").arg(framePath);
                break;
            }
        }
    }
    if (ok) setStatus(QStringLiteral("애니메이션 저장 완료: %1").arg(path));
    else QMessageBox::warning(this, QStringLiteral("애니메이션 저장 실패"), error);
}

} // namespace youngcraft
