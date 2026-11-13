#include "render/MoleculeView.h"

#include "render/IsosurfaceBuilder.h"

#include <QApplication>
#include <QClipboard>
#include <QLineF>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QSurfaceFormat>
#include <QThread>
#include <QWheelEvent>

#include <cmath>
#include <iterator>

namespace youngcraft {

void MoleculeView::centerOnMolecule() {
    m_target = m_center;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    emit cameraChanged();
}

void MoleculeView::bestView() {
    m_target = m_center;
    float surfacePadding = 0.0f;
    QVector3D minimum = m_center;
    QVector3D maximum = m_center;
    bool haveVisibleAtom = false;
    for (const Atom &atom : m_molecule.atoms) {
        if (!atom.visible) continue;
        surfacePadding = qMax(surfacePadding, atomVisualRadius(atom));
        if (!haveVisibleAtom) { minimum = maximum = atom.position; haveVisibleAtom = true; }
        minimum.setX(qMin(minimum.x(), atom.position.x()));
        minimum.setY(qMin(minimum.y(), atom.position.y()));
        minimum.setZ(qMin(minimum.z(), atom.position.z()));
        maximum.setX(qMax(maximum.x(), atom.position.x()));
        maximum.setY(qMax(maximum.y(), atom.position.y()));
        maximum.setZ(qMax(maximum.z(), atom.position.z()));
    }
    for(const auto &mesh:{m_positiveSurface,m_negativeSurface}) if(mesh) for(const auto &point:mesh->positions) {
        minimum.setX(qMin(minimum.x(),point.x()));minimum.setY(qMin(minimum.y(),point.y()));minimum.setZ(qMin(minimum.z(),point.z()));
        maximum.setX(qMax(maximum.x(),point.x()));maximum.setY(qMax(maximum.y(),point.y()));maximum.setZ(qMax(maximum.z(),point.z()));
    }
    m_target=(minimum+maximum)*.5f;
    const QVector3D halfExtent = (maximum - minimum) * 0.5f;
    const float aspect = height() > 0 ? qMax(0.2f, float(width()) / float(height())) : 1.0f;
    const float screenHalfExtent = qMax(halfExtent.y() + surfacePadding,
                                        (halfExtent.x() + surfacePadding) / aspect);
    const float fovRadians = qDegreesToRadians(qBound(20.0f, m_style.perspectiveFov, 70.0f));
    m_distance = qMax(2.0f, halfExtent.z() + screenHalfExtent * 1.10f / qTan(fovRadians * 0.5f));
    m_orthoScale = qMax(0.75f, screenHalfExtent * 1.12f);
    m_rotation = QQuaternion();
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    emit cameraChanged();
}

void MoleculeView::isometricView() {
    bestView();
    m_style.perspective=false;
    m_rotation=QQuaternion::fromDirection(QVector3D(1,1,1).normalized(),QVector3D(0,1,0)).conjugated();
    const float aspect=height()>0?qMax(.2f,float(width())/height()):1.0f;
    float extent=.5f;
    const auto include=[&](const QVector3D &position,float padding) {
        const auto local=m_rotation.rotatedVector(position-m_target);
        extent=qMax(extent,qMax(qAbs(local.y())+padding,(qAbs(local.x())+padding)/aspect));
    };
    for(const auto &atom:m_molecule.atoms)if(atom.visible&&(m_style.showHydrogen||atom.atomicNumber!=1))include(atom.position,atomVisualRadius(atom));
    for(const auto &mesh:{m_positiveSurface,m_negativeSurface})if(mesh)for(const auto &point:mesh->positions)include(point,0);
    m_orthoScale=qMax(.75f,extent*1.12f);
    update();if(m_previewOverlay)m_previewOverlay->update();emit cameraChanged();
}

void MoleculeView::updateRotationCenter(bool selectedOnly) {
    if (selectedOnly && m_selectedAtom >= 0 && m_selectedAtom < m_molecule.atoms.size()) {
        m_target = m_molecule.atoms[m_selectedAtom].position;
    } else {
        m_target = m_center;
    }
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    emit cameraChanged();
}

CameraState MoleculeView::cameraState() const {
    return {m_target - m_center, m_rotation, m_distance, m_orthoScale,
            m_style.perspective, m_style.perspectiveFov};
}

void MoleculeView::setCameraState(const CameraState &state) {
    m_target = m_center + state.centerOffset;
    m_rotation = state.rotation;
    m_distance = state.distance;
    m_orthoScale = state.orthoScale;
    m_style.perspective = state.perspective;
    m_style.perspectiveFov = state.perspectiveFov;
    update();
    if (m_previewOverlay) m_previewOverlay->update();
    // Applying a shared state must not broadcast it back to the source.
}

void MoleculeView::saveCameraToClipboard() {
    if (!QApplication::clipboard()) return;
    const QString value = QStringLiteral("%1 %2 %3 | %4 %5 %6 %7 | %8 | %9 %10 %11")
                              .arg(m_target.x()).arg(m_target.y()).arg(m_target.z())
                              .arg(m_rotation.scalar()).arg(m_rotation.x()).arg(m_rotation.y()).arg(m_rotation.z())
                              .arg(m_distance).arg(m_orthoScale).arg(m_style.perspective ? 1 : 0);
    QApplication::clipboard()->setText(value);
}

void MoleculeView::restoreCameraFromClipboard() {
    if (!QApplication::clipboard()) return;
    const QStringList fields = QApplication::clipboard()->text().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 12 || !fields.contains(QLatin1Char('|'))) return;
    bool ok = false;
    const QVector3D target(fields[0].toFloat(&ok), fields[1].toFloat(), fields[2].toFloat());
    if (!ok) return;
    m_target = target;
    m_rotation = QQuaternion(fields[4].toFloat(), fields[5].toFloat(), fields[6].toFloat(), fields[7].toFloat());
    m_distance = fields[9].toFloat();
    m_orthoScale = fields[10].toFloat();
    m_style.perspective = fields[11].toInt() != 0;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    emit cameraChanged();
}

QMatrix4x4 MoleculeView::viewProjection() const {
    QMatrix4x4 projection;
    const auto logicalSize = renderLogicalSize();
    const float aspect = logicalSize.height() > 0 ? float(logicalSize.width() / logicalSize.height()) : 1.0f;
    // A narrower requested capture must retain the full interactive horizontal view.
    // Widen its vertical frustum instead of resizing the widget or changing the camera.
    const float displayAspect=height()>0 ? float(width())/height() : 1.0f;
    const float captureFit=m_captureSize.isEmpty() ? 1.0f : qMax(1.0f,displayAspect/qMax(.001f,aspect));
    if (m_style.perspective) {
        const float fov=qRadiansToDegrees(2.0f*std::atan(std::tan(qDegreesToRadians(qBound(20.0f,m_style.perspectiveFov,70.0f))*.5f)*captureFit));
        projection.perspective(fov, aspect, 0.01f,
                               qMax(1000.0f, m_distance + m_radius * 10.0f));
    } else {
        const float scale = m_orthoScale*captureFit;
        projection.ortho(-scale * aspect, scale * aspect, -scale, scale, -1000.0f, 1000.0f);
    }
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -m_distance);
    view.rotate(m_rotation);
    view.translate(-m_target);
    return projection * view;
}

void MoleculeView::recalculateBounds() {
    if (m_molecule.atoms.isEmpty()) {
        m_center = {};
        m_radius = 1.0f;
        return;
    }
    QVector3D minimum = m_molecule.atoms.first().position;
    QVector3D maximum = minimum;
    for (const Atom &atom : m_molecule.atoms) {
        minimum.setX(qMin(minimum.x(), atom.position.x()));
        minimum.setY(qMin(minimum.y(), atom.position.y()));
        minimum.setZ(qMin(minimum.z(), atom.position.z()));
        maximum.setX(qMax(maximum.x(), atom.position.x()));
        maximum.setY(qMax(maximum.y(), atom.position.y()));
        maximum.setZ(qMax(maximum.z(), atom.position.z()));
    }
    m_center = (minimum + maximum) * 0.5f;
    const QVector3D extent = maximum - minimum;
    m_radius = qMax(0.5f, qMax(extent.x(), qMax(extent.y(), extent.z())) * 0.5f);
}

QVector3D MoleculeView::rotationCenter() const { return m_target; }

} // namespace youngcraft
