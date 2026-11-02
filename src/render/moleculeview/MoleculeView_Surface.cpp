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

void MoleculeView::setCubeGrid(const std::shared_ptr<const CubeGrid> &grid) {
    m_potentialGrid.reset();
    m_cubeGrid = grid;
    m_fitNewSurface = bool(grid);
    ++m_surfaceGeneration;
    m_positiveSurface.reset();
    m_negativeSurface.reset();
    m_positiveSurfaceVertexCount = 0;
    m_negativeSurfaceVertexCount = 0;
    if (m_glReady) {
        makeCurrent();
        clearSurfaceBuffers();
        doneCurrent();
    }
    if (m_cubeGrid && m_cubeGrid->valid) rebuildIsosurfaces();
    update();
}

void MoleculeView::setPotentialGrid(std::shared_ptr<const CubeGrid> grid, double minimum, double maximum, QColor low, QColor high) {
    m_potentialGrid = std::move(grid); m_potentialMin=minimum; m_potentialMax=maximum; m_potentialLow=low; m_potentialHigh=high;
    if(m_glReady) { makeCurrent(); uploadSurfaceMeshes(); doneCurrent(); }
    update();
}

void MoleculeView::clearCubeGrid() {
    setCubeGrid({});
}

void MoleculeView::clearSurfaceBuffers() {
    m_positiveSurfaceBuffer.destroy();
    m_negativeSurfaceBuffer.destroy();
    m_positiveSurfaceVao.destroy();
    m_negativeSurfaceVao.destroy();
    m_positiveSurfaceVertexCount = 0;
    m_negativeSurfaceVertexCount = 0;
}

void MoleculeView::uploadSurfaceMeshes() {
    if (!m_program) return;
    clearSurfaceBuffers();

    auto upload = [this](const std::shared_ptr<SurfaceMesh> &mesh,
                         QOpenGLBuffer &buffer, QOpenGLVertexArrayObject &vao,
                         int &vertexCount) {
        if (!mesh || mesh->positions.isEmpty()) return;
        const int count = qMin(mesh->positions.size(), mesh->normals.size());
        if (count <= 0) return;
        QVector<Vertex> vertices;
        vertices.reserve(count);
        for (int i = 0; i < count; ++i) {
            QVector3D color(0.5f,0.5f,0.5f);
            if (m_potentialGrid) {
                const float v=m_potentialGrid->sample(mesh->positions[i]);
                if(std::isfinite(v) && m_potentialMax>m_potentialMin) {
                    const float t=qBound(0.0, (v-m_potentialMin)/(m_potentialMax-m_potentialMin), 1.0);
                    const QVector3D lo(m_potentialLow.redF(),m_potentialLow.greenF(),m_potentialLow.blueF());
                    const QVector3D hi(m_potentialHigh.redF(),m_potentialHigh.greenF(),m_potentialHigh.blueF());
                    const QVector3D mid(1,1,1);
                    color=t<0.5f?lo*(1-2*t)+mid*(2*t):mid*(2-2*t)+hi*(2*t-1);
                }
            }
            vertices.push_back({mesh->positions[i], mesh->normals[i], color});
        }

        vao.create();
        vao.bind();
        buffer.create();
        buffer.bind();
        buffer.allocate(vertices.constData(), vertices.size() * int(sizeof(Vertex)));
        m_program->enableAttributeArray(0);
        m_program->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
        m_program->enableAttributeArray(1);
        m_program->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));
        m_program->enableAttributeArray(2);
        m_program->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, color), 3, sizeof(Vertex));
        buffer.release();
        vao.release();
        vertexCount = count;
    };

    m_program->bind();
    upload(m_positiveSurface, m_positiveSurfaceBuffer, m_positiveSurfaceVao,
           m_positiveSurfaceVertexCount);
    upload(m_negativeSurface, m_negativeSurfaceBuffer, m_negativeSurfaceVao,
           m_negativeSurfaceVertexCount);
    m_program->release();
}

void MoleculeView::rebuildIsosurfaces() {
    if (!m_cubeGrid || !m_cubeGrid->valid) return;
    const std::shared_ptr<const CubeGrid> grid = m_cubeGrid;
    const RenderStyle style = m_style;
    const quint64 generation = ++m_surfaceGeneration;
    QPointer<MoleculeView> receiver(this);
    QThread *thread = QThread::create([receiver, grid, style, generation] {
        const auto positive = std::make_shared<SurfaceMesh>(
            IsosurfaceBuilder::build(*grid, style.isoValue, true));
        const auto negative = std::make_shared<SurfaceMesh>(
            IsosurfaceBuilder::build(*grid, style.isoValue, false));
        if (!receiver) return;
        QMetaObject::invokeMethod(receiver, [receiver, generation, positive, negative] {
            if (!receiver || generation != receiver->m_surfaceGeneration) return;
            receiver->m_positiveSurface = positive;
            receiver->m_negativeSurface = negative;
            if(receiver->m_fitNewSurface){receiver->m_fitNewSurface=false;receiver->bestView();}
            if (receiver->m_glReady) {
                receiver->makeCurrent();
                receiver->uploadSurfaceMeshes();
                receiver->doneCurrent();
            }
            receiver->update();
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

} // namespace youngcraft
