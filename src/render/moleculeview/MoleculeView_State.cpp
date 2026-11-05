#include "render/MoleculeView.h"

#include "render/IsosurfaceBuilder.h"

#include <QApplication>
#include <QClipboard>
#include <QLineF>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QOpenGLPaintDevice>
#include <QColorSpace>
#include <QPointer>
#include <QSurfaceFormat>
#include <QSignalBlocker>
#include <QThread>
#include <QWheelEvent>

#include <cmath>
#include <iterator>

namespace youngcraft {

void MoleculeView::setMolecule(const Molecule &molecule) {
    endDrawingChain();
    m_molecule = molecule;
    recalculateBounds();
    { const QSignalBlocker blocker(this); bestView(); }
    emit geometryChanged();
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setAtomVisibility(const QVector<bool> &visibility) {
    if (visibility.size() != m_molecule.atoms.size()) return;
    for (int i = 0; i < visibility.size(); ++i) m_molecule.atoms[i].visible = visibility[i];
    // Visibility changes must not refit the camera or replace live frame coordinates.
    update();
    if (m_previewOverlay) m_previewOverlay->update();
}

void MoleculeView::setAnimationMolecule(const Molecule &molecule) {
    m_molecule = molecule;
    update();
    if (m_previewOverlay) m_previewOverlay->update();
}

void MoleculeView::setAtomDisplay(const Molecule &appearance) {
    if (appearance.atoms.size() != m_molecule.atoms.size()) return;
    for (int i = 0; i < appearance.atoms.size(); ++i) {
        m_molecule.atoms[i].visible = appearance.atoms[i].visible;
        m_molecule.atoms[i].displayOpacity = qBound(0.0f, appearance.atoms[i].displayOpacity, 1.0f);
    }
    update();
    if (m_previewOverlay) m_previewOverlay->update();
}

void MoleculeView::setEditedMolecule(const Molecule &molecule) {
    endDrawingChain();
    const bool wasEmpty = m_molecule.atoms.isEmpty();
    m_molecule = molecule;
    recalculateBounds();
    if (wasEmpty) bestView();
    emit geometryChanged();
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setDrawingAnchorAt(const QVector3D &position) {
    m_chainAtom=-1;
    if(m_chainDrawing&&m_drawingMode&&!m_insertPreview)
        for(int i=0;i<m_molecule.atoms.size();++i)
            if(!m_molecule.atoms[i].autoHydrogen&&(m_molecule.atoms[i].position-position).lengthSquared()<1e-8f){m_chainAtom=i;break;}
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setFrequencyMode(const FrequencyMode *mode) {
    m_frequencyDisplacements = mode ? mode->displacement : QVector<QVector3D>{};
    m_frequencyBase.clear();
    if (mode) for (const Atom &atom : m_molecule.atoms) m_frequencyBase.push_back(atom.position);
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setCoordinates(const QVector<QVector3D> &coordinates) {
    if (coordinates.size() != m_molecule.atoms.size()) return;
    for (int i = 0; i < coordinates.size(); ++i) m_molecule.atoms[i].position = coordinates[i];
    recalculateBounds();
    emit geometryChanged();
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setStyle(const RenderStyle &style) {
    const bool cameraChangedByStyle = m_style.perspective != style.perspective ||
                                      m_style.perspectiveFov != style.perspectiveFov;
    const bool isoChanged = m_style.isoValue != style.isoValue;
    m_style = style;
    setAutoFillBackground(false);
    if (isoChanged && m_cubeGrid && m_cubeGrid->valid) rebuildIsosurfaces();
    if (cameraChangedByStyle) emit cameraChanged();
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setCrystalCell(const QVector3D &a, const QVector3D &b, const QVector3D &c,
                                  int repeat, bool visible) {
    m_cellA = a; m_cellB = b; m_cellC = c;
    m_cellRepeat = qBound(1, repeat, 2);
    m_cellVisible = visible;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

QSize MoleculeView::renderPixelSize() const {
    return m_captureSize.isEmpty()
        ? QSize(qMax(1, qRound(width() * devicePixelRatioF())),
                qMax(1, qRound(height() * devicePixelRatioF())))
        : m_captureSize;
}

QSizeF MoleculeView::renderLogicalSize() const {
    if (m_captureSize.isEmpty()) return QSizeF(size());
    const qreal logicalHeight = qMax(1, height());
    return QSizeF(logicalHeight * m_captureSize.width() / m_captureSize.height(), logicalHeight);
}

GLuint MoleculeView::renderTargetFramebuffer() const {
    return m_captureSize.isEmpty() ? defaultFramebufferObject() : m_captureFramebuffer;
}

void MoleculeView::beginOverlayPainter(QPainter &painter) {
    painter.begin(m_capturePaintDevice ? m_capturePaintDevice : this);
    if (!m_captureSize.isEmpty()) {
        const qreal scale = m_captureSize.height() / renderLogicalSize().height();
        painter.scale(scale, scale);
    }
}

QImage MoleculeView::imageAtSize(const QSize &pixels, bool transparent, bool viewportOnly) {
    if (!isValid() || pixels.isEmpty() || qint64(pixels.width()) * pixels.height() > 33554432)
        return {};
    makeCurrent();
    GLint maxRenderbuffer = 0, maxViewport[2] = {}, maxSamples = 0;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbuffer);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (pixels.width() > qMin(maxRenderbuffer, maxViewport[0]) ||
        pixels.height() > qMin(maxRenderbuffer, maxViewport[1])) {
        doneCurrent();
        return {};
    }
    QImage captured;
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GL_RGBA8);
        format.setSamples(m_style.antiAliasing ? qMin(4, maxSamples) : 0);
        QOpenGLFramebufferObject target(pixels, format);
        if (target.isValid() && target.bind()) {
            QOpenGLPaintDevice paintDevice(pixels);
            m_captureSize = pixels;
            m_captureFramebuffer = target.handle();
            m_capturePaintDevice = &paintDevice;
            m_transparentCapture = transparent;
            m_viewportOnlyCapture = viewportOnly;
            paintGL();
            captured = target.toImage(); // Qt resolves multisamples before readback.
            m_captureSize = {};
            m_captureFramebuffer = 0;
            m_capturePaintDevice = nullptr;
            m_transparentCapture = false;
            m_viewportOnlyCapture = false;
            // Release the large export SSAA buffer while its context is current.
            m_supersampleFbo.reset();
            m_supersamplingActive = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        const QSize native = renderPixelSize();
        glViewport(0, 0, native.width(), native.height());
    }
    doneCurrent();
    update();
    if (!captured.isNull()) {
        captured = captured.convertToFormat(transparent ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        captured.setDotsPerMeterX(qRound(600.0 / 0.0254));
        captured.setDotsPerMeterY(qRound(600.0 / 0.0254));
        captured.setColorSpace(QColorSpace::SRgb);
    }
    return captured;
}

QImage MoleculeView::image(bool transparent, bool viewportOnly) {
    m_transparentCapture = transparent;
    m_viewportOnlyCapture = viewportOnly;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    repaint();
    // A queued widget repaint may still contain the preceding opaque frame.
    // Render capture flags synchronously before reading the framebuffer.
    makeCurrent();
    paintGL();
    QImage captured = grabFramebuffer();
    doneCurrent();
    m_transparentCapture = false;
    m_viewportOnlyCapture = false;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    if (transparent) {
        // Coverage averaging produces premultiplied RGB in the OpenGL buffer.
        // Export straight-alpha pixels so PNG/clipboard edges keep their color.
        if (captured.format() == QImage::Format_ARGB32_Premultiplied ||
            captured.format() == QImage::Format_RGBA8888_Premultiplied)
            return captured.convertToFormat(QImage::Format_ARGB32);
        QImage premultiplied = captured.convertToFormat(QImage::Format_ARGB32);
        premultiplied.reinterpretAsFormat(QImage::Format_ARGB32_Premultiplied);
        return premultiplied.convertToFormat(QImage::Format_ARGB32);
    }
    return captured;
}

void MoleculeView::setVibrationVectorScaleFactor(double factor) {
    m_vibrationVectorScaleFactor = qBound(0.05, factor, 4.0);
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setBonds(const QVector<Bond> &bonds) {
    m_molecule.bonds = bonds;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::rebuildBonds(double cutoffScale, bool includeHydrogen, bool includeMetalCoordination) {
    m_molecule.rebuildBonds(cutoffScale, includeHydrogen, includeMetalCoordination, false);
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setSelectedAtom(int index) {
    m_selectedAtom = index;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setFocusAtoms(const QVector<int> &indices) {
    m_focusAtoms = indices;
    updateRotationCenter(true);
}

void MoleculeView::setGeometrySelection(const QVector<int> &indices) {
    m_geometrySelection = indices;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setMeasurementVisible(int kind, bool visible) {
    if (kind < 0 || kind > 2) return;
    m_measurementVisible[kind] = visible;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

void MoleculeView::setMeasurementStyle(int kind, const MeasurementTextStyle &style) {
    if (kind < 0 || kind > 2) return;
    m_measurementStyles[kind] = style;
    update();
    if(m_previewOverlay)m_previewOverlay->update();
}

} // namespace youngcraft
