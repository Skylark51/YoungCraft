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

void MoleculeView::drawDepthMesh(QOpenGLVertexArrayObject &vao, int vertexCount,
                                 const QMatrix4x4 &model) {
    m_depthProgram->setUniformValue("uModel", model);
    m_depthProgram->setUniformValue("uLightMvp", m_lightViewProjection);
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    vao.release();
}

void MoleculeView::renderShadowMap(const QVector3D &lightDirection) {
    if (!m_style.shadows || !m_shadowReady || !m_depthProgram || m_molecule.atoms.isEmpty()) return;
    QVector3D direction = lightDirection.normalized();
    if (direction.lengthSquared() < 0.1f) direction = QVector3D(1, 1, 1).normalized();
    const QVector3D eye = m_center + direction * qMax(5.0f, m_radius * 4.0f);
    QMatrix4x4 lightView; lightView.lookAt(eye, m_center, qAbs(QVector3D::dotProduct(direction, QVector3D(0, 1, 0))) > 0.95f
                                                        ? QVector3D(1, 0, 0) : QVector3D(0, 1, 0));
    QMatrix4x4 lightProjection;
    const float extent = qMax(2.0f, m_radius * 1.5f);
    lightProjection.ortho(-extent, extent, -extent, extent, 0.1f, qMax(20.0f, m_radius * 10.0f));
    m_lightViewProjection = lightProjection * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
    glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1f, 1.0f);
    m_depthProgram->bind();
    m_depthPass = true;
    if (m_style.displayMode != DisplayMode::SpaceFilling) {
        // Share the visible geometry, including multiple, dashed and trimmed bonds.
        for (const Bond &bond : m_molecule.bonds) drawColoredBond(bond);
    }
    for (const Atom &atom : m_molecule.atoms) {
        if (!atom.visible || atom.displayOpacity < 1.0f || (!m_style.showHydrogen && atom.atomicNumber == 1)) continue;
        // Exactly the same tessellation as the color pass: a finer shadow mesh
        // otherwise sits outside a coarse visible sphere and casts a shadow on it.
        drawAtomSphere(atom);
    }
    m_depthPass = false;
    m_depthProgram->release();
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, renderTargetFramebuffer());

    glViewport(0, 0, renderPixelSize().width(), renderPixelSize().height());
}

} // namespace youngcraft
