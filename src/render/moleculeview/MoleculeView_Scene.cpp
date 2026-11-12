#include "render/MoleculeView.h"
#include "ui/AppPattern.h"


#include "render/IsosurfaceBuilder.h"

#include <QApplication>
#include <QElapsedTimer>
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
#include <algorithm>

namespace youngcraft {

void MoleculeView::drawPatternBackground() {
    const int motif=window()->property("appPattern").toInt();
    const QColor ink=window()->property("appPatternInk").value<QColor>();
    const bool photograph=window()->property("professorBackdrop").toBool();
    const int side=photograph?qMax(width(),height()):(motif==0?96:motif==5?28:24);
    const QString key=photograph?QStringLiteral("professor-photo-v1"):QString::number(motif)+ink.name();
    if(!m_patternTexture)glGenTextures(1,&m_patternTexture);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,m_patternTexture);
    if(key!=m_patternTextureKey){
        QImage tile;
        if(photograph) {
            tile=QImage(1024,1024,QImage::Format_RGBA8888);tile.fill(Qt::transparent);
            const QImage photo(QStringLiteral(":/youngcraft/professor-watching.png"));
            const QSize fitted=photo.size().scaled(tile.size(),Qt::KeepAspectRatio);
            QPainter painter(&tile);painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.setOpacity(0.60);
            painter.drawImage(QRect(QPoint((tile.width()-fitted.width())/2,(tile.height()-fitted.height())/2),fitted),photo);
        } else {
            tile=QImage(side*2,side*2,QImage::Format_RGBA8888);tile.fill(Qt::transparent);
            QPainter painter(&tile);painter.scale(2,2);auto color=ink;color.setAlpha(38);
            uiPattern::drawMotif(painter,QRect(0,0,side,side),motif,color);
        }
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,tile.width(),tile.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,tile.constBits());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        m_patternTextureKey=key;
    }
    const auto origin=photograph?QPoint((side-width())/2,(side-height())/2):mapTo(window(),QPoint());
    const QSize target=m_supersamplingActive?m_supersampleFbo->size():renderPixelSize();
    const auto paper=window()->property("appPatternPaper").value<QColor>();
    glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);
    m_patternProgram->bind();m_patternProgram->setUniformValue("motif",0);
    m_patternProgram->setUniformValue("origin",QVector2D(origin));
    m_patternProgram->setUniformValue("pixelScale",float(devicePixelRatioF()*(m_supersamplingActive?2:1)));
    m_patternProgram->setUniformValue("targetHeight",float(target.height()));
    m_patternProgram->setUniformValue("tileSize",float(side));
    static QElapsedTimer glowClock;
    if(!glowClock.isValid())glowClock.start();
    // A soft glow builds into a short glint every 2.4 seconds.
    const float phase=float(glowClock.elapsed()%2400)/2400.0f;
    const float pulse=0.16f+0.84f*std::pow(0.5f+0.5f*std::cos(phase*6.2831853f),12.0f);
    m_patternProgram->setUniformValue("eyeGlow",photograph?pulse:0.0f);
    m_patternProgram->setUniformValue("paper",QVector3D(paper.redF(),paper.greenF(),paper.blueF()));
    m_downsampleVao.bind();glDrawArrays(GL_TRIANGLES,0,3);m_downsampleVao.release();m_patternProgram->release();
    glBindTexture(GL_TEXTURE_2D,0);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glEnable(GL_BLEND);
}

void MoleculeView::paintGL() {
    if (!m_glReady) return;
    const QQuaternion cameraToWorld = m_rotation.conjugated();
    const QVector3D viewDirection = cameraToWorld.rotatedVector(QVector3D(0, 0, 1));
    const auto light = [&](QVector3D direction, const QVector3D &fallback) {
        if (direction.lengthSquared() < 1e-6f) direction = fallback;
        direction.normalize();
        return m_style.cameraRelativeLights ? cameraToWorld.rotatedVector(direction) : direction;
    };
    const QVector3D keyLight = light(m_style.lightDirection, QVector3D(-1, 1, 2));
    const QVector3D fillLight = light(m_style.secondLightDirection, QVector3D(1, 0, 1));
    if (m_style.shadows && m_shadowReady) renderShadowMap(keyLight);
    m_supersamplingActive = beginSupersampling();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDepthFunc(GL_LESS);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    if (m_style.antiAliasing) glEnable(GL_MULTISAMPLE); else glDisable(GL_MULTISAMPLE);
    const bool patterned=(window()->property("professorBackdrop").toBool() || uiPattern::active(this)) && !m_transparentCapture
        && !m_viewportOnlyCapture && m_captureSize.isEmpty();
    const QColor background = patterned ? window()->property("appPatternPaper").value<QColor>() : m_style.background;
    if (m_transparentCapture) glClearColor(0, 0, 0, 0);
    else glClearColor(background.redF(), background.greenF(), background.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(patterned)drawPatternBackground();
    if (m_molecule.atoms.isEmpty()) {
        QPainter painter; beginOverlayPainter(painter);
        painter.setPen(m_style.labelColor);
        painter.drawText(QRectF(QPointF(), renderLogicalSize()), Qt::AlignCenter, QStringLiteral("파일을 열어 분자 구조를 표시하세요"));
        return;
    }

    const QMatrix4x4 vp = viewProjection();
    m_program->bind();
    m_program->setUniformValue("uLightDirection", keyLight);
    m_program->setUniformValue("uSecondLightDirection", fillLight);
    m_program->setUniformValue("uViewDirection", viewDirection);
    m_program->setUniformValue("uPerspective", m_style.perspective);
    m_program->setUniformValue("uAmbientUp", m_style.cameraRelativeLights
        ? cameraToWorld.rotatedVector(QVector3D(0, 1, 0)) : QVector3D(0, 1, 0));
    float depthExtent = 0.5f;
    for (const Atom &atom : m_molecule.atoms)
        if (atom.visible && (m_style.showHydrogen || atom.atomicNumber != 1))
            depthExtent = qMax(depthExtent, qAbs(QVector3D::dotProduct(atom.position - m_center, viewDirection))
                                          + atomVisualRadius(atom));
    m_program->setUniformValue("uDepthCenter", m_center);
    m_program->setUniformValue("uDepthSpan", 2.0f * depthExtent);
    m_program->setUniformValue("uLightColor", m_style.lightColor.redF(), m_style.lightColor.greenF(), m_style.lightColor.blueF());
    m_program->setUniformValue("uSecondLightColor", m_style.secondLightColor.redF(), m_style.secondLightColor.greenF(), m_style.secondLightColor.blueF());
    m_program->setUniformValue("uAmbient", m_style.ambientIntensity);
    m_program->setUniformValue("uDiffuse", m_style.diffuseIntensity);
    m_program->setUniformValue("uSpecular", m_style.specularIntensity);
    m_program->setUniformValue("uShininess", m_style.shininess);
    m_program->setUniformValue("uAtomSheenStrength", m_style.atomSheenStrength);
    m_program->setUniformValue("uColorSaturation", m_style.colorSaturation);
    m_program->setUniformValue("uOpacity", m_style.opacity);
    m_program->setUniformValue("uLighting", m_style.lighting);
    m_program->setUniformValue("uTwoLights", m_style.twoLights);
    m_program->setUniformValue("uDepthCueing", m_style.depthCueing);
    m_program->setUniformValue("uDepthCueStrength", m_style.depthCueStrength);
    m_program->setUniformValue("uBackground", background.redF(), background.greenF(), background.blueF());
    const QVector3D cameraPosition = m_target + m_rotation.conjugated().rotatedVector(QVector3D(0, 0, m_distance));
    m_program->setUniformValue("uCameraPosition", cameraPosition);
    m_program->setUniformValue("uShadows", m_style.shadows && m_shadowReady);
    m_program->setUniformValue("uShadowStrength", m_style.shadowStrength);
    const float shadowExtent = qMax(2.0f, m_radius * 1.5f);
    // A small world-space penumbra softens shadows without changing atom colors.
    m_program->setUniformValue("uShadowFilterRadius",
        qBound(2.0f, 0.10f * m_shadowMapSize / (2.0f * shadowExtent), 16.0f));
    m_program->setUniformValue("uLightMvp", m_lightViewProjection);
    m_program->setUniformValue("uShadowMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_shadowTexture);

    if (m_style.displayMode != DisplayMode::SpaceFilling) {
        m_program->setUniformValue("uMaterialSpecular", 0.42f);
        m_program->setUniformValue("uMaterialShininess", 0.72f);
        for (const Bond &bond : m_molecule.bonds) {
            if (bond.first < 0 || bond.second < 0 || bond.first >= m_molecule.atoms.size() || bond.second >= m_molecule.atoms.size()) continue;
            if (qMin(m_molecule.atoms[bond.first].displayOpacity, m_molecule.atoms[bond.second].displayOpacity) * bond.displayOpacity >= 1.0f)
                drawColoredBond(bond);
        }
    }

    if ((m_style.showPositiveSurface && m_positiveSurfaceVertexCount > 0) ||
        (m_style.showNegativeSurface && m_negativeSurfaceVertexCount > 0)) {
        glDisable(GL_CULL_FACE);
        m_program->setUniformValue("uMaterialSpecular", 0.55f);
        m_program->setUniformValue("uMaterialShininess", 0.85f);
        if (m_style.showPositiveSurface) {
            drawSurface(m_positiveSurfaceVao, m_positiveSurfaceVertexCount,
                        m_style.positiveSurfaceColor);
        }
        if (m_style.showNegativeSurface) {
            drawSurface(m_negativeSurfaceVao, m_negativeSurfaceVertexCount,
                        m_style.negativeSurfaceColor);
        }
        glEnable(GL_CULL_FACE);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    for (int i = 0; i < m_molecule.atoms.size(); ++i) {
        const Atom &atom = m_molecule.atoms[i];
        if (!atom.visible || atom.displayOpacity < 1.0f || (!m_style.showHydrogen && atom.atomicNumber == 1)) continue;
        const bool metal = isMetal(atom.atomicNumber);
        m_program->setUniformValue("uMaterialSpecular", metal ? 1.12f : 1.0f);
        m_program->setUniformValue("uMaterialShininess", metal ? 1.18f : 1.0f);
        drawAtomSphere(atom);
    }
    glEnable(GL_BLEND);
    // Draw faded atoms and their bonds back-to-front after opaque geometry.
    // They test against opaque depth but never obscure atoms behind them.
    struct FadedItem { float depth; int index; bool bond; };
    QVector<FadedItem> faded;
    for (int i = 0; i < m_molecule.atoms.size(); ++i) {
        const auto &atom = m_molecule.atoms[i];
        if (atom.visible && atom.displayOpacity < 1.0f && (m_style.showHydrogen || atom.atomicNumber != 1))
            faded.push_back({QVector3D::dotProduct(atom.position, viewDirection), i, false});
    }
    if (m_style.displayMode != DisplayMode::SpaceFilling) for (int i = 0; i < m_molecule.bonds.size(); ++i) {
        const auto &bond = m_molecule.bonds[i];
        if (bond.first < 0 || bond.second < 0 || bond.first >= m_molecule.atoms.size() || bond.second >= m_molecule.atoms.size()) continue;
        const auto &a = m_molecule.atoms[bond.first]; const auto &b = m_molecule.atoms[bond.second];
        if (a.visible && b.visible && qMin(a.displayOpacity, b.displayOpacity) * bond.displayOpacity < 1.0f)
            faded.push_back({QVector3D::dotProduct((a.position + b.position) * 0.5f, viewDirection), i, true});
    }
    std::stable_sort(faded.begin(), faded.end(), [](const FadedItem &a, const FadedItem &b) { return a.depth < b.depth; });
    glDepthMask(GL_FALSE);
    for (const auto &item : faded) {
        if (item.bond) {
            m_program->setUniformValue("uMaterialSpecular", 0.42f);
            m_program->setUniformValue("uMaterialShininess", 0.72f);
            drawColoredBond(m_molecule.bonds[item.index]);
        } else {
            const auto &atom = m_molecule.atoms[item.index];
            m_program->setUniformValue("uMaterialSpecular", isMetal(atom.atomicNumber) ? 1.12f : 1.0f);
            m_program->setUniformValue("uMaterialShininess", isMetal(atom.atomicNumber) ? 1.18f : 1.0f);
            drawAtomSphere(atom);
        }
    }
    glDepthMask(GL_TRUE);
    m_program->release();

    if (m_style.showAxes) drawAxes(vp);
    // Resolve the 3D scene before QPainter draws annotations in logical pixels.
    if (m_supersamplingActive) resolveSupersampling();
    if (m_cellVisible) drawCrystalCell(vp);
    if (m_style.showVibrationVectors) drawVibrationVectors(vp);
    // Screen-space annotations must remain visible over every rendered atom.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    drawLabels(vp);
    drawGeometryMeasurements(vp);
    if (m_potentialGrid && m_style.showPositiveSurface) {
        QPainter painter; beginOverlayPainter(painter);
        const double width=qMin(240.0,renderLogicalSize().width()-40.0);
        const QRectF bar((renderLogicalSize().width()-width)/2,renderLogicalSize().height()-48,width,12);
        QLinearGradient gradient(bar.topLeft(),bar.topRight());gradient.setColorAt(0,m_potentialLow);gradient.setColorAt(.5,Qt::white);gradient.setColorAt(1,m_potentialHigh);
        painter.setPen(QPen(m_style.labelColor,1));painter.setBrush(gradient);painter.drawRect(bar);
        painter.drawText(bar.adjusted(0,-22,0,-12),Qt::AlignCenter,QStringLiteral("MEP (a.u.)"));
        painter.drawText(bar.adjusted(0,14,0,30),Qt::AlignLeft,QString::number(m_potentialMin,'g',4));
        painter.drawText(bar.adjusted(0,14,0,30),Qt::AlignRight,QString::number(m_potentialMax,'g',4));
    }
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

bool MoleculeView::beginSupersampling() {
    const QSize native = renderPixelSize();
    glBindFramebuffer(GL_FRAMEBUFFER, renderTargetFramebuffer());
    glViewport(0, 0, native.width(), native.height());
    if (!m_style.antiAliasing || m_molecule.atoms.isEmpty()) {
        m_supersampleFbo.reset();
        return false;
    }
    const QSize high = native * 2;
    GLint maxTexture = 0, maxViewport[2] = {};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    // Bound framebuffer memory on very large/high-DPI windows; retain native MSAA.
    const bool supported = high.width() <= qMin(maxTexture, maxViewport[0]) &&
        high.height() <= qMin(maxTexture, maxViewport[1]) &&
        qint64(high.width()) * high.height() <= (m_captureSize.isEmpty() ? 16777216 : 33554432);
    if (supported && (!m_supersampleFbo || m_supersampleFbo->size() != high)) {
        m_supersampleFbo.reset();
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GL_RGBA8);
        format.setSamples(0);
        m_supersampleFbo = std::make_unique<QOpenGLFramebufferObject>(high, format);
    }
    if (!supported || !m_supersampleFbo || !m_supersampleFbo->isValid() || !m_supersampleFbo->bind()) {
        m_supersampleFbo.reset();
        glBindFramebuffer(GL_FRAMEBUFFER, renderTargetFramebuffer());
        if (!m_supersamplingWarning) {
            qWarning("2x supersampling unavailable at this viewport size; retaining native MSAA.");
            m_supersamplingWarning = true;
        }
        return false;
    }
    glViewport(0, 0, high.width(), high.height());
    return true;
}

void MoleculeView::resolveSupersampling() {
    glBindFramebuffer(GL_FRAMEBUFFER, renderTargetFramebuffer());
    glViewport(0, 0, renderPixelSize().width(), renderPixelSize().height());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_supersampleFbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_downsampleProgram->bind();
    m_downsampleProgram->setUniformValue("scene", 0);
    m_downsampleVao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    m_downsampleVao.release();
    m_downsampleProgram->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void MoleculeView::drawCrystalCell(const QMatrix4x4 &vp) {
    QPainter painter; beginOverlayPainter(painter);
    painter.setRenderHint(QPainter::Antialiasing, m_style.antiAliasing);
    QColor color = m_style.background.lightnessF() > 0.5f ? QColor(65, 85, 110) : QColor(185, 205, 230);
    color.setAlpha(190);
    painter.setPen(QPen(color, 1.4));
    const auto projected = [this, &vp](const QVector3D &p) {
        const QVector3D q = project(p, vp);
        return QPointF(q.x(), q.y());
    };
    for (int x = 0; x <= m_cellRepeat; ++x)
        for (int y = 0; y <= m_cellRepeat; ++y)
            for (int z = 0; z <= m_cellRepeat; ++z) {
                const QVector3D origin = x * m_cellA + y * m_cellB + z * m_cellC;
                if (x < m_cellRepeat) painter.drawLine(projected(origin), projected(origin + m_cellA));
                if (y < m_cellRepeat) painter.drawLine(projected(origin), projected(origin + m_cellB));
                if (z < m_cellRepeat) painter.drawLine(projected(origin), projected(origin + m_cellC));
            }
}

void MoleculeView::drawMesh(QOpenGLVertexArrayObject &vao, int vertexCount, const QMatrix4x4 &model,
                            const QColor &color, bool lighting, float opacity,
                            const QColor &secondColor, float colorStart, float colorEnd) {
    if (m_depthPass) { drawDepthMesh(vao, vertexCount, model); return; }
    const QMatrix4x4 vp = viewProjection();
    m_program->setUniformValue("uMvp", vp);
    m_program->setUniformValue("uModel", model);
    m_program->setUniformValue("uColor", color.redF(), color.greenF(), color.blueF());
    m_program->setUniformValue("uColorBlend", secondColor.isValid() && secondColor != color);
    if (secondColor.isValid())
        m_program->setUniformValue("uSecondColor", secondColor.redF(), secondColor.greenF(), secondColor.blueF());
    m_program->setUniformValue("uColorStart", colorStart);
    m_program->setUniformValue("uColorEnd", colorEnd);
    m_program->setUniformValue("uLighting", lighting && m_style.lighting);
    m_program->setUniformValue("uOpacity", opacity < 0.0f ? m_style.opacity : opacity);
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    vao.release();
}

void MoleculeView::drawSurface(QOpenGLVertexArrayObject &vao, int vertexCount,
                               const QColor &color) {
    if (vertexCount <= 0) return;
    m_program->setUniformValue("uSurfaceColor", bool(m_potentialGrid));
    QMatrix4x4 model;
    const QMatrix4x4 vp = viewProjection();
    m_program->setUniformValue("uMvp", vp);
    m_program->setUniformValue("uModel", model);
    m_program->setUniformValue("uColor", color.redF(), color.greenF(), color.blueF());
    m_program->setUniformValue("uColorBlend", false);
    m_program->setUniformValue("uOpacity", m_style.isoOpacity);
    m_program->setUniformValue("uSurfacePattern", int(m_style.surfacePattern));
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport);
    m_program->setUniformValue("uPatternScale", float(viewport[2]) / qMax(1, width()));
    m_program->setUniformValue("uLighting", m_style.lighting);
    vao.bind();
    const bool mesh = m_style.surfacePattern == SurfacePattern::Mesh;
    if (mesh) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    if (mesh) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    vao.release();
    m_program->setUniformValue("uSurfacePattern", 0);
    m_program->setUniformValue("uSurfaceColor", false);
}

void MoleculeView::drawAxes(const QMatrix4x4 &vp) {
    if (!m_lineProgram) return;
    QMatrix4x4 model;
    model.translate(m_style.centerAxes ? m_center : QVector3D());
    m_lineProgram->bind();
    m_lineProgram->setUniformValue("uMvp", vp * model);
    m_lineVao.bind();
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 15);
    m_lineVao.release();
    m_lineProgram->release();
}

} // namespace youngcraft
