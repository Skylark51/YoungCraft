#pragma once

#include "model/Molecule.h"
#include "render/CameraState.h"
#include "model/GeometryEditor.h"
#include "model/MoleculeEditController.h"
#include "model/FragmentLibrary.h"
#include "settings/StyleManager.h"

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QCache>

#include <memory>

class SmokeTest;
class QPainter;

namespace youngcraft {

struct CubeGrid;
struct SurfaceMesh;

class MoleculeView final : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    friend class ::SmokeTest;
    Q_OBJECT
public:
    explicit MoleculeView(QWidget *parent = nullptr);
    ~MoleculeView() override;

    void setMolecule(const Molecule &molecule);
    // Update an animation inside already fitted scene bounds, keeping the camera fixed.
    void setAnimationMolecule(const Molecule &molecule);
    void setAtomVisibility(const QVector<bool> &visibility);
    void setAtomDisplay(const Molecule &appearance);
    void setEditedMolecule(const Molecule &molecule);
    void setCoordinates(const QVector<QVector3D> &coordinates);
    void setFrequencyMode(const FrequencyMode *mode);
    void setCubeGrid(const std::shared_ptr<const CubeGrid> &grid, float isoValue = -1.0f);
    void clearCubeGrid();
    void setPotentialGrid(std::shared_ptr<const CubeGrid> grid, double minimum, double maximum, QColor low, QColor high);
    void setStyle(const RenderStyle &style);
    void setCrystalCell(const QVector3D &a, const QVector3D &b, const QVector3D &c,
                        int repeat, bool visible);
    const RenderStyle &style() const { return m_style; }
    const Molecule &molecule() const { return m_molecule; }
    QImage image(bool transparent = false, bool viewportOnly = false);
    // Render fresh geometry at the requested pixel size, independently of the widget.
    QImage imageAtSize(const QSize &pixels, bool transparent = false, bool viewportOnly = false);
    void setVibrationVectorScaleFactor(double factor);
    void setVibrationVectorLengthFactor(double factor);
    void setBonds(const QVector<Bond> &bonds);
    void setDrawingMode(bool enabled) {
        if(m_drawingMode!=enabled)setDrawingArmed(false);
        m_drawingMode = enabled;
        if (!enabled) endDrawingChain();
        setCursor(enabled && m_drawingArmed ? Qt::CrossCursor : Qt::ArrowCursor);
        if (m_previewOverlay) m_previewOverlay->update();
    }
    void setAtomDragMode(bool enabled) { m_atomDragMode = enabled; if(!enabled)m_transformMode=0; }
    // 0 selected atoms; 1 attached-branch angle; 2 attached-branch torsion; 3/4 clicked component translate/rotate.
    void setTransformMode(int mode) { m_transformMode=mode; }
    int transformMode() const { return m_transformMode; }
    bool drawingArmed() const { return m_drawingArmed; }
    void setDrawingArmed(bool armed) {
        m_drawingArmed=armed;
        if(!armed){endDrawingChain();clearInsertionPreview();setProperty("pendingFragment",QString());setProperty("pendingCenter",0);}
        setCursor(armed?Qt::CrossCursor:Qt::ArrowCursor);
        emit drawingToolChanged(armed);
    }
    void setDrawingElement(int atomicNumber) { setDrawingArmed(true); m_drawAtomicNumber = atomicNumber; if (m_previewOverlay) m_previewOverlay->update(); }
    SelectionModel &selection() { return m_selection; }
    const SelectionModel &selection() const { return m_selection; }
    void setDrawingBondOrder(double order) { m_drawOrder=order; }
    void setChainDrawing(bool enabled) { m_chainDrawing=enabled;endDrawingChain(); }
    void setDrawingAnchorAt(const QVector3D &position);
    void endDrawingChain() { m_chainAtom=-1;m_drawStartAtom=-1;m_drawFromCanvas=false;m_ghostActive=false;if(m_previewOverlay)m_previewOverlay->update(); }
    bool bondCentricMode() const { return m_bondCentric; }
    void setBondCentricMode(bool on) { m_bondCentric=on; m_activeBond=-1; }
    void setFragmentPreview(const Fragment &f) { m_drawingArmed=true; m_insertFragment=f; m_insertPreview=true; m_templatePreview=-1; }
    void setTemplatePreview(int g) { m_drawingArmed=true; m_templatePreview=g; m_insertPreview=true; }
    void clearInsertionPreview() { m_insertPreview=false; m_templatePreview=-1; if(m_previewOverlay)m_previewOverlay->update(); }
    bool hasGhostBond() const { return m_ghostActive; }
    QVector3D ghostPosition() const { return m_ghostPosition; }
    void rebuildBonds(double cutoffScale, bool includeHydrogen, bool includeMetalCoordination);
    void centerOnMolecule();
    void bestView();
    void isometricView();
    void updateRotationCenter(bool selectedOnly = false);
    void setSelectedAtom(int index);
    int selectedAtom() const { return m_selectedAtom; }
    void setFocusAtoms(const QVector<int> &indices);
    void setGeometrySelection(const QVector<int> &indices);
    const QVector<int> &geometrySelection() const { return m_geometrySelection; }
    void setMeasurementVisible(int kind, bool visible);
    void setMeasurementStyle(int kind, const MeasurementTextStyle &style);
    CameraState cameraState() const;
    void setCameraState(const CameraState &state);
    // -2: free screen rotation; 0/1/2: screen X/Y/Z; -1: standard viewer.
    void setCaptureRotation(int axis, float sensitivity=1.0f) { m_captureRotation=axis; m_captureSensitivity=sensitivity; }
    void saveCameraToClipboard();
    void restoreCameraFromClipboard();

signals:
    void geometryChanged();
    void cameraChanged();
    void atomEditRequested(int index);
    void bondEditRequested(int index);
    void deleteRequested(int atom, int bond);
    void attachmentClicked(int center, const QVector3D &direction);
    void drawingToolChanged(bool armed);
    void groupMoved(const QVector3D &translation);
    void groupRotated(const QQuaternion &rotation, const QVector3D &center);
    void bondManipulated(int first, int second, int kind, double value, bool opposite);
    void atomPicked(int index);
    void canvasClicked(const QVector3D &position);
    void bondDrawn(int first, int second, const QVector3D &position);
    void bondStrokeDrawn(const QVector3D &start, const QVector3D &end);
    void atomMoved(int index, const QVector3D &position);
    void interactionStarted();
    void interactionFinished();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

public:
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector3D color;
    };

private:

    bool m_fitNewSurface = false;
    std::shared_ptr<const CubeGrid> m_potentialGrid;
    double m_potentialMin = -0.05, m_potentialMax = 0.05;
    QColor m_potentialLow = Qt::red, m_potentialHigh = Qt::blue;
    bool createPrograms();
    QSize renderPixelSize() const;
    QSizeF renderLogicalSize() const;
    GLuint renderTargetFramebuffer() const;
    void beginOverlayPainter(QPainter &painter);
    void drawPatternBackground();
    QOpenGLShaderProgram *m_patternProgram=nullptr;
    GLuint m_patternTexture=0;
    QString m_patternTextureKey;

    bool beginSupersampling();
    void resolveSupersampling();
    void createMeshes();
    void drawMesh(QOpenGLVertexArrayObject &vao, int vertexCount, const QMatrix4x4 &model,
                  const QColor &color, bool lighting = true, float opacity = -1.0f,
                  const QColor &secondColor = QColor(), float colorStart = 0.0f, float colorEnd = 1.0f);
    int sphereMeshLevel(const QVector3D &position, float radius) const;
    void drawAtomSphere(const Atom &atom);
    void drawBondCap(const QVector3D &center, const QVector3D &axis, float radius, float axialRadius,
                     const QColor &firstColor, const QColor &secondColor, float colorPosition);
    void drawSurface(QOpenGLVertexArrayObject &vao, int vertexCount, const QColor &color);
    void drawAxes(const QMatrix4x4 &viewProjection);
    void drawCrystalCell(const QMatrix4x4 &viewProjection);
    void drawLabels(const QMatrix4x4 &viewProjection);
    void drawVibrationVectors(const QMatrix4x4 &viewProjection);
    void drawGeometryMeasurements(const QMatrix4x4 &viewProjection, QPainter *overlayPainter = nullptr);
    void drawEditorPreview();
    void renderShadowMap(const QVector3D &lightDirection);
    void drawDepthMesh(QOpenGLVertexArrayObject &vao, int vertexCount, const QMatrix4x4 &model);
    QColor atomRenderColor(const Atom &atom) const;
    float atomVisualRadius(const Atom &atom) const;
    QVector3D stableBondSideVector(const Bond &bond, const QVector3D &axis) const;
    void drawBondSegment(const QVector3D &start, const QVector3D &end, float radius,
                         const QColor &firstColor, const QColor &secondColor, bool dashed, float trim,
                         bool roundStart, bool roundEnd);
    void drawColoredBond(const Bond &bond);
    QMatrix4x4 viewProjection() const;
    QVector3D rotationCenter() const;
    void recalculateBounds();
    int pickAtom(const QPoint &point) const;
    int pickBond(const QPoint &point) const;
    int pickSite(const QPoint &point) const;
    void updateDrawingPreview(const QPoint &point, Qt::KeyboardModifiers modifiers);
    QVector3D pointOnDrawingPlane(const QPoint &point, const QVector3D &origin) const;
    QVector3D project(const QVector3D &position, const QMatrix4x4 &viewProjection) const;
    void rotateBy(const QPoint &delta);
    void panBy(const QPoint &delta);
    void clearSurfaceBuffers();
    void uploadSurfaceMeshes();
    void rebuildIsosurfaces();

    Molecule m_molecule;
    SelectionModel m_selection;
    int m_hoverAtom=-1, m_hoverBond=-1;
    double m_drawOrder=0, m_previewOrder=1;
    bool m_rectangle=false, m_groupDrag=false;
    QVector3D m_groupTranslation, m_groupCenter, m_groupAxis{1,0,0};
    QQuaternion m_groupRotation;
    int m_transformMode=0;
    bool m_insertPreview=false;
    Fragment m_insertFragment;
    int m_templatePreview=-1;
    QVector3D m_insertPosition, m_insertDirection{1,0,0};
    bool m_bondCentric=false, m_centricDrag=false;
    int m_activeBond=-1, m_centricKind=0;
    double m_centricStart=0, m_centricValue=0;
    QString m_centricText;
    Molecule m_centricProbe;
    QSet<int> m_centricAtoms;
    QVector3D m_centricOrigin, m_centricTranslation;
    QQuaternion m_centricRotation;
    bool m_centricRejected=false;
    bool m_drawingMode = false;
    bool m_drawingArmed = false;
    bool m_chainDrawing = false, m_drawFromCanvas = false;
    int m_chainAtom = -1;
    QVector3D m_drawOrigin;
    bool m_atomDragMode = false;
    int m_drawStartAtom = -1;
    int m_dragAtom = -1;
    int m_drawAtomicNumber = 6;
    int m_ghostSnapAtom = -1;
    bool m_ghostActive = false;
    QWidget *m_previewOverlay = nullptr;
    QVector3D m_ghostPosition;
    QVector3D m_dragStartPosition;
    RenderStyle m_style;
    float m_bondDisplayOpacity = 1.0f;
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLShaderProgram *m_lineProgram = nullptr;
    QOpenGLShaderProgram *m_depthProgram = nullptr;
    QOpenGLBuffer m_sphereBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_sphereMediumBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_sphereLowBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_cylinderBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_lineBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject m_sphereVao;
    QOpenGLVertexArrayObject m_sphereMediumVao;
    QOpenGLVertexArrayObject m_sphereLowVao;
    QOpenGLVertexArrayObject m_cylinderVao;
    QOpenGLVertexArrayObject m_lineVao;
    QOpenGLBuffer m_positiveSurfaceBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_negativeSurfaceBuffer{QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject m_positiveSurfaceVao;
    QOpenGLVertexArrayObject m_negativeSurfaceVao;
    int m_sphereVertexCount = 0;
    int m_sphereMediumVertexCount = 0;
    int m_sphereLowVertexCount = 0;
    int m_cylinderVertexCount = 0;
    int m_positiveSurfaceVertexCount = 0;
    int m_negativeSurfaceVertexCount = 0;
    std::unique_ptr<QOpenGLFramebufferObject> m_supersampleFbo;
    QSize m_captureSize;
    GLuint m_captureFramebuffer = 0;
    QPaintDevice *m_capturePaintDevice = nullptr;
    QOpenGLShaderProgram *m_downsampleProgram = nullptr;
    QOpenGLVertexArrayObject m_downsampleVao;
    bool m_supersamplingActive = false;
    bool m_supersamplingWarning = false;
    bool m_glReady = false;
    bool m_shadowReady = false;
    bool m_depthPass = false;
    std::shared_ptr<const CubeGrid> m_cubeGrid;
    std::shared_ptr<SurfaceMesh> m_positiveSurface;
    std::shared_ptr<SurfaceMesh> m_negativeSurface;
    quint64 m_surfaceGeneration = 0;
    struct CachedSurface {
        std::shared_ptr<const CubeGrid> grid; // Keep pointer identity valid while cached.
        std::shared_ptr<SurfaceMesh> positive, negative;
    };
    QCache<QString, CachedSurface> m_surfaceCache{256 * 1024}; // KiB
    bool m_surfaceBuilding = false;
    QVector3D m_center;
    float m_radius = 1.0f;
    QVector3D m_target;
    float m_distance = 8.0f;
    float m_orthoScale = 5.0f;
    QQuaternion m_rotation;
    int m_captureRotation=-1;
    float m_captureSensitivity=1.0f;
    QVector<int> m_focusAtoms;
    int m_selectedAtom = -1;
    QVector<int> m_geometrySelection;
    bool m_measurementVisible[3]{false,false,false};
    MeasurementTextStyle m_measurementStyles[3];
    QPoint m_lastMouse;
    QPoint m_pressMouse;
    Qt::MouseButton m_dragButton = Qt::NoButton;
    bool m_dragging = false;
    QVector<QVector3D> m_frequencyDisplacements;
    QVector<QVector3D> m_frequencyBase;
    double m_vibrationVectorScaleFactor = 1.0;
    double m_vibrationVectorLengthFactor = 1.0;
    bool m_transparentCapture = false;
    bool m_viewportOnlyCapture = false;
    GLuint m_shadowFbo = 0;
    GLuint m_shadowTexture = 0;
    int m_shadowMapSize = 2048;
    QVector3D m_cellA, m_cellB, m_cellC;
    int m_cellRepeat = 1;
    bool m_cellVisible = false;
    QMatrix4x4 m_lightViewProjection;
};

} // namespace youngcraft
