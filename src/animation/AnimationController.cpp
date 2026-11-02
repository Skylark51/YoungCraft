#include "animation/AnimationController.h"

#include <QtMath>

#include <cmath>
#include <numbers>

namespace youngcraft {

AnimationController::AnimationController(QObject *parent) : QObject(parent) {
    setFps(m_fps);
    connect(&m_timer, &QTimer::timeout, this, &AnimationController::tick);
}

void AnimationController::setFrames(const QVector<Frame> &frames) {
    m_frames = frames;
    m_kind = Kind::Frames;
    m_index = 0;
}

void AnimationController::setFrequency(const QVector<QVector3D> &base, const FrequencyMode &mode) {
    m_base = base;
    m_frequency = mode;
    m_kind = Kind::Frequency;
    m_phase = 0.0;
    m_index = mode.number;
}

void AnimationController::setAmplitude(double amplitude) { m_amplitude = qBound(0.0, amplitude, 3.0); }

void AnimationController::setCycleTime(double seconds) { m_cycleTime = qBound(0.2, seconds, 20.0); }

void AnimationController::setFps(int fps) {
    m_fps = qBound(5, fps, 120);
    m_timer.setInterval(qMax(5, qRound(1000.0 / m_fps)));
}

QVector<QVector3D> AnimationController::coordinatesAtPhase(const QVector<QVector3D> &base,
                                                            const FrequencyMode &mode,
                                                            double amplitude, double phase) {
    QVector<QVector3D> coordinates = base;
    const int count = qMin(coordinates.size(), mode.displacement.size());
    const float factor = float(qBound(0.0, amplitude, 3.0) * qSin(phase));
    for (int i = 0; i < count; ++i) coordinates[i] += mode.displacement[i] * factor;
    return coordinates;
}

void AnimationController::startFrames() {
    if (m_frames.isEmpty()) return;
    m_kind = Kind::Frames;
    if (!m_timer.isActive()) {
        m_timer.start();
        emit playingChanged(true);
    }
}

void AnimationController::startFrequency() {
    if (m_base.isEmpty() || m_frequency.displacement.isEmpty()) return;
    m_kind = Kind::Frequency;
    if (!m_timer.isActive()) {
        m_timer.start();
        emit playingChanged(true);
    }
}

void AnimationController::stop() {
    if (m_timer.isActive()) {
        m_timer.stop();
        emit playingChanged(false);
    }
}

void AnimationController::tick() {
    if (m_kind == Kind::Frames) {
        if (m_frames.isEmpty()) return;
        m_index = (m_index + 1) % m_frames.size();
        emit coordinatesReady(m_frames[m_index].coordinates);
        emit frameChanged(m_index);
    } else if (m_kind == Kind::Frequency) {
        if (m_base.isEmpty()) return;
        constexpr double fullCycle = 2.0 * std::numbers::pi_v<double>;
        m_phase = std::fmod(m_phase + fullCycle / (m_fps * m_cycleTime), fullCycle);
        emit coordinatesReady(coordinatesAtPhase(m_base, m_frequency, m_amplitude, m_phase));
        emit frameChanged(m_frequency.number);
    }
}

} // namespace youngcraft
