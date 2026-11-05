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

QColor MoleculeView::atomRenderColor(const Atom &atom) const {
    QColor color;
    if (m_style.monochrome) color = elementColor(atom.atomicNumber, true);
    else if (atom.colorOverride.isValid()) color = atom.colorOverride;
    else color = m_style.elementColors.value(atom.atomicNumber, elementColor(atom.atomicNumber));
    return color;
}

float MoleculeView::atomVisualRadius(const Atom &atom) const {
    float radius = m_style.atomScale;
    if (m_style.displayMode == DisplayMode::SpaceFilling) {
        radius = float(vanDerWaalsRadius(atom.atomicNumber)) * m_style.spaceFillingScale;
    } else if (m_style.displayMode != DisplayMode::Stick) {
        const float covalent = float(covalentRadius(atom.atomicNumber));
        float normalized = atom.atomicNumber == 1
                               ? 0.72f
                               : qBound(0.88f, 0.90f + (covalent - 0.65f) * 0.38f, 1.24f);
        if (isMetal(atom.atomicNumber)) normalized = qMin(1.30f, normalized + 0.08f);
        radius *= normalized;
    }
    return radius * atom.radiusScale * m_style.elementRadiusScales.value(atom.atomicNumber, 1.0f);
}

int MoleculeView::sphereMeshLevel(const QVector3D &position, float radius) const {
    const QVector3D viewDirection = m_rotation.conjugated().rotatedVector(QVector3D(0, 0, 1));
    const QVector3D eye = m_target + viewDirection * m_distance;
    const float distance = m_style.perspective
        ? qMax(0.5f, QVector3D::dotProduct(eye - position, viewDirection)) : qMax(0.5f, m_orthoScale);
    const float pixels = float(renderPixelSize().height()) * (m_supersamplingActive ? 2.0f : 1.0f);
    const float projectedRadius = m_style.perspective
        ? radius * pixels / (2.0f * distance * std::tan(qBound(20.0f, m_style.perspectiveFov, 70.0f) * 0.00872664626f))
        : radius * pixels / (2.0f * distance);
    // Keep silhouette chord error near a tenth of a physical pixel at each level.
    return projectedRadius < 18.0f ? 0 : projectedRadius < 70.0f ? 1 : 2;
}

void MoleculeView::drawAtomSphere(const Atom &atom) {
    const float radius = atomVisualRadius(atom);
    QMatrix4x4 model; model.translate(atom.position); model.scale(radius);
    const int level = sphereMeshLevel(atom.position, radius);
    if (level == 0) drawMesh(m_sphereLowVao, m_sphereLowVertexCount, model, atomRenderColor(atom), true, atom.displayOpacity);
    else if (level == 1) drawMesh(m_sphereMediumVao, m_sphereMediumVertexCount, model, atomRenderColor(atom), true, atom.displayOpacity);
    else drawMesh(m_sphereVao, m_sphereVertexCount, model, atomRenderColor(atom), true, atom.displayOpacity);
}

void MoleculeView::drawBondCap(const QVector3D &center, const QVector3D &axis, float radius, float axialRadius,
                               const QColor &firstColor, const QColor &secondColor, float colorPosition) {
    QMatrix4x4 model; model.translate(center);
    model.rotate(QQuaternion::rotationTo(QVector3D(0, 0, 1), axis));
    model.scale(radius, radius, axialRadius);
    const int level = sphereMeshLevel(center, radius);
    if (level == 0)
        drawMesh(m_sphereLowVao, m_sphereLowVertexCount, model, firstColor, true,
                 m_style.bondOpacity * m_bondDisplayOpacity, secondColor, colorPosition, colorPosition);
    else if (level == 1)
        drawMesh(m_sphereMediumVao, m_sphereMediumVertexCount, model, firstColor, true,
                 m_style.bondOpacity * m_bondDisplayOpacity, secondColor, colorPosition, colorPosition);
    else
        drawMesh(m_sphereVao, m_sphereVertexCount, model, firstColor, true,
                 m_style.bondOpacity * m_bondDisplayOpacity, secondColor, colorPosition, colorPosition);
}

QVector3D MoleculeView::stableBondSideVector(const Bond &bond, const QVector3D &axis) const {
    auto projectedNeighbor = [&](int center, int excluded) {
        QVector3D best;
        float bestLength = 0.0f;
        for (const Bond &candidate : m_molecule.bonds) {
            int neighbor = -1;
            if (candidate.first == center && candidate.second != excluded) neighbor = candidate.second;
            else if (candidate.second == center && candidate.first != excluded) neighbor = candidate.first;
            if (neighbor < 0 || neighbor >= m_molecule.atoms.size()) continue;
            QVector3D value = m_molecule.atoms[neighbor].position - m_molecule.atoms[center].position;
            value -= axis * QVector3D::dotProduct(value, axis);
            if (value.lengthSquared() > bestLength) { best = value; bestLength = value.lengthSquared(); }
        }
        return best;
    };
    QVector3D side = projectedNeighbor(bond.first, bond.second);
    if (side.lengthSquared() < 1e-5f) side = projectedNeighbor(bond.second, bond.first);
    if (side.lengthSquared() < 1e-5f) {
        const QVector3D reference = qAbs(axis.x()) <= qAbs(axis.y()) && qAbs(axis.x()) <= qAbs(axis.z())
                                        ? QVector3D(1, 0, 0)
                                        : (qAbs(axis.y()) <= qAbs(axis.z()) ? QVector3D(0, 1, 0) : QVector3D(0, 0, 1));
        side = QVector3D::crossProduct(axis, reference);
    }
    return side.normalized();
}

void MoleculeView::drawBondSegment(const QVector3D &start, const QVector3D &end, float radius,
                                   const QColor &firstColor, const QColor &secondColor, bool dashed, float trim,
                                   bool roundStart, bool roundEnd) {
    QVector3D vector = end - start;
    const float totalLength = vector.length();
    if (totalLength < 0.001f) return;
    const QVector3D axis = vector / totalLength;
    const QVector3D trimmedStart = start + axis * qMin(trim, totalLength * 0.2f);
    const QVector3D trimmedEnd = end - axis * qMin(trim, totalLength * 0.2f);
    vector = trimmedEnd - trimmedStart;
    const float length = vector.length();
    if (length < 0.001f) return;
    const float period = qMax(0.02f, m_style.dashLength + m_style.dashGap);
    const int segments = dashed ? qMax(1, int(std::ceil(length / period))) : 1;
    for (int segment = 0; segment < segments; ++segment) {
        const float t0 = dashed ? segment * period / length : 0.0f;
        const float t1 = dashed ? qMin(1.0f, t0 + m_style.dashLength / length) : 1.0f;
        const QVector3D a = trimmedStart + vector * t0;
        const QVector3D b = trimmedStart + vector * t1;
        const float segmentLength = (b - a).length();
        if (segmentLength < 0.001f) continue;
        // Rounded dash caps stay within the original dash interval, preserving gaps.
        const float capDepth = dashed ? qMin(radius, segmentLength * 0.5f) : radius;
        const QVector3D bodyStart = a + (dashed ? axis * capDepth : QVector3D());
        const QVector3D bodyEnd = b - (dashed ? axis * capDepth : QVector3D());
        const float colorStart = t0 + (dashed ? capDepth / length : 0.0f);
        const float colorEnd = t1 - (dashed ? capDepth / length : 0.0f);
        const float bodyLength = (bodyEnd - bodyStart).length();
        if (bodyLength > 0.0001f) {
            QMatrix4x4 model; model.translate((bodyStart + bodyEnd) * 0.5f);
            model.rotate(QQuaternion::rotationTo(QVector3D(0, 0, 1), axis));
            model.scale(radius, radius, bodyLength);
            drawMesh(m_cylinderVao, m_cylinderVertexCount, model, firstColor, true,
                     m_style.bondOpacity * m_bondDisplayOpacity, secondColor, colorStart, colorEnd);
        }
        if (dashed || roundStart)
            drawBondCap(bodyStart, axis, radius, capDepth, firstColor, secondColor, colorStart);
        if ((dashed || roundEnd) && (!dashed || bodyLength > 0.0001f))
            drawBondCap(bodyEnd, axis, radius, capDepth, firstColor, secondColor, colorEnd);
    }
}

void MoleculeView::drawColoredBond(const Bond &bond) {
    if (bond.first < 0 || bond.second < 0 || bond.first >= m_molecule.atoms.size() || bond.second >= m_molecule.atoms.size()) return;
    const Atom &first = m_molecule.atoms[bond.first];
    const Atom &second = m_molecule.atoms[bond.second];
    m_bondDisplayOpacity = qMin(first.displayOpacity, second.displayOpacity) * bond.displayOpacity;
    if (m_depthPass && m_bondDisplayOpacity < 1.0f) return;
    if (!first.visible || !second.visible || (!m_style.showHydrogen && (first.atomicNumber == 1 || second.atomicNumber == 1))) return;
    const QVector3D vector = second.position - first.position;
    const float length = vector.length();
    if (length < 0.001f) return;
    const QVector3D axis = vector / length;
    const QVector3D side = stableBondSideVector(bond, axis);
    auto bondEndpointColor = [this](const Atom &atom) {
        return atomRenderColor(atom);
    };
    QColor firstColor = m_style.bondColor, secondColor = m_style.bondColor;
    if (m_style.bondColorMode == BondColorMode::EndpointAtoms) {
        firstColor = bondEndpointColor(first);
        secondColor = bondEndpointColor(second);
    } else if (m_style.bondColorMode == BondColorMode::MixedAtoms) {
        const QColor a = bondEndpointColor(first), b = bondEndpointColor(second);
        firstColor = secondColor = QColor((a.red() + b.red()) / 2,
                                          (a.green() + b.green()) / 2,
                                          (a.blue() + b.blue()) / 2);
    }
    const float spacing = m_style.bondRadius * m_style.doubleBondSpacing;
    const float firstRadius = atomVisualRadius(first);
    const float secondRadius = atomVisualRadius(second);
    // Equal-radius tube junctions must meet at the sphere center. Trimming them
    // like ball-and-stick exposes the uncapped cylinder ends outside the hub.
    const float endpointInset = m_style.displayMode == DisplayMode::Stick ? 0.0f : 0.72f;

    auto component = [&](float offset, bool dashed, float trim) {
        const QVector3D start = first.position + axis * firstRadius * endpointInset + side * offset;
        const QVector3D end = second.position - axis * secondRadius * endpointInset + side * offset;
        if (QVector3D::dotProduct(end - start, axis) <= 0.001f) return;
        const auto needsCap = [&](float atomRadius) {
            const float inset = atomRadius * endpointInset;
            const float surfaceRadius = std::sqrt(qMax(0.0f, atomRadius * atomRadius - inset * inset));
            return qAbs(offset) + m_style.bondRadius > surfaceRadius;
        };
        // One cylinder and a local color transition avoid an artificial middle seam.
        drawBondSegment(start, end, m_style.bondRadius, firstColor, secondColor, dashed, trim,
                        needsCap(firstRadius), needsCap(secondRadius));
    };

    if (bond.coordination || !m_style.showBondOrders) {
        component(0.0f, false, 0.0f);
    } else if (bond.style == BondStyle::Dashed && qAbs(bond.order - 1.5) > 0.1) {
        component(0.0f, true, 0.0f);
    } else if (qAbs(bond.order - 1.5) <= 0.1) {
        component(-spacing * 0.42f, false, 0.0f);
        component(spacing * 0.42f, true, m_style.bondRadius * 1.4f);
    } else {
        const int order = qBound(1, qRound(bond.order), 3);
        for (int i = 0; i < order; ++i)
            component(order == 1 ? 0.0f : (i - (order - 1) * 0.5f) * spacing, false, 0.0f);
    }
}

} // namespace youngcraft
