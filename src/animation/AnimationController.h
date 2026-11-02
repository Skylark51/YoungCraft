#pragma once

#include "model/Molecule.h"

#include <QObject>
#include <QTimer>

namespace youngcraft {

class AnimationController final : public QObject {
    Q_OBJECT
public:
    enum class Kind { None, Frames, Frequency };
    explicit AnimationController(QObject *parent = nullptr);

    void setFrames(const QVector<Frame> &frames);
    void setFrequency(const QVector<QVector3D> &base, const FrequencyMode &mode);
    void setAmplitude(double amplitude);
    void setCycleTime(double seconds);
    void setFps(int fps);
    void startFrames();
    void startFrequency();
    void stop();
    bool isPlaying() const { return m_timer.isActive(); }
    Kind kind() const { return m_kind; }
    int currentIndex() const { return m_index; }
    void setFrameIndex(int index) { if (index >= 0 && index < m_frames.size()) m_index = index; }
    double amplitude() const { return m_amplitude; }
    double cycleTime() const { return m_cycleTime; }
    int fps() const { return m_fps; }
    static QVector<QVector3D> coordinatesAtPhase(const QVector<QVector3D> &base,
                                                 const FrequencyMode &mode,
                                                 double amplitude, double phase);

signals:
    void coordinatesReady(const QVector<QVector3D> &coordinates);
    void frameChanged(int index);
    void playingChanged(bool playing);

private slots:
    void tick();

private:
    QTimer m_timer;
    Kind m_kind = Kind::None;
    QVector<Frame> m_frames;
    QVector<QVector3D> m_base;
    FrequencyMode m_frequency;
    double m_amplitude = 0.5;
    double m_cycleTime = 2.0;
    double m_phase = 0.0;
    int m_index = 0;
    int m_fps = 30;
};

} // namespace youngcraft
