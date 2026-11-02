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
#include <QOpenGLContext>
#include <QThread>
#include <QWheelEvent>

#include <cmath>
#include <iterator>

namespace youngcraft {

MoleculeView::MoleculeView(QWidget *parent) : QOpenGLWidget(parent) {
    QSurfaceFormat requested = format();
    requested.setAlphaBufferSize(8);
    requested.setDepthBufferSize(24);
    static const int samples = [requested] {
        QSurfaceFormat eight = requested;
        eight.setSamples(8);
        QOpenGLContext probe;
        probe.setFormat(eight);
        return probe.create() && probe.format().samples() >= 8 ? 8 : 4;
    }();
    requested.setSamples(samples);
    setFormat(requested);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_rotation = QQuaternion();
    m_previewOverlay = new QWidget(this);
    m_previewOverlay->setObjectName(QStringLiteral("DrawingPreviewOverlay"));
    m_previewOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_previewOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_previewOverlay->setAttribute(Qt::WA_NoSystemBackground);
    m_previewOverlay->setGeometry(rect());
    m_previewOverlay->installEventFilter(this);
    m_previewOverlay->show();
}

MoleculeView::~MoleculeView() {
    if (context()) makeCurrent();
    clearSurfaceBuffers();
    m_supersampleFbo.reset();
    m_downsampleVao.destroy();
    delete m_downsampleProgram;
    delete m_patternProgram;
    if(m_patternTexture)glDeleteTextures(1,&m_patternTexture);
    delete m_program;
    delete m_lineProgram;
    delete m_depthProgram;
    if (m_shadowTexture) glDeleteTextures(1, &m_shadowTexture);
    if (m_shadowFbo) glDeleteFramebuffers(1, &m_shadowFbo);
    m_sphereBuffer.destroy();
    m_sphereMediumBuffer.destroy();
    m_sphereLowBuffer.destroy();
    m_cylinderBuffer.destroy();
    m_lineBuffer.destroy();
    m_sphereVao.destroy();
    m_sphereMediumVao.destroy();
    m_sphereLowVao.destroy();
    m_cylinderVao.destroy();
    m_lineVao.destroy();
    doneCurrent();
}

} // namespace youngcraft
