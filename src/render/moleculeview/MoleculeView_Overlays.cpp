#include "render/MoleculeView.h"

#include "render/IsosurfaceBuilder.h"

#include <QApplication>
#include <QClipboard>
#include <QLineF>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QSurfaceFormat>
#include <QThread>
#include <QWheelEvent>

#include <cmath>
#include <iterator>
#include <limits>

namespace youngcraft {

QVector3D MoleculeView::project(const QVector3D &position, const QMatrix4x4 &vp) const {
    const QVector4D clip = vp * QVector4D(position, 1.0f);
    if (qFuzzyIsNull(clip.w())) return {-10000, -10000, 2};
    const QVector3D ndc = (clip / clip.w()).toVector3D();
    return {(ndc.x() * 0.5f + 0.5f) * float(renderLogicalSize().width()), (1.0f - (ndc.y() * 0.5f + 0.5f)) * float(renderLogicalSize().height()), ndc.z()};
}

bool MoleculeView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_previewOverlay && event->type() == QEvent::Paint) {
        drawEditorPreview();
        // Keep measurements visible when Qt recomposites the OpenGL child widget.
        QPainter painter(m_previewOverlay);
        drawGeometryMeasurements(viewProjection(), &painter);
        return true;
    }
    return QOpenGLWidget::eventFilter(watched, event);
}

void MoleculeView::drawEditorPreview() {
    const auto vp=viewProjection();QPainter painter(m_previewOverlay);painter.setRenderHint(QPainter::Antialiasing,true);
    auto point=[&](const QVector3D&p){auto v=project(p,vp);return QPointF(v.x(),v.y());};
    painter.setBrush(Qt::NoBrush);painter.setPen(QPen(m_style.selectionColor,2));
    for(int i:m_selection.atoms)if(i>=0&&i<m_molecule.atoms.size())painter.drawEllipse(point(m_molecule.atoms[i].position),13,13);
    for(int i:m_selection.bonds)if(i>=0&&i<m_molecule.bonds.size()){auto b=m_molecule.bonds[i];painter.drawLine(point(m_molecule.atoms[b.first].position),point(m_molecule.atoms[b.second].position));}
    if(m_hoverAtom>=0&&m_hoverAtom<m_molecule.atoms.size()){painter.setPen(QPen((m_dragButton==Qt::RightButton&&m_drawingMode?QColor(240,70,70,220):QColor(255,190,80,170)),2));painter.drawEllipse(point(m_molecule.atoms[m_hoverAtom].position),16,16);}
    if(m_hoverBond>=0&&m_hoverBond<m_molecule.bonds.size()){auto b=m_molecule.bonds[m_hoverBond];painter.setPen(QPen((m_dragButton==Qt::RightButton&&m_drawingMode?QColor(240,70,70,220):QColor(255,190,80,150)),5));painter.drawLine(point(m_molecule.atoms[b.first].position),point(m_molecule.atoms[b.second].position));}
    if(m_rectangle){painter.setPen(QPen(m_style.selectionColor,1,Qt::DashLine));painter.setBrush(QColor(80,160,250,30));painter.drawRect(QRect(m_pressMouse,m_lastMouse).normalized());painter.setBrush(Qt::NoBrush);}
    if(m_groupDrag&&m_dragging){
        auto ghost=[&](int i){auto p=m_molecule.atoms[i].position;if(m_selection.atoms.contains(i))p=m_groupCenter+m_groupRotation.rotatedVector(p-m_groupCenter)+m_groupTranslation;return point(p);};
        painter.setPen(QPen(m_style.selectionColor,2,Qt::DashLine));
        for(auto b:m_molecule.bonds)if(m_selection.atoms.contains(b.first)||m_selection.atoms.contains(b.second))painter.drawLine(ghost(b.first),ghost(b.second));
        for(int i:m_selection.atoms)painter.drawEllipse(ghost(i),8,8);
    }
    painter.setPen(QPen(QColor(80,130,210),2));
    for(const auto &a:m_molecule.atoms)if(a.optimizationFixed&&a.visible&&(m_style.showHydrogen||a.atomicNumber!=1)){auto p=point(a.position)+QPointF(10,-10);painter.drawRect(QRectF(p,QSizeF(6,6)));}
    painter.setPen(QPen(QColor(120,180,230,180),1,Qt::DashLine));
    for(const auto &site:m_molecule.coordinationSites)if(!site.occupied&&site.center>=0&&site.center<m_molecule.atoms.size()){auto origin=m_molecule.atoms[site.center].position;auto end=point(origin+site.direction*2);painter.drawLine(point(origin),end);painter.drawEllipse(end,6,6);}
    for(const auto &h:m_molecule.hapticContacts){if(h.metal<0||h.metal>=m_molecule.atoms.size())continue;QVector3D c;for(int i:h.atoms)c+=m_molecule.atoms[i].position;if(!h.atoms.isEmpty()){c/=h.atoms.size();painter.drawLine(point(m_molecule.atoms[h.metal].position),point(c));painter.drawText(point(c)+QPointF(8,0),h.name);}}
    if(m_bondCentric&&m_activeBond>=0&&m_activeBond<m_molecule.bonds.size()){auto b=m_molecule.bonds[m_activeBond];auto a=point(m_molecule.atoms[b.first].position),c=point(m_molecule.atoms[b.second].position);painter.setPen(QPen(m_style.selectionColor,3));painter.drawLine(a,c);painter.setPen(QPen(m_style.selectionColor,1,Qt::DashLine));painter.drawEllipse((a+c)/2,32,18);painter.drawText((a+c)/2+QPointF(12,-25),m_centricText);}
    if(m_centricDrag&&m_dragging){painter.setPen(QPen(m_style.selectionColor,2,Qt::DashLine));
        auto ghost=[&](int i){auto p=m_molecule.atoms[i].position;if(m_centricAtoms.contains(i))p=m_centricOrigin+m_centricRotation.rotatedVector(p-m_centricOrigin)+m_centricTranslation;return point(p);};
        for(auto b:m_molecule.bonds)if(m_centricAtoms.contains(b.first)||m_centricAtoms.contains(b.second))painter.drawLine(ghost(b.first),ghost(b.second));
        for(int i:m_centricAtoms)painter.drawEllipse(ghost(i),7,7);
    }
    if(m_insertPreview){painter.setPen(QPen(QColor(80,170,245,190),2,Qt::DashLine));
        if(m_templatePreview>=0){for(auto dir:FragmentLibrary::directions(m_templatePreview)){auto p=point(m_insertPosition+dir*2);painter.drawLine(point(m_insertPosition),p);painter.drawEllipse(p,5,5);}}
        else{auto q=QQuaternion::rotationTo(m_insertFragment.direction,m_insertDirection);auto pos=[&](int i){return point(m_insertPosition+q.rotatedVector(m_insertFragment.atoms[i].position-m_insertFragment.anchor));};for(auto b:m_insertFragment.bonds)if(m_insertFragment.atoms[b.first].atomicNumber>0&&m_insertFragment.atoms[b.second].atomicNumber>0)painter.drawLine(pos(b.first),pos(b.second));for(int i=0;i<m_insertFragment.atoms.size();++i)if(m_insertFragment.atoms[i].atomicNumber>0)painter.drawEllipse(pos(i),5,5);painter.drawText(point(m_insertPosition)+QPointF(12,-15),m_insertFragment.name);}
    }
    if(!m_ghostActive||(!m_drawFromCanvas&&(m_drawStartAtom<0||m_drawStartAtom>=m_molecule.atoms.size())))return;
    auto start=point(m_drawFromCanvas?m_drawOrigin:m_molecule.atoms[m_drawStartAtom].position),end=point(m_ghostPosition);
    painter.setPen(QPen(m_ghostSnapAtom>=0?m_style.selectionColor:elementColor(m_drawAtomicNumber),3,Qt::DashLine,Qt::RoundCap));painter.drawLine(start,end);
    painter.setBrush(QColor(elementColor(m_drawAtomicNumber).red(),elementColor(m_drawAtomicNumber).green(),elementColor(m_drawAtomicNumber).blue(),170));painter.drawEllipse(end,9,9);
    const auto origin=m_drawFromCanvas?m_drawOrigin:m_molecule.atoms[m_drawStartAtom].position;
    painter.setPen(m_style.labelColor);painter.drawText(end+QPointF(12,-8),QString("%1 · %2 · %3 Å").arg(m_ghostSnapAtom>=0?m_molecule.atoms[m_ghostSnapAtom].element:elementSymbol(m_drawAtomicNumber)).arg(m_previewOrder).arg((m_ghostPosition-origin).length(),0,'f',2));
}

void MoleculeView::drawLabels(const QMatrix4x4 &vp) {
    const int selectedAtom = m_viewportOnlyCapture ? -1 : m_selectedAtom;
    if (!m_style.showLabels && !m_style.showProperties && selectedAtom < 0 && m_geometrySelection.isEmpty()) return;
    QPainter painter; beginOverlayPainter(painter);
    painter.setRenderHint(QPainter::TextAntialiasing, m_style.antiAliasing);
    QFont font = painter.font();
    font.setPointSizeF(m_style.labelSize);
    painter.setFont(font);
    for (int i = 0; i < m_molecule.atoms.size(); ++i) {
        const Atom &atom = m_molecule.atoms[i];
        if (!atom.visible || (!m_style.showHydrogen && atom.atomicNumber == 1)) continue;
        const QVector3D screen = project(atom.position, vp);
        if (screen.z() < -1.0f || screen.z() > 1.0f) continue;
        if (m_style.showLabels || i == selectedAtom || m_geometrySelection.contains(i)) {
            QFont numberFont = font;
            numberFont.setBold(true);
            QPainterPath number;
            number.addText(QPointF(), numberFont, QString::number(atom.index > 0 ? atom.index : i + 1));
            const QRectF bounds = number.boundingRect();
            const QVector3D edge = project(atom.position + m_rotation.conjugated().rotatedVector(QVector3D(atomVisualRadius(atom), 0, 0)), vp);
            const double radius = QLineF(QPointF(screen.x(), screen.y()), QPointF(edge.x(), edge.y())).length();
            const double scale = qMin(1.0, qMin(radius * 1.6 / qMax(1.0, bounds.width()), radius * 1.4 / qMax(1.0, bounds.height())));
            QTransform transform;
            transform.translate(screen.x(), screen.y());
            transform.scale(scale, scale);
            transform.translate(-bounds.center().x(), -bounds.center().y());
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QColor(20, 24, 30, 220), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            const QPainterPath centeredNumber = transform.map(number);
            painter.drawPath(centeredNumber);
            painter.fillPath(centeredNumber, Qt::white);
        }
        QString label;
        if (m_style.showProperties) {
            if (std::isfinite(atom.charge)) label += QStringLiteral("\nq=%1").arg(atom.charge, 0, 'f', 3);
            if (std::isfinite(atom.spinDensity)) label += QStringLiteral("\nρ=%1").arg(atom.spinDensity, 0, 'f', 3);
        }
        if (label.isEmpty()) continue;
        const QString text = label.trimmed();
        const QSizeF textSize = QFontMetricsF(font).size(Qt::TextExpandTabs, text);
        QRectF box(screen.x() + 6.0, screen.y() - textSize.height() - 4.0,
                   textSize.width() + 10.0, textSize.height() + 6.0);
        box.moveLeft(qBound(0.0, box.left(), qMax(0.0, renderLogicalSize().width() - box.width())));
        box.moveTop(qBound(0.0, box.top(), qMax(0.0, renderLogicalSize().height() - box.height())));
        QColor background = m_style.background;
        background.setAlpha(235);
        QColor border = m_style.labelColor;
        border.setAlpha(90);
        painter.setPen(i == selectedAtom ? m_style.selectionColor : border);
        painter.setBrush(background);
        painter.drawRoundedRect(box, 3.0, 3.0);
        painter.setPen(m_style.labelColor);
        painter.drawText(box.adjusted(5, 3, -5, -3), Qt::AlignLeft | Qt::AlignTop, text);
    }
}

void MoleculeView::drawVibrationVectors(const QMatrix4x4 &vp) {
    if (m_frequencyDisplacements.isEmpty()) return;
    QPainter painter; beginOverlayPainter(painter);
    painter.setRenderHint(QPainter::Antialiasing, m_style.antiAliasing);
    QPen pen(m_style.vectorColor, m_style.vectorWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    const int count = qMin(m_molecule.atoms.size(), m_frequencyDisplacements.size());
    for (int i = 0; i < count; ++i) {
        const QVector3D displacement = m_frequencyDisplacements[i];
        if (displacement.length() < m_style.vectorThreshold) continue;
        const QVector3D start3 = i < m_frequencyBase.size() ? m_frequencyBase[i] : m_molecule.atoms[i].position;
        const float vectorScale = qBound(0.05f, m_style.vectorScale * float(m_vibrationVectorScaleFactor), 20.0f);
        const QVector3D end3 = start3 + displacement * vectorScale;
        const QVector3D start = project(start3, vp);
        const QVector3D end = project(end3, vp);
        const QLineF line(QPointF(start.x(), start.y()), QPointF(end.x(), end.y()));
        if (line.length() < 2.0) continue;
        painter.drawLine(line);
        const double angle = std::atan2(-line.dy(), line.dx());
        const double arrow = qBound(4.0, double(m_style.vectorWidth * 3.5f), 14.0);
        const QPointF tip = line.p2();
        painter.drawLine(tip, tip - QPointF(std::cos(angle + 0.55) * arrow, -std::sin(angle + 0.55) * arrow));
        painter.drawLine(tip, tip - QPointF(std::cos(angle - 0.55) * arrow, -std::sin(angle - 0.55) * arrow));
    }
}

void MoleculeView::drawGeometryMeasurements(const QMatrix4x4 &vp, QPainter *overlayPainter) {
    if (m_geometrySelection.isEmpty()) return;
    QPainter capturePainter;
    if (!overlayPainter) beginOverlayPainter(capturePainter);
    QPainter &painter = overlayPainter ? *overlayPainter : capturePainter;
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
    QVector<QPointF> points;for(int index:m_geometrySelection){if(index<0||index>=m_molecule.atoms.size())return;const QVector3D p=project(m_molecule.atoms[index].position,vp);points.push_back({p.x(),p.y()});}
    for(int i=0;i<points.size();++i){const Atom &atom=m_molecule.atoms[m_geometrySelection[i]];const auto edge=project(atom.position+m_rotation.conjugated().rotatedVector(QVector3D(atomVisualRadius(atom),0,0)),vp);const double radius=QLineF(points[i],QPointF(edge.x(),edge.y())).length()+3;painter.setPen(QPen(QColor(230,165,35),2));painter.setBrush(Qt::NoBrush);painter.drawEllipse(points[i],radius,radius);}
    auto style=[&](int kind){MeasurementTextStyle value=m_measurementStyles[kind];if(value.fontFamily.isEmpty())value.fontFamily=painter.font().family();QFont font(value.fontFamily,value.pointSize);font.setBold(value.bold);painter.setFont(font);painter.setPen(QPen(value.color,1.5,Qt::DashLine,Qt::RoundCap,Qt::RoundJoin));};
    // Place callouts in nearby free space rather than on the measured geometry.
    QVector<QRectF> atomBounds;
    QVector<QPointF> atomCenters(m_molecule.atoms.size());
    QVector<bool> visible(m_molecule.atoms.size(), false);
    for (int i = 0; i < m_molecule.atoms.size(); ++i) {
        const Atom &atom = m_molecule.atoms[i];
        if (!atom.visible || (!m_style.showHydrogen && atom.atomicNumber == 1)) continue;
        const auto screen = project(atom.position, vp);
        if (screen.z() < -1.0f || screen.z() > 1.0f) continue;
        const QPointF center(screen.x(), screen.y());
        const auto edge = project(atom.position + m_rotation.conjugated().rotatedVector(
            QVector3D(atomVisualRadius(atom), 0, 0)), vp);
        const double radius = QLineF(center, QPointF(edge.x(), edge.y())).length() + 6;
        atomBounds.append(QRectF(center - QPointF(radius, radius), QSizeF(2*radius, 2*radius)));
        atomCenters[i] = center;
        visible[i] = true;
    }
    QVector<QLineF> bondLines;
    for (const auto &bond : m_molecule.bonds) {
        if (bond.first >= 0 && bond.second >= 0 && bond.first < visible.size() &&
            bond.second < visible.size() && visible[bond.first] && visible[bond.second])
            bondLines.append(QLineF(atomCenters[bond.first], atomCenters[bond.second]));
    }
    // Include measurement guides even when the measured atoms have no bond.
    for (int i = 1; i < points.size(); ++i) bondLines.append(QLineF(points[i-1], points[i]));
    QVector<QRectF> placedLabels;
    auto label = [&](const QPointF &position, const QString &text, int kind = 0) {
        const QFontMetricsF metrics(painter.font());
        const QSizeF size(metrics.horizontalAdvance(text) + 10, metrics.height() + 6);
        const QSizeF viewport = renderLogicalSize();
        const QRectF viewportBounds(QPointF(4, 4), QSizeF(qMax(0.0, viewport.width()-8), qMax(0.0, viewport.height()-8)));
        QRectF box;
        double bestScore = std::numeric_limits<double>::max();
        const QLineF axis(points[0], points[1]);
        const double baseAngle = std::atan2(axis.dy(), axis.dx()) + 1.5707963267948966;
        for (double extra : {0.0, 22.0, 44.0, 72.0, 110.0}) {
            for (int direction = 0; direction < 16; ++direction) {
                const double angle = baseAngle + direction * 0.39269908169872414;
                const QPointF unit(std::cos(angle), std::sin(angle));
                const double offset = std::abs(unit.x()) * size.width()/2 +
                                      std::abs(unit.y()) * size.height()/2 + 14 + extra;
                QRectF candidate(QPointF(), size);
                candidate.moveCenter(position + unit * offset);
                candidate.moveLeft(qBound(viewportBounds.left(), candidate.left(),
                                          qMax(viewportBounds.left(), viewportBounds.right()-size.width())));
                candidate.moveTop(qBound(viewportBounds.top(), candidate.top(),
                                         qMax(viewportBounds.top(), viewportBounds.bottom()-size.height())));
                double score = QLineF(position, candidate.center()).length();
                const QRectF clearance = candidate.adjusted(-3, -3, 3, 3);
                for (const auto &atom : atomBounds) {
                    const auto overlap = clearance.intersected(atom);
                    score += overlap.width() * overlap.height() * 100;
                }
                for (const auto &placed : placedLabels) {
                    const auto overlap = clearance.intersected(placed);
                    score += overlap.width() * overlap.height() * 200;
                }
                const QLineF edges[] = {QLineF(clearance.topLeft(), clearance.topRight()),
                    QLineF(clearance.topRight(), clearance.bottomRight()),
                    QLineF(clearance.bottomRight(), clearance.bottomLeft()),
                    QLineF(clearance.bottomLeft(), clearance.topLeft())};
                for (const auto &bond : bondLines) {
                    bool crosses = clearance.contains(bond.p1()) || clearance.contains(bond.p2());
                    for (const auto &edge : edges)
                        crosses |= bond.intersects(edge, nullptr) == QLineF::BoundedIntersection;
                    if (crosses) score += 1500;
                }
                if (score < bestScore) { bestScore = score; box = candidate; }
            }
        }
        placedLabels.append(box);
        painter.save();
        const QColor textColor = painter.pen().color();
        QColor guideColor = textColor;
        guideColor.setAlpha(100);
        painter.setPen(kind == 0
            ? QPen(QColor(220, 35, 50), 1.2, Qt::DashLine, Qt::RoundCap)
            : QPen(guideColor, 1.0, Qt::SolidLine));
        const QPointF end(qBound(box.left(), position.x(), box.right()),
                          qBound(box.top(), position.y(), box.bottom()));
        painter.drawLine(position, end);
        painter.setPen(Qt::NoPen);
        QColor background = m_style.background;
        background.setAlpha(200);
        painter.setBrush(background);
        painter.drawRoundedRect(box, 3, 3);
        painter.setPen(textColor);
        painter.drawText(box, Qt::AlignCenter, text);
        painter.restore();
    };
    if (m_measurementVisible[0] && points.size() >= 2) {
        style(0);
        // Soft pink glow follows the same dash spacing at every stroke width.
        // Stop at sphere edges so the overlay leaves atom numbers readable.
        QLineF scanLine(points[0], points[1]);
        double inset[2]{};
        for (int i = 0; i < 2; ++i) {
            const Atom &atom = m_molecule.atoms[m_geometrySelection[i]];
            const auto edge = project(atom.position + m_rotation.conjugated().rotatedVector(
                QVector3D(atomVisualRadius(atom), 0, 0)), vp);
            inset[i] = QLineF(points[i], QPointF(edge.x(), edge.y())).length() + 3;
        }
        if (scanLine.length() > inset[0] + inset[1]) {
            const QPointF unit = (scanLine.p2() - scanLine.p1()) / scanLine.length();
            scanLine = QLineF(scanLine.p1() + unit*inset[0], scanLine.p2() - unit*inset[1]);
            painter.save();
            for (const auto &stroke : {QPair<qreal,int>(8,25), {5,55}, {2,230}}) {
                QPen neon(QColor(255,55,175,stroke.second), stroke.first, Qt::CustomDashLine, Qt::RoundCap);
                neon.setDashPattern({6.0/stroke.first, 5.0/stroke.first});
                painter.setPen(neon);
                painter.drawLine(scanLine);
            }
            painter.restore();
        }
        label((points[0]+points[1])/2.0, QStringLiteral("%1 Å").arg(
            GeometryEditor::distance(m_molecule,m_geometrySelection[0],m_geometrySelection[1]),0,'f',2));
    }
    if(m_measurementVisible[1]&&points.size()>=3){style(1);painter.drawLine(points[1],points[0]);painter.drawLine(points[1],points[2]);const QPointF center=(points[0]+points[1]+points[2])/3.0;label(center,QStringLiteral("%1°").arg(GeometryEditor::angleDegrees(m_molecule,m_geometrySelection[0],m_geometrySelection[1],m_geometrySelection[2]),0,'f',2),1);}
    if(m_measurementVisible[2]&&points.size()>=4){style(2);painter.drawPolyline(points.constData(),points.size());const QPointF center=(points[1]+points[2])/2.0;label(center,QStringLiteral("%1°").arg(GeometryEditor::dihedralDegrees(m_molecule,m_geometrySelection[0],m_geometrySelection[1],m_geometrySelection[2],m_geometrySelection[3]),0,'f',2),2);}
}

} // namespace youngcraft
