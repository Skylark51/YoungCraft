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

int MoleculeView::pickAtom(const QPoint &point) const {
    const QMatrix4x4 vp = viewProjection();
    int picked = -1;
    float bestDistance = 26.0f;
    for (int i = 0; i < m_molecule.atoms.size(); ++i) {
        const Atom &atom = m_molecule.atoms[i];
        if (!atom.visible || (!m_style.showHydrogen && atom.atomicNumber == 1)) continue;
        const QVector3D projected = project(atom.position, vp);
        const float distance = QLineF(point, QPointF(projected.x(), projected.y())).length();
        if (distance < bestDistance) { bestDistance = distance; picked = i; }
    }
    return picked;
}

QVector3D MoleculeView::pointOnDrawingPlane(const QPoint &point, const QVector3D &origin) const {
    bool invertible = false;
    const QMatrix4x4 inverse = viewProjection().inverted(&invertible);
    if (!invertible || width() <= 0 || height() <= 0) return origin;
    const float x = 2.0f * point.x() / width() - 1.0f;
    const float y = 1.0f - 2.0f * point.y() / height();
    QVector4D nearPoint = inverse * QVector4D(x, y, -1.0f, 1.0f);
    QVector4D farPoint = inverse * QVector4D(x, y, 1.0f, 1.0f);
    nearPoint /= nearPoint.w(); farPoint /= farPoint.w();
    const QVector3D start = nearPoint.toVector3D();
    const QVector3D direction = farPoint.toVector3D() - start;
    const QVector3D normal = m_rotation.conjugated().rotatedVector(QVector3D(0, 0, 1));
    const float denominator = QVector3D::dotProduct(direction, normal);
    if (std::abs(denominator) < 1e-8f) return origin;
    return start + direction * (QVector3D::dotProduct(origin - start, normal) / denominator);
}

void MoleculeView::updateDrawingPreview(const QPoint &point, Qt::KeyboardModifiers modifiers) {
    if (!m_drawFromCanvas&&(m_drawStartAtom < 0 || m_drawStartAtom >= m_molecule.atoms.size())) return;
    const QVector3D origin = m_drawFromCanvas?m_drawOrigin:m_molecule.atoms[m_drawStartAtom].position;
    const int startElement=m_drawFromCanvas?m_drawAtomicNumber:m_molecule.atoms[m_drawStartAtom].atomicNumber;
    m_ghostSnapAtom = pickAtom(point);
    if (m_ghostSnapAtom == m_drawStartAtom)
        m_ghostSnapAtom = -1;
    if(m_chainDrawing&&m_ghostSnapAtom>=0&&m_molecule.atoms[m_ghostSnapAtom].autoHydrogen)m_ghostSnapAtom=-1;
    if (m_ghostSnapAtom >= 0) {
        m_ghostPosition = m_molecule.atoms[m_ghostSnapAtom].position;
    } else {
        m_ghostPosition = pointOnDrawingPlane(point, origin);
        if (!(modifiers & Qt::AltModifier)) {
            QVector3D direction = m_ghostPosition - origin;
            if (direction.lengthSquared() < 1e-6f) direction = QVector3D(1, 0, 0);
            if(m_chainDrawing){
                const auto local=m_rotation.rotatedVector(direction);
                constexpr double step=3.14159265358979323846/6;
                const double angle=std::round(std::atan2(local.y(),local.x())/step)*step;
                direction=m_rotation.conjugated().rotatedVector(QVector3D(float(std::cos(angle)),float(std::sin(angle)),0))*direction.length();
                m_ghostPosition=origin+direction;
            }
            m_ghostPosition = MoleculeEditController::snappedPosition(origin, m_ghostPosition,
                startElement,m_drawAtomicNumber,m_drawOrder,false);
        }
    }
    int z=m_ghostSnapAtom>=0?m_molecule.atoms[m_ghostSnapAtom].atomicNumber:m_drawAtomicNumber;
    m_previewOrder=m_drawOrder==0?MoleculeEditController::predictOrder(startElement,z,(m_ghostPosition-origin).length()):m_drawOrder;
    m_ghostActive = true;
    if (m_previewOverlay) m_previewOverlay->update();
}

void MoleculeView::rotateBy(const QPoint &delta) {
    if(m_captureRotation!=-1) {
        const float gain=180.0f/qMax(180,qMin(width(),height()))*m_captureSensitivity;
        QVector3D axis;
        float angle;
        if(m_captureRotation>=0) {
            axis[m_captureRotation]=1.0f;
            angle=(m_captureRotation==0?delta.y():delta.x())*gain;
        } else {
            axis=QVector3D(delta.y(),delta.x(),0);
            angle=axis.length()*gain;
        }
        if(!axis.isNull())m_rotation=(QQuaternion::fromAxisAndAngle(axis.normalized(),angle)*m_rotation).normalized();
    } else {
        const auto yaw=QQuaternion::fromAxisAndAngle(0,1,0,delta.x()*0.55f);
        const auto pitch=QQuaternion::fromAxisAndAngle(1,0,0,delta.y()*0.55f);
        m_rotation=yaw*pitch*m_rotation;
    }
    update();
    if (m_previewOverlay) m_previewOverlay->update();
    emit cameraChanged();
}

void MoleculeView::panBy(const QPoint &delta) {
    const float scale = (m_style.perspective ? m_distance : m_orthoScale) / qMax(1, height()) * 1.8f;
    const QVector3D shift(-delta.x() * scale, delta.y() * scale, 0.0f);
    m_target += m_rotation.conjugated().rotatedVector(shift);
    update();
    if (m_previewOverlay) m_previewOverlay->update();
    emit cameraChanged();
}

int MoleculeView::pickBond(const QPoint &point) const {
    auto vp=viewProjection();int picked=-1;double best=9;
    for(int i=0;i<m_molecule.bonds.size();++i){auto b=m_molecule.bonds[i];if(!m_molecule.atoms[b.first].visible||!m_molecule.atoms[b.second].visible)continue;
        if(!m_style.showHydrogen&&(m_molecule.atoms[b.first].atomicNumber==1||m_molecule.atoms[b.second].atomicNumber==1))continue;
        auto a=project(m_molecule.atoms[b.first].position,vp),c=project(m_molecule.atoms[b.second].position,vp);QPointF u(a.x(),a.y()),v(c.x()-a.x(),c.y()-a.y()),p(point);double den=v.x()*v.x()+v.y()*v.y();if(den<1)continue;double t=((p-u).x()*v.x()+(p-u).y()*v.y())/den;if(t<0.1||t>0.9)continue;double d=QLineF(p,u+v*t).length();if(d<best){best=d;picked=i;}}
    return picked;
}
int MoleculeView::pickSite(const QPoint &point) const {
    auto vp=viewProjection();int picked=-1;double best=16;
    for(int i=0;i<m_molecule.coordinationSites.size();++i){auto s=m_molecule.coordinationSites[i];if(s.occupied||s.center<0||s.center>=m_molecule.atoms.size())continue;auto p=project(m_molecule.atoms[s.center].position+s.direction*2.0f,vp);double d=QLineF(point,QPointF(p.x(),p.y())).length();if(d<best){best=d;picked=i;}}
    return picked;
}
void MoleculeView::mousePressEvent(QMouseEvent *event) {
    setFocus();m_lastMouse=event->pos();m_pressMouse=event->pos();m_dragButton=event->button();m_dragging=false;
    int atom=pickAtom(event->pos());m_drawStartAtom=m_drawingMode&&m_drawingArmed&&event->button()==Qt::LeftButton&&!m_insertPreview?atom:-1;
    m_drawFromCanvas=false;
    if(m_chainDrawing&&m_drawingMode&&m_drawingArmed&&!m_insertPreview&&event->button()==Qt::LeftButton&&!(event->modifiers()&(Qt::ControlModifier|Qt::ShiftModifier))&&atom<0&&pickBond(event->pos())<0){
        if(m_chainAtom>=0)m_drawStartAtom=m_chainAtom;
        else{m_drawFromCanvas=true;m_drawOrigin=pointOnDrawingPlane(event->pos(),m_target);}
    }
    m_dragAtom=m_atomDragMode&&event->button()==Qt::LeftButton&&!(event->modifiers()&(Qt::ShiftModifier|Qt::ControlModifier))?atom:-1;
    m_groupDrag=false;m_groupTranslation={};m_groupRotation={};m_rectangle=false;m_centricDrag=false;m_centricRejected=false;
    if(m_dragAtom>=0){
        m_dragStartPosition=m_molecule.atoms[m_dragAtom].position;
        if(m_transformMode==1||m_transformMode==2){
            QSet<int> branch;
            QVector3D pivot,axis;
            if(MoleculeEditController::rotatableBranch(m_molecule,m_dragAtom,branch,pivot,axis)){
                m_selection.clear();m_selection.atoms=branch;
                m_groupCenter=pivot;m_groupAxis=axis;m_groupDrag=true;
            }
        }else{
            if(m_transformMode>=3)m_selection.connected(m_molecule,m_dragAtom);
            else if(!m_selection.atoms.contains(m_dragAtom))m_selection.click(m_dragAtom,false,false);
            m_groupCenter={};
            for(int i:m_selection.atoms)m_groupCenter+=m_molecule.atoms[i].position;
            m_groupCenter/=qMax(1,int(m_selection.atoms.size()));
            m_groupDrag=true;
        }
    }
    if(m_bondCentric&&m_activeBond>=0&&m_activeBond<m_molecule.bonds.size()&&atom>=0){
        auto b=m_molecule.bonds[m_activeBond];if(atom==b.first||atom==b.second){
            m_centricKind=event->button()==Qt::RightButton?0:event->modifiers()&Qt::ControlModifier?2:1;
            int a=b.first,c=b.second;auto sa=MoleculeEditController::side(m_molecule,a,a,c),sc=MoleculeEditController::side(m_molecule,c,a,c);
            bool moveA=sa.size()<sc.size();if(event->modifiers()&Qt::AltModifier)moveA=!moveA;if(moveA)std::swap(a,c);
            int oa=-1,oc=-1;for(auto bond:m_molecule.bonds){int n=bond.first==a?bond.second:bond.second==a?bond.first:-1;if(n>=0&&n!=c)oa=n;n=bond.first==c?bond.second:bond.second==c?bond.first:-1;if(n>=0&&n!=a)oc=n;}
            m_centricStart=m_centricKind==0?GeometryEditor::distance(m_molecule,a,c):m_centricKind==1&&oa>=0?GeometryEditor::angleDegrees(m_molecule,oa,a,c):m_centricKind==2&&oa>=0&&oc>=0?GeometryEditor::dihedralDegrees(m_molecule,oa,a,c,oc):kNaN;
            m_centricDrag=std::isfinite(m_centricStart)&&!sa.contains(c);m_centricValue=m_centricStart;
            m_centricRejected=!m_centricDrag;
            if(m_centricRejected)m_centricText=sa.contains(c)?"고리 결합: 분자 조각을 안전하게 분리할 수 없습니다.":"각도 / 비틀림 조절에는 인접 원자가 필요합니다.";
            m_centricAtoms=MoleculeEditController::side(m_molecule,c,a,c);m_centricProbe={};
            // Only four probe atoms are passed through the existing geometry functions.
            // The molecular graph and coordinates remain untouched throughout preview.
            for(int i:{oa,a,c,oc}){Atom atom;if(i>=0)atom=m_molecule.atoms[i];m_centricProbe.atoms<<atom;}
            m_centricProbe.setBond(0,1,1);m_centricProbe.setBond(1,2,1);m_centricProbe.setBond(2,3,1);
            m_centricOrigin=m_molecule.atoms[a].position;m_centricTranslation={};m_centricRotation={};
        }
    }
    m_ghostActive=false;if(m_previewOverlay)m_previewOverlay->update();
    if(event->button()==Qt::RightButton||event->button()==Qt::MiddleButton)emit interactionStarted();
}
void MoleculeView::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta=event->pos()-m_lastMouse;
    m_hoverAtom=pickAtom(event->pos());m_hoverBond=m_hoverAtom<0?pickBond(event->pos()):-1;
    if(m_chainDrawing&&m_drawingMode&&m_drawingArmed&&!m_insertPreview&&event->buttons()==Qt::NoButton&&m_chainAtom>=0){
        m_drawStartAtom=m_chainAtom;updateDrawingPreview(event->pos(),event->modifiers());
    }
    if(m_insertPreview){int site=pickSite(event->pos());m_insertDirection={1,0,0};m_insertPosition=pointOnDrawingPlane(event->pos(),m_target);
        if(site>=0){auto s=m_molecule.coordinationSites[site];m_insertPosition=m_molecule.atoms[s.center].position;m_insertDirection=s.direction;}
        else if(m_hoverAtom>=0){m_insertPosition=m_molecule.atoms[m_hoverAtom].position;for(auto b:m_molecule.bonds)if(b.first==m_hoverAtom||b.second==m_hoverAtom){int n=b.first==m_hoverAtom?b.second:b.first;if(m_molecule.atoms[m_hoverAtom].atomicNumber==1){m_insertDirection=(m_insertPosition-m_molecule.atoms[n].position).normalized();m_insertPosition=m_molecule.atoms[n].position;}else if(m_molecule.atoms[n].atomicNumber==1)m_insertDirection=(m_molecule.atoms[n].position-m_insertPosition).normalized();}}
    }
    if(m_centricDrag){int pixels=(event->pos()-m_pressMouse).x();m_centricValue=m_centricStart+pixels*(m_centricKind==0?0.01:0.5);if(m_centricKind==0)m_centricValue=qMax(0.1,m_centricValue);if(m_centricKind==1)m_centricValue=qBound(0.0,m_centricValue,180.0);if(m_centricKind>0&&(event->modifiers()&Qt::ShiftModifier))m_centricValue=std::round(m_centricValue/15)*15;m_centricText=QString("%1: %2 %3").arg(m_centricKind==0?"거리":m_centricKind==1?"각도":"이면각").arg(m_centricValue,0,'f',m_centricKind==0?3:1).arg(m_centricKind==0?"Å":"°");}
    if(m_centricDrag){
        Molecule probe=m_centricProbe;bool ok=false;
        if(m_centricKind==0)ok=GeometryEditor::setDistance(probe,1,2,m_centricValue,GeometryMoveMode::OneSide);
        else if(m_centricKind==1)ok=GeometryEditor::setAngle(probe,0,1,2,m_centricValue,GeometryMoveMode::OneSide);
        else ok=GeometryEditor::setDihedral(probe,0,1,2,3,m_centricValue,GeometryMoveMode::OneSide);
        if(ok){if(m_centricKind==0)m_centricTranslation=probe.atoms[2].position-m_centricProbe.atoms[2].position;
            else{QVector3D before=m_centricProbe.atoms[m_centricKind==1?2:3].position-m_centricOrigin,after=probe.atoms[m_centricKind==1?2:3].position-m_centricOrigin;
                if(m_centricKind==2){auto axis=(m_centricProbe.atoms[2].position-m_centricOrigin).normalized();before-=axis*QVector3D::dotProduct(before,axis);after-=axis*QVector3D::dotProduct(after,axis);}
                m_centricRotation=QQuaternion::rotationTo(before,after);
            }
        }
    }
    else if(m_dragButton==Qt::MiddleButton||((event->buttons()&Qt::RightButton)&&(event->modifiers()&Qt::ControlModifier)))panBy(delta);
    else if((event->buttons()&Qt::RightButton)&&!m_centricRejected)rotateBy(delta);
    if((event->buttons()&Qt::LeftButton)&&(event->pos()-m_pressMouse).manhattanLength()>3&&!m_centricDrag){
        if(m_drawingMode&&m_drawingArmed&&(m_drawStartAtom>=0||m_drawFromCanvas)&&!m_insertPreview&&!(event->modifiers()&(Qt::ControlModifier|Qt::ShiftModifier)))updateDrawingPreview(event->pos(),event->modifiers());
        else if(m_groupDrag){
            if(m_transformMode==1||m_transformMode==2||m_transformMode==4){
                const QPoint total=event->pos()-m_pressMouse;
                if(m_transformMode==2)
                    m_groupRotation=QQuaternion::fromAxisAndAngle(m_groupAxis,total.x()*0.75f);
                else{
                    const auto cameraRotation=QQuaternion::fromEulerAngles(total.y()*0.55f,total.x()*0.55f,0);
                    m_groupRotation=(m_rotation.conjugated()*cameraRotation*m_rotation).normalized();
                }
            }else m_groupTranslation=pointOnDrawingPlane(event->pos(),m_dragStartPosition)-m_dragStartPosition;
        }
        else if(pickAtom(m_pressMouse)<0&&!m_insertPreview)m_rectangle=true;
    }
    m_lastMouse=event->pos();m_dragging=(event->pos()-m_pressMouse).manhattanLength()>3;
    if(m_previewOverlay)m_previewOverlay->update();
}
void MoleculeView::mouseReleaseEvent(QMouseEvent *event) {
    int atom=pickAtom(event->pos()),bond=atom<0?pickBond(event->pos()):-1;
    bool add=event->modifiers()&Qt::ShiftModifier,toggle=event->modifiers()&Qt::ControlModifier;
    if(event->button()==Qt::RightButton&&!m_dragging&&atom<0&&bond<0){
        m_selection.clear();setSelectedAtom(-1);setGeometrySelection({});emit atomPicked(-1);
        if(m_drawingMode)endDrawingChain();
    }
    else if(m_centricDrag&&m_dragging){auto b=m_molecule.bonds[m_activeBond];emit bondManipulated(b.first,b.second,m_centricKind,m_centricValue,event->modifiers()&Qt::AltModifier);}
    else if(m_rectangle){if(!add&&!toggle)m_selection.clear();QRect rect=QRect(m_pressMouse,event->pos()).normalized();auto vp=viewProjection();for(int i=0;i<m_molecule.atoms.size();++i){auto a=m_molecule.atoms[i];if(!a.visible||(!m_style.showHydrogen&&a.atomicNumber==1))continue;auto p=project(a.position,vp);if(rect.contains(QPoint(qRound(p.x()),qRound(p.y()))))m_selection.click(i,true,toggle);}}
    else if(m_groupDrag&&m_dragging){if(m_transformMode==1||m_transformMode==2||m_transformMode==4)emit groupRotated(m_groupRotation,m_groupCenter);else emit groupMoved(m_groupTranslation);}
    else if(m_drawingMode&&m_drawingArmed&&event->button()==Qt::RightButton&&!m_dragging){if(m_chainDrawing&&m_chainAtom>=0)endDrawingChain();else emit deleteRequested(atom,bond);}
    else if(m_drawingMode&&m_drawingArmed&&event->button()==Qt::LeftButton&&!add&&!toggle){
        int site=pickSite(event->pos());
        if(m_insertPreview&&site>=0){auto s=m_molecule.coordinationSites[site];emit attachmentClicked(s.center,s.direction);}
        else if(m_drawFromCanvas&&m_dragging){updateDrawingPreview(event->pos(),event->modifiers());const auto start=m_drawOrigin,end=m_ghostPosition;if(m_ghostSnapAtom>=0)emit bondDrawn(m_ghostSnapAtom,-1,start);else emit bondStrokeDrawn(start,end);}
        else if(m_drawStartAtom>=0&&m_dragging){updateDrawingPreview(event->pos(),event->modifiers());emit bondDrawn(m_drawStartAtom,m_ghostSnapAtom,m_ghostPosition);}
        else if(m_chainDrawing&&m_chainAtom>=0&&!m_insertPreview&&!m_dragging&&bond<0&&atom!=m_chainAtom){
            m_drawFromCanvas=false;m_drawStartAtom=m_chainAtom;updateDrawingPreview(event->pos(),event->modifiers());
            emit bondDrawn(m_drawStartAtom,m_ghostSnapAtom,m_ghostPosition);
        }
        else if(!m_dragging){if(atom>=0){setSelectedAtom(atom);emit atomEditRequested(atom);if(m_selectedAtom>=0)emit atomPicked(m_selectedAtom);}else if(bond>=0&&!m_insertPreview)emit bondEditRequested(bond);else emit canvasClicked(pointOnDrawingPlane(event->pos(),m_target));}
    }else if(event->button()==Qt::LeftButton&&!m_dragging){
        if(m_bondCentric&&bond>=0){m_activeBond=bond;m_centricText="끝점 드래그: 각도 · 오른쪽: 길이 · Ctrl: 비틀림";}
        else{m_selection.click(atom,add,toggle);if(bond>=0){if(toggle&&m_selection.bonds.contains(bond))m_selection.bonds.remove(bond);else m_selection.bonds.insert(bond);}setSelectedAtom(atom);emit atomPicked(atom);}
    }
    m_groupTranslation={};m_dragButton=Qt::NoButton;m_ghostActive=false;m_drawStartAtom=-1;m_drawFromCanvas=false;m_dragAtom=-1;m_groupDrag=false;m_rectangle=false;m_centricDrag=false;m_centricRejected=false;
    update();if(m_previewOverlay)m_previewOverlay->update();
    if(event->button()==Qt::RightButton||event->button()==Qt::MiddleButton)emit interactionFinished();
}
void MoleculeView::mouseDoubleClickEvent(QMouseEvent *event) {
    if(event->button()!=Qt::LeftButton)return;int atom=pickAtom(event->pos());
    if(atom>=0){m_selection.connected(m_molecule,atom);m_selectedAtom=atom;m_target=m_molecule.atoms[atom].position;update();if(m_previewOverlay)m_previewOverlay->update();emit atomPicked(atom);emit cameraChanged();}
}

void MoleculeView::wheelEvent(QWheelEvent *event) {
    const float factor = std::pow(0.88f, event->angleDelta().y() / 120.0f);
    if (m_style.perspective) m_distance = qBound(0.2f, m_distance * factor, 5000.0f);
    else m_orthoScale = qBound(0.2f, m_orthoScale * factor, 5000.0f);
    update();
    if(m_previewOverlay)m_previewOverlay->update();
    emit cameraChanged();
}

} // namespace youngcraft
