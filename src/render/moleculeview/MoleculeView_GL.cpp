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

namespace {
#include "render/moleculeview/MoleculeShaders.inc"

QVector<MoleculeView::Vertex> makeSphere(int slices, int stacks) {
    QVector<MoleculeView::Vertex> vertices;
    for (int stack = 0; stack < stacks; ++stack) {
        const float v0 = float(stack) / stacks;
        const float v1 = float(stack + 1) / stacks;
        const float phi0 = kPi * (v0 - 0.5f);
        const float phi1 = kPi * (v1 - 0.5f);
        for (int slice = 0; slice < slices; ++slice) {
            const float u0 = float(slice) / slices;
            const float u1 = float(slice + 1) / slices;
            const float theta0 = 2.0f * kPi * u0;
            const float theta1 = 2.0f * kPi * u1;
            const QVector3D p00(std::cos(phi0) * std::cos(theta0), std::cos(phi0) * std::sin(theta0), std::sin(phi0));
            const QVector3D p01(std::cos(phi0) * std::cos(theta1), std::cos(phi0) * std::sin(theta1), std::sin(phi0));
            const QVector3D p10(std::cos(phi1) * std::cos(theta0), std::cos(phi1) * std::sin(theta0), std::sin(phi1));
            const QVector3D p11(std::cos(phi1) * std::cos(theta1), std::cos(phi1) * std::sin(theta1), std::sin(phi1));
            // Counter-clockwise from outside, consistent with the outward normals.
            // Reversed winding exposes the far hemisphere when back-face culling is on.
            vertices << MoleculeView::Vertex{p00, p00} << MoleculeView::Vertex{p11, p11}
                     << MoleculeView::Vertex{p10, p10} << MoleculeView::Vertex{p00, p00}
                     << MoleculeView::Vertex{p01, p01} << MoleculeView::Vertex{p11, p11};
        }
    }
    return vertices;
}

QVector<MoleculeView::Vertex> makeCylinder(int slices) {
    QVector<MoleculeView::Vertex> vertices;
    for (int slice = 0; slice < slices; ++slice) {
        const float a0 = 2.0f * kPi * slice / slices;
        const float a1 = 2.0f * kPi * (slice + 1) / slices;
        const QVector3D n0(std::cos(a0), std::sin(a0), 0.0f);
        const QVector3D n1(std::cos(a1), std::sin(a1), 0.0f);
        const QVector3D p0(n0.x(), n0.y(), -0.5f);
        const QVector3D p1(n1.x(), n1.y(), -0.5f);
        const QVector3D p2(n1.x(), n1.y(), 0.5f);
        const QVector3D p3(n0.x(), n0.y(), 0.5f);
        vertices << MoleculeView::Vertex{p0, n0} << MoleculeView::Vertex{p1, n1}
                 << MoleculeView::Vertex{p2, n1} << MoleculeView::Vertex{p0, n0}
                 << MoleculeView::Vertex{p2, n1} << MoleculeView::Vertex{p3, n0};
    }
    return vertices;
}

} // namespace

void MoleculeView::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!createPrograms()) return;
    createMeshes();
    glGenFramebuffers(1, &m_shadowFbo);
    glGenTextures(1, &m_shadowTexture);
    glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowMapSize, m_shadowMapSize,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Compare each depth sample before filtering, rather than interpolating depth
    // first and making a discontinuous binary decision in the fragment shader.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowTexture, 0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    m_shadowReady = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    uploadSurfaceMeshes();
    m_glReady = true;
}

void MoleculeView::resizeGL(int width, int height) {
    const qreal dpr = devicePixelRatioF();
    glViewport(0, 0, qRound(width * dpr), qRound(height * dpr));
    if (m_previewOverlay) m_previewOverlay->setGeometry(rect());
}

bool MoleculeView::createPrograms() {
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader);
    m_program->link();
    m_lineProgram = new QOpenGLShaderProgram;
    m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kLineVertexShader);
    m_lineProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kLineFragmentShader);
    m_lineProgram->link();
    m_depthProgram = new QOpenGLShaderProgram;
    m_depthProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kDepthVertexShader);
    m_depthProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kDepthFragmentShader);
    m_depthProgram->link();
    m_downsampleProgram = new QOpenGLShaderProgram;
    m_downsampleProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        #version 330 core
        out vec2 uv;
        void main() {
            vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
            uv = p;
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        }
    )");
    m_downsampleProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D scene;
        out vec4 color;
        void main() {
            // At a 2:1 ratio, bilinear sampling averages exactly four source pixels.
            color = texture(scene, uv);
        }
    )");
    m_downsampleProgram->link();
    m_patternProgram=new QOpenGLShaderProgram;
    m_patternProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,R"(
        #version 330 core
        void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.0-1.0,0,1);}
    )");
    m_patternProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,R"(
        #version 330 core
        uniform sampler2D motif;
        uniform vec2 origin;
        uniform float pixelScale;
        uniform float targetHeight;
        uniform float tileSize;
        uniform vec3 paper;
        uniform float eyeGlow;
        float eyeFlare(vec2 uv,vec2 eye) {
            vec2 d=uv-eye;
            float halo=0.65*exp(-dot(d,d)/0.00045);
            float core=exp(-dot(d,d)/0.000016);
            float rays=0.75*exp(-abs(d.x)/0.045-abs(d.y)/0.0018)
                      +0.45*exp(-abs(d.x)/0.002-abs(d.y)/0.025);
            return halo+core+rays;
        }
        out vec4 color;
        void main(){
            vec2 point=vec2(gl_FragCoord.x,targetHeight-gl_FragCoord.y)/pixelScale+origin;
            vec4 ink=texture(motif,point/tileSize);
            vec3 base=mix(paper,ink.rgb,ink.a);
            vec2 uv=point/tileSize;
            float flare=eyeGlow*(eyeFlare(uv,vec2(0.410,0.299))+eyeFlare(uv,vec2(0.558,0.291)));
            base=mix(base,vec3(0.55,0.88,1.0),clamp(flare*0.7,0.0,1.0));
            base+=vec3(0.75,0.9,1.0)*flare*0.85;
            color=vec4(base,1.0);
        }
    )");
    m_patternProgram->link();
    m_downsampleVao.create();
    for (auto *program : {m_program, m_lineProgram, m_depthProgram, m_downsampleProgram, m_patternProgram}) {
        if (!program->isLinked()) { qWarning() << "Molecular shader failed:" << program->log(); return false; }
    }
    return true;
}

void MoleculeView::createMeshes() {
    const QVector<Vertex> sphere = makeSphere(128, 96);
    m_program->bind();
    m_sphereVao.create();
    m_sphereVao.bind();
    m_sphereBuffer.create();
    m_sphereBuffer.bind();
    m_sphereBuffer.allocate(sphere.constData(), sphere.size() * int(sizeof(Vertex)));
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));
    m_sphereBuffer.release();
    m_sphereVao.release();
    m_sphereVertexCount = sphere.size();

    const auto uploadSphere = [this](QOpenGLVertexArrayObject &vao, QOpenGLBuffer &buffer,
                                     int slices, int stacks, int &count) {
        const QVector<Vertex> mesh = makeSphere(slices, stacks);
        vao.create(); vao.bind();
        buffer.create(); buffer.bind();
        buffer.allocate(mesh.constData(), mesh.size() * int(sizeof(Vertex)));
        m_program->enableAttributeArray(0);
        m_program->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
        m_program->enableAttributeArray(1);
        m_program->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));
        buffer.release(); vao.release();
        count = mesh.size();
    };
    uploadSphere(m_sphereMediumVao, m_sphereMediumBuffer, 64, 48, m_sphereMediumVertexCount);
    uploadSphere(m_sphereLowVao, m_sphereLowBuffer, 32, 24, m_sphereLowVertexCount);

    const QVector<Vertex> cylinder = makeCylinder(64);
    m_cylinderVao.create();
    m_cylinderVao.bind();
    m_cylinderBuffer.create();
    m_cylinderBuffer.bind();
    m_cylinderBuffer.allocate(cylinder.constData(), cylinder.size() * int(sizeof(Vertex)));
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, position), 3, sizeof(Vertex));
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));
    m_cylinderBuffer.release();
    m_cylinderVao.release();
    m_cylinderVertexCount = cylinder.size();
    m_program->release();

    const QVector<QVector3D> lines = {
        {0, 0, 0}, {2, 0, 0}, {1.7f, 0.15f, 0}, {2, 0, 0}, {1.7f, -0.15f, 0},
        {0, 0, 0}, {0, 2, 0}, {0.15f, 1.7f, 0}, {0, 2, 0}, {-0.15f, 1.7f, 0},
        {0, 0, 0}, {0, 0, 2}, {0.15f, 0, 1.7f}, {0, 0, 2}, {-0.15f, 0, 1.7f}};
    QVector<float> lineData;
    const QVector3D colors[] = {{0.95f, 0.25f, 0.25f}, {0.25f, 0.95f, 0.35f}, {0.35f, 0.55f, 1.0f}};
    for (int i = 0; i < lines.size(); ++i) {
        const QVector3D color = colors[(i / 5) % 3];
        lineData << lines[i].x() << lines[i].y() << lines[i].z() << color.x() << color.y() << color.z();
    }
    m_lineVao.create();
    m_lineVao.bind();
    m_lineBuffer.create();
    m_lineBuffer.bind();
    m_lineBuffer.allocate(lineData.constData(), lineData.size() * int(sizeof(float)));
    m_lineProgram->enableAttributeArray(0);
    m_lineProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
    m_lineProgram->enableAttributeArray(1);
    m_lineProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    m_lineBuffer.release();
    m_lineVao.release();
    m_lineProgram->release();
}

} // namespace youngcraft
