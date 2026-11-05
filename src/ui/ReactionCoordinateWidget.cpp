#include "ui/ReactionCoordinateWidget.h"
#include "io/FileDispatcher.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QInputDialog>
#include <QFileInfo>
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QImageReader>
#include <QJsonArray>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace youngcraft {
namespace {
constexpr double pi=3.141592653589793;
double smooth(double t) { return t*t*(3-2*t); }
double energy(const MechanismStep &step) { return step.presentation["energy"].toDouble(); }
bool hasEnergy(const MechanismStep &step) {
    const auto value=step.presentation["energy"];
    return value.isDouble()&&std::isfinite(value.toDouble());
}
double coordinate(const MechanismStep &step, int index) { return step.presentation["coordinate"].toDouble(index); }
QRectF fitImage(const QImage &image, QRectF bounds) {
    if(image.isNull())return bounds;
    const QSizeF size=QSizeF(image.size()).scaled(bounds.size(),Qt::KeepAspectRatio);
    return QRectF(bounds.center()-QPointF(size.width()/2,size.height()/2),size);
}
QImage readImage(const QJsonObject &object) {
    const auto bytes=QByteArray::fromBase64(object["image"].toString().toLatin1());
    QBuffer buffer;buffer.setData(bytes);buffer.open(QIODevice::ReadOnly);QImageReader reader(&buffer);
    const auto size=reader.size();
    if(!size.isValid()||size.width()>4096||size.height()>4096)return {};
    return reader.read();
}
QString encodeImage(QImage image) {
    if(image.width()>1600||image.height()>1200)image=image.scaled(1600,1200,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");return QString::fromLatin1(bytes.toBase64());
}
QPointF point(const QJsonArray &a,int offset=0) { return {qBound(0.,a[offset].toDouble(),1.),qBound(0.,a[offset+1].toDouble(),1.)}; }
void annotations(QPainter &p,const QJsonArray &items,const QRectF &rect,double u,bool editing=false) {
    auto screen=[&](QPointF pt){return rect.topLeft()+QPointF(pt.x()*rect.width(),pt.y()*rect.height());};
    for(const auto &value:items) {
        const auto obj=value.toObject();const auto a=obj["points"].toArray();if(a.size()!=4)continue;
        const auto from=screen(point(a)),to=screen(point(a,2));const QString type=obj["type"].toString();
        const double start=qBound(0.,obj["start"].toDouble(),1.);
        const double end=qBound(start,obj["end"].toDouble(1),1.);
        if(!editing&&(u<start||u>end))continue;
        const double progress=editing?.5:qBound(0.,(u-start)/qMax(.001,end-start),1.);
        p.save();
        if(type=="form"||type=="break") {
            const bool forming=type=="form";QColor color=forming?QColor(19,184,119):QColor(242,126,45);
            color.setAlphaF(editing?.8:(forming?.15+.75*smooth(progress):.9-.75*smooth(progress)));
            QPen pen(color,editing?8:8+5*std::sin(pi*progress),forming?Qt::SolidLine:Qt::DashLine,Qt::RoundCap);
            p.setPen(pen);p.drawLine(from,to);
        } else {
            const QPointF control=(from+to)/2+QPointF(0,-rect.height()*.22);
            QPainterPath path(from);path.quadTo(control,to);p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor(100,100,180,140),1.5,Qt::DashLine));p.drawPath(path);
            const QPointF center=path.pointAtPercent(smooth(progress));
            p.setPen(Qt::NoPen);
            if(type=="orbital") {
                const double angle=std::atan2(to.y()-from.y(),to.x()-from.x())*180/pi;
                p.translate(center);p.rotate(angle);p.setBrush(QColor(80,125,240,120));p.drawEllipse(QRectF(-27,-12,27,24));
                p.setBrush(QColor(245,114,140,130));p.drawEllipse(QRectF(0,-12,27,24));
            } else {
                QRadialGradient gradient(center,28);gradient.setColorAt(0,QColor(125,91,240,210));gradient.setColorAt(1,QColor(125,91,240,0));
                p.setBrush(gradient);p.drawEllipse(center,28,28);p.setBrush(QColor(85,57,190));p.drawEllipse(center+QPointF(-4,0),2.5,2.5);p.drawEllipse(center+QPointF(4,0),2.5,2.5);
            }
        }
        p.restore();
    }
}
class AnnotationCanvas : public QWidget {
public:
    QImage image;QJsonArray marks;QString type="form";QPointF first;bool pending=false;
    std::function<void()> changed;
    explicit AnnotationCanvas(QWidget *parent=nullptr):QWidget(parent){setMinimumSize(320,230);setCursor(Qt::CrossCursor);setObjectName("ReactionAnnotationCanvas");}
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);p.fillRect(rect(),QColor(249,250,254));p.setRenderHint(QPainter::Antialiasing);
        if(image.isNull()){p.drawText(rect(),Qt::AlignCenter,QStringLiteral("먼저 선택 단계에 2D 그림을 삽입하세요"));return;}
        const auto r=fitImage(image,QRectF(rect()).adjusted(12,12,-12,-12));p.drawImage(r,image);annotations(p,marks,r,.5,true);
        if(pending){p.setBrush(QColor(100,85,230));p.setPen(Qt::NoPen);p.drawEllipse(r.topLeft()+QPointF(first.x()*r.width(),first.y()*r.height()),5,5);}
    }
    void mousePressEvent(QMouseEvent *event) override {
        if(image.isNull())return;
        if(event->button()==Qt::RightButton){pending=false;update();return;}
        if(event->button()!=Qt::LeftButton)return;
        const auto r=fitImage(image,QRectF(rect()).adjusted(12,12,-12,-12));if(!r.contains(event->position()))return;
        const auto pt=event->position()-r.topLeft();QPointF normalized(pt.x()/r.width(),pt.y()/r.height());
        if(!pending){first=normalized;pending=true;}else{marks.append(QJsonObject{{"type",type},{"points",QJsonArray{first.x(),first.y(),normalized.x(),normalized.y()}}});pending=false;if(changed)changed();}update();
    }
};
}

ReactionCoordinateWidget::ReactionCoordinateWidget(QVector<MechanismStep> *steps,QWidget *parent):QWidget(parent),m_steps(steps) {
    setObjectName("ReactionCoordinatePreview");setMinimumHeight(120);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);hide();
}
bool ReactionCoordinateWidget::ready() const {
    if(m_steps->size()<2||m_images.size()!=m_steps->size())return false;
    double previous=-std::numeric_limits<double>::infinity();
    for(int i=0;i<m_steps->size();++i){const auto &s=(*m_steps)[i];const double x=coordinate(s,i);
        if(!std::isfinite(x)||x<=previous)return false;
        previous=x;}
    return hasImages()||hasEnergies();
}
bool ReactionCoordinateWidget::hasImages() const {
    return std::any_of(m_images.cbegin(),m_images.cend(),[](const QImage &image){return !image.isNull();});
}
bool ReactionCoordinateWidget::hasEnergies() const {
    return std::any_of(m_steps->cbegin(),m_steps->cend(),[](const MechanismStep &step){return hasEnergy(step);});
}
void ReactionCoordinateWidget::refresh() {m_images.clear();for(const auto &s:*m_steps)m_images.push_back(readImage(s.presentation));setVisible(active());update();}
void ReactionCoordinateWidget::setEnabledPresentation(bool enabled){m_enabled=enabled;setVisible(active());update();emit configurationChanged();}
void ReactionCoordinateWidget::setTime(double seconds){m_time=seconds;update();}
QVector<double> ReactionCoordinateWidget::stageTimes() const {
    QVector<double> times;if(m_steps->isEmpty())return times;
    double time=(*m_steps)[0].frames.size()>1?(*m_steps)[0].travelSeconds():0;times.push_back(time);
    for(int i=1;i<m_steps->size();++i){
        time+=(*m_steps)[i-1].holdSeconds()+(*m_steps)[i].travelSeconds();
        times.push_back(time);
    }return times;
}
ReactionCoordinateWidget::Position ReactionCoordinateWidget::position(double seconds) const {
    Position result;result.time=seconds;const auto times=stageTimes();if(times.isEmpty())return result;
    if(times.size()<2||seconds<=times.first()+m_steps->first().holdSeconds())return result;
    int to=1;
    while(to<times.size()-1&&seconds>times[to]+(*m_steps)[to].holdSeconds())++to;
    result.from=to-1;result.to=to;
    const double departure=times[to-1]+(*m_steps)[to-1].holdSeconds();
    const double length=times[to]-departure;
    double u=qBound(0.,(seconds-departure)/length,1.);
    const auto settings=(*m_steps)[to].presentation;
    if(settings["impactBarrier"].toBool()&&seconds<times[to]) {
        const double hit=qBound(.1,settings["impactAt"].toDouble(.38),.7);
        const double resume=hit+.24;
        const double recoil=qMin(hit,qBound(0.,settings["impactRecoil"].toDouble(.15),.5));
        if(u>=hit&&u<resume) {
            result.collision=true;
            const double flash=qBound(.05,settings["impactFlash"].toDouble(.65),3.);
            result.impact=(u-hit)*length/qMin(.24*length,flash);
            u=hit-recoil*smooth((u-hit)/.24);
        } else if(u>=resume) {
            u=hit-recoil+(1-hit+recoil)*smooth((u-resume)/(1-resume));
        }
        result.time=departure+u*length;
    }
    result.fraction=u;return result;
}
double ReactionCoordinateWidget::mappedTime(double seconds) const {return position(seconds).time;}
double ReactionCoordinateWidget::timeForMapped(double seconds) const {
    const auto times=stageTimes();
    for(int i=1;i<times.size();++i) {
        if(seconds>times[i])continue;
        const double departure=times[i-1]+(*m_steps)[i-1].holdSeconds();
        const auto settings=(*m_steps)[i].presentation;
        const double hit=qBound(.1,settings["impactAt"].toDouble(.38),.7);
        const double length=times[i]-departure;
        if(!settings["impactBarrier"].toBool()||seconds<=departure+hit*length||seconds>=times[i])return seconds;
        double left=departure+(hit+.24)*length,right=times[i];
        for(int j=0;j<32;++j) {
            const double middle=(left+right)/2;
            if(mappedTime(middle)<seconds)left=middle;else right=middle;
        }
        return (left+right)/2;
    }
    return seconds;
}

float ReactionCoordinateWidget::impactFlash(double seconds) const {
    const auto hit=position(seconds);
    if(!hit.collision||hit.impact>=1)return 0;
    return float((1-hit.impact)*(.35+.65*std::abs(std::cos(3*pi*hit.impact))));
}
QJsonObject ReactionCoordinateWidget::settings() const {
    return {{"enabled",m_enabled},{"grid",m_grid},{"values",m_values},{"reference",m_reference}};
}
void ReactionCoordinateWidget::restoreSettings(const QJsonObject &s) {
    m_enabled=s["enabled"].toBool();
    m_grid=s["grid"].toBool(true);
    m_values=s["values"].toBool();
    m_reference=qBound(0,s["reference"].toInt(),2);
    refresh();
}
void ReactionCoordinateWidget::paintEvent(QPaintEvent *){QPainter p(this);draw(p,rect(),m_time);}
QImage ReactionCoordinateWidget::imageAtTime(QSize size,double seconds,bool transparent,bool force) const {
    QImage result(size,QImage::Format_ARGB32_Premultiplied);result.fill(Qt::transparent);QPainter p(&result);draw(p,QRectF(QPointF(),QSizeF(size)),seconds,transparent,force);return result;
}
void ReactionCoordinateWidget::draw(QPainter &p,const QRectF &bounds,double seconds,bool transparent,bool force) const {
    if(!active()&&!(force&&ready()))return;
    p.save();p.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    if(!transparent)p.fillRect(bounds,QColor(250,251,255));
    const double scale=qMin(bounds.width()/900.,bounds.height()/420.);
    p.translate(bounds.topLeft()+QPointF((bounds.width()-900*scale)/2,(bounds.height()-420*scale)/2));p.scale(scale,scale);
    const auto pos=position(seconds);const auto &a=(*m_steps)[pos.from],&b=(*m_steps)[pos.to];
    const bool showEnergy=hasEnergies(),showImages=hasImages();
    const double u=pos.fraction;const int mode=b.presentation["mode"].toInt();
    auto yFraction=[](int shape,double t){if(shape==0)return smooth(t);if(shape==1)return smooth(qBound(0.,(t-.22)/.56,1.));return t;};
    const double e=energy(a)+(energy(b)-energy(a))*yFraction(mode,u);
    const bool currentEnergy=(hasEnergy(a)&&hasEnergy(b))||(u<=0&&hasEnergy(a))||(u>=1&&hasEnergy(b));
    double reference=0;
    if(m_reference!=0&&showEnergy) {
        bool first=true;
        for(const auto &step:*m_steps)if(hasEnergy(step)) {
            if(first){reference=energy(step);first=false;}
            else if(m_reference==2)reference=qMin(reference,energy(step));
        }
    }
    const double displayedEnergy=(u>=1?energy(b):(u<=0?energy(a):e))-reference;
    const double x=coordinate(a,pos.from)+(coordinate(b,pos.to)-coordinate(a,pos.from))*u;
    p.setFont(QFont("Arial",force?14:11,QFont::Bold));p.setPen(QColor(64,54,120));
    const QString heading=showEnergy?(currentEnergy?QStringLiteral("Energetics   ΔE = %1 kcal/mol").arg(displayedEnergy,0,'f',2):QStringLiteral("Energetics   ΔE 미입력 구간")):QStringLiteral("2D 단계 애니메이션");
    p.drawText(QRectF(18,10,850,26),Qt::AlignLeft,heading+QStringLiteral("    ·    coordinate = %1").arg(x,0,'f',2));
    p.setFont(QFont("Arial",force?11:9));p.setPen(QColor(100,102,120));p.drawText(QRectF(18,37,550,20),Qt::AlignLeft,showEnergy?QStringLiteral("사용자 지정 에너지 · 입력한 에너지 구간만 표시"):QStringLiteral("결합 / 전자 이동 모식도"));
    const double xmin=coordinate(m_steps->first(),0),xmax=coordinate(m_steps->last(),m_steps->size()-1);
    auto drawImpact=[&](const QPointF &hit) {
        if(pos.impact>=1)return;
        const double flash=(1-pos.impact)*(.3+.7*std::abs(std::cos(3*pi*pos.impact)));
        QColor glow(255,159,45);glow.setAlphaF(flash);
        const double radius=12+26*pos.impact;
        p.setPen(QPen(glow,2.5));p.setBrush(Qt::NoBrush);p.drawEllipse(hit,radius,radius);
        for(int k=0;k<8;++k) {
            const double angle=2*pi*k/8;
            const QPointF direction(std::cos(angle),std::sin(angle));
            p.drawLine(hit+direction*(radius+3),hit+direction*(radius+13));
        }
        glow.setAlphaF(.45*flash);p.setPen(Qt::NoPen);p.setBrush(glow);p.drawEllipse(hit,10,10);
    };
    if(showEnergy) {
    double low=0,high=0;
    for(const auto &s:*m_steps)if(hasEnergy(s)) {
        low=qMin(low,energy(s)-reference);
        high=qMax(high,energy(s)-reference);
    }
    const double roughStep=qMax(.01,(high-low)/4);
    const double magnitude=std::pow(10.,std::floor(std::log10(roughStep)));
    const double normalized=roughStep/magnitude;
    const double tickStep=(normalized<=1?1:normalized<=2?2:normalized<=5?5:10)*magnitude;
    const int firstTick=int(std::floor(low/tickStep));
    const int lastTick=qMax(firstTick+1,int(std::ceil(high/tickStep)));
    low=firstTick*tickStep;
    high=lastTick*tickStep;
    const QRectF graph(72,85,showImages?473:763,260);
    auto screen=[&](double cx,double cy){return QPointF(graph.left()+(cx-xmin)/(xmax-xmin)*graph.width(),graph.bottom()-(cy-low)/(high-low)*graph.height());};
    for(int tick=firstTick;tick<=lastTick;++tick) {
        const double value=tick*tickStep;
        const auto at=screen(xmin,value);
        p.setPen(QPen(tick==0?QColor(135,137,160):QColor(195,199,215),1));
        p.drawLine(at,QPointF(m_grid||tick==0?graph.right():at.x()-4,at.y()));
        p.setPen(QColor(70,65,100));
        p.drawText(QRectF(0,at.y()-10,65,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(value,'g',5));
    }
    p.setPen(QPen(QColor(94,95,112),1.5));p.drawLine(graph.bottomLeft(),graph.topLeft());p.drawLine(graph.bottomLeft(),graph.bottomRight());
    for(int i=1;i<m_steps->size();++i){const auto &left=(*m_steps)[i-1],&right=(*m_steps)[i];if(!hasEnergy(left)||!hasEnergy(right))continue;QPainterPath path;const int shape=right.presentation["mode"].toInt();if(shape==3)continue;
        for(int k=0;k<=100;++k){const double t=k/100.;const auto pt=screen(coordinate(left,i-1)+(coordinate(right,i)-coordinate(left,i-1))*t,energy(left)-reference+(energy(right)-energy(left))*yFraction(shape,t));if(k==0)path.moveTo(pt);else path.lineTo(pt);}
        p.setPen(QPen(QColor(112,104,167),2.7,right.presentation["dashed"].toBool()?Qt::DashLine:Qt::SolidLine));p.drawPath(path);
    }
    for(int i=0;i<m_steps->size();++i){const auto &step=(*m_steps)[i];const auto at=screen(coordinate(step,i),hasEnergy(step)?energy(step)-reference:low);p.setBrush(Qt::white);p.setPen(QPen(QColor(125,119,160),1.5));
        if(hasEnergy(step)) {
            p.drawEllipse(at,3.5,3.5);
            if(m_values)p.drawText(QRectF(at.x()-40,at.y()-22,80,16),Qt::AlignCenter,QString::number(energy(step)-reference,'g',5));
        }
        p.drawText(QRectF(at.x()-42,graph.bottom()+9,84,32),Qt::AlignHCenter|Qt::TextWordWrap,(*m_steps)[i].name.left(24));}
    const auto current=screen(x,currentEnergy?displayedEnergy:low);p.setPen(QPen(QColor(133,104,225,125),1.5,Qt::DashLine));p.drawLine(QPointF(current.x(),graph.bottom()),QPointF(current.x(),graph.top()));
    if(currentEnergy) {
    p.setBrush(QColor(132,102,235,35));p.setPen(Qt::NoPen);p.drawEllipse(current,19,19);p.setBrush(QColor(113,77,228));p.drawEllipse(current,7,7);
    const double barY=screen(xmin,displayedEnergy).y();p.setPen(QPen(QColor(113,77,228),5,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(18,screen(xmin,0).y()),QPointF(18,barY));p.setBrush(QColor(113,77,228));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(18,barY),6,6);
    }
    p.setPen(QColor(70,65,100));p.drawText(QRectF(graph.left(),390,graph.width(),22),Qt::AlignCenter,QStringLiteral("Reaction coordinate"));
    if(pos.collision) {
        const double t=qBound(.1,b.presentation["impactAt"].toDouble(.38),.7);
        const double hitX=coordinate(a,pos.from)+(coordinate(b,pos.to)-coordinate(a,pos.from))*t;
        const double hitY=hasEnergy(a)&&hasEnergy(b)?energy(a)-reference+(energy(b)-energy(a))*yFraction(mode,t):0;
        drawImpact(screen(hitX,hitY));
    }
    } else {
        // With artwork alone the horizontal tracker represents progress, never energy.
        const double progress=(x-xmin)/(xmax-xmin);p.setPen(QPen(QColor(190,185,211),3));p.drawLine(QPointF(65,350),QPointF(835,350));
        p.setBrush(QColor(113,77,228));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(65+770*progress,350),7,7);
        if(pos.collision) {
            const double t=qBound(.1,b.presentation["impactAt"].toDouble(.38),.7);
            const double hit=coordinate(a,pos.from)*(1-t)+coordinate(b,pos.to)*t;
            drawImpact(QPointF(65+770*(hit-xmin)/(xmax-xmin),350));
        }
        p.setPen(QColor(70,65,100));p.drawText(QRectF(65,365,770,22),Qt::AlignCenter,QStringLiteral("Reaction coordinate"));
    }
    if(showImages) {
    const QRectF artwork=showEnergy?QRectF(590,95,285,230):QRectF(190,70,520,250);const double eased=smooth(u);
    // Two supplied drawings slide and cross-fade together; no invented atom correspondence.
    auto imageRect=fitImage(m_images[pos.from],artwork);p.save();p.setOpacity(1-eased);p.drawImage(imageRect.translated(-26*eased,0),m_images[pos.from]);p.restore();
    p.save();p.setOpacity(eased);p.drawImage(fitImage(m_images[pos.to],artwork).translated(26*(1-eased),0),m_images[pos.to]);p.restore();
    if(!m_images[pos.to].isNull())annotations(p,b.presentation["annotations"].toArray(),fitImage(m_images[pos.to],artwork).translated(26*(1-eased),0),u);
    if(m_images[pos.from].isNull()&&m_images[pos.to].isNull()){p.setPen(QColor(100,102,120));p.drawText(artwork,Qt::AlignCenter,QStringLiteral("이 구간의 2D 그림 미삽입"));}
    }
    const QRectF caption=showEnergy?(showImages?QRectF(575,345,310,36):QRectF(575,37,310,36)):QRectF(190,390,520,26);
    p.setFont(QFont("Arial",10,QFont::Bold));p.setPen(pos.collision?QColor(210,122,32):QColor(64,54,120));p.drawText(caption,Qt::AlignCenter|Qt::TextWordWrap,pos.collision?QStringLiteral("높은 장벽 · 충돌 / 재시도"):a.name+(pos.from==pos.to?QString():QStringLiteral(" → ")+b.name));
    p.restore();
}

void ReactionCoordinateWidget::edit(int selectedStage) {
    if(m_steps->isEmpty())return;
    QDialog dialog(this);dialog.setObjectName("ReactionCoordinateEditor");dialog.setWindowTitle(QStringLiteral("반응 좌표 · 2D / 에너지 설정"));dialog.resize(QSize(1050,760).boundedTo(screen()->availableGeometry().size()-QSize(32,64)));
    auto draft=*m_steps;auto *layout=new QVBoxLayout(&dialog);
    auto *help=new QLabel(QStringLiteral("2D 그림 또는 에너지(kcal/mol) 중 하나만 입력해도 사용할 수 있습니다. 둘 다 입력하면 함께 움직입니다. 비어 있는 에너지 값은 그래프에서 연결하지 않습니다. 좌표는 단계 순서대로 증가해야 합니다.\n결합·전자 효과는 도착 단계 그림에서 시작점과 끝점을 차례로 클릭해 지정합니다. 오른쪽 클릭은 지정 취소입니다."));help->setWordWrap(true);layout->addWidget(help);
    auto *table=new QTableWidget(draft.size(),7);table->setObjectName("ReactionCoordinateStages");table->setHorizontalHeaderLabels({QStringLiteral("단계"),QStringLiteral("좌표"),QStringLiteral("ΔE (kcal/mol)"),QStringLiteral("2D 그림"),QStringLiteral("구간 모양"),QStringLiteral("파선"),QStringLiteral("진입 장벽")});table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setSelectionMode(QAbstractItemView::SingleSelection);layout->addWidget(table,1);
    auto *canvas=new AnnotationCanvas;layout->addWidget(canvas,2);
    auto *row=new QHBoxLayout;layout->addLayout(row);
    auto *insert=new QPushButton(QStringLiteral("2D 그림 삽입…")),*paste=new QPushButton(QStringLiteral("그림 붙여넣기")),*remove=new QPushButton(QStringLiteral("그림 제거"));row->addWidget(insert);row->addWidget(paste);row->addWidget(remove);
    auto *type=new QComboBox;type->addItem(QStringLiteral("결합 형성 · 녹색"),"form");type->addItem(QStringLiteral("결합 분해 · 주황색"),"break");type->addItem(QStringLiteral("전자 밀도 이동"),"density");type->addItem(QStringLiteral("오비탈 이동"),"orbital");row->addWidget(type);
    auto *undo=new QPushButton(QStringLiteral("효과 하나 취소")),*clear=new QPushButton(QStringLiteral("효과 지우기"));row->addWidget(undo);row->addWidget(clear);
    auto *settingsRow=new QHBoxLayout;layout->addLayout(settingsRow);auto *show=new QCheckBox(QStringLiteral("반응 좌표 강조 · 2D / 에너지 동기화 사용"));show->setChecked(m_enabled);settingsRow->addWidget(show);
    auto *graphRow=new QHBoxLayout;layout->addLayout(graphRow);
    graphRow->addWidget(new QLabel(QStringLiteral("에너지 0 기준")));
    auto *reference=new QComboBox;reference->addItems({QStringLiteral("입력값 유지"),QStringLiteral("첫 입력 단계"),QStringLiteral("최저 에너지")});
    reference->setCurrentIndex(m_reference);graphRow->addWidget(reference);
    auto *grid=new QCheckBox(QStringLiteral("격자선"));grid->setChecked(m_grid);graphRow->addWidget(grid);
    auto *values=new QCheckBox(QStringLiteral("단계별 값 표시"));values->setChecked(m_values);graphRow->addWidget(values);
    graphRow->addStretch();
    auto *actions=new QHBoxLayout;layout->addLayout(actions);
    auto *importEnergy=new QPushButton(QStringLiteral("계산 에너지 가져오기"));actions->addWidget(importEnergy);
    auto *editMark=new QPushButton(QStringLiteral("주석 위치·시간 편집…"));actions->addWidget(editMark);
    auto *keepMarks=new QCheckBox(QStringLiteral("그림 교체 시 주석 유지"));actions->addWidget(keepMarks);
    auto *status=new QLabel;status->setWordWrap(true);layout->addWidget(status);
    connect(importEnergy,&QPushButton::clicked,&dialog,[&]{
        QVector<QPair<int,double>> values;
        double reference=std::numeric_limits<double>::quiet_NaN();
        const int selected=table->currentRow();
        auto endpoint=[](const MechanismStep &step,const char *key)->QJsonValue {
            const auto energies=step.presentation[key].toArray();
            if(energies.isEmpty())return {};
            return step.reversed?energies.first():energies.last();
        };
        for(int i=0;i<draft.size();++i) {
            auto &step=draft[i];
            const QJsonValue savedReference=step.presentation["energySource"].toObject().value("referenceHartree");
            if(!std::isfinite(reference)&&savedReference.isDouble())reference=savedReference.toDouble();
            if(!step.presentation.contains("sourceEnergiesHartree")&&i==selected&&QFileInfo(step.path).isFile()) {
                QApplication::setOverrideCursor(Qt::WaitCursor);
                const auto parsed=FileDispatcher::load(step.path).parseResult;
                QApplication::restoreOverrideCursor();
                if(parsed.success) {
                    const auto source=MechanismStep::fromCalculation(parsed.data,step.frames.size()>1).presentation;
                    for(const auto key:{"sourceEnergiesHartree","sourceEnergiesKcal"})
                        if(source.contains(key))step.presentation[key]=source.value(key);
                }
            }
            const auto value=endpoint(step,"sourceEnergiesHartree");
            if(value.isDouble())values.push_back({i,value.toDouble()});
        }
        if(selected>=0&&!endpoint(draft[selected],"sourceEnergiesHartree").isDouble()) {
            const auto relative=endpoint(draft[selected],"sourceEnergiesKcal");
            if(relative.isDouble()) {
                auto *cell=table->item(selected,2);
                if(!cell->text().trimmed().isEmpty()){status->setText(QStringLiteral("선택 단계에 이미 에너지 값이 있습니다."));return;}
                cell->setText(QString::number(relative.toDouble(),'f',6));
                draft[selected].presentation["energySource"]=QJsonObject{{"relativeKcal",relative},{"path",draft[selected].path}};
                status->setText(QStringLiteral("선택한 원본 경로의 0 기준으로 가져왔습니다. 다른 계산의 기준과 비교하기 전에 확인하세요."));
                return;
            }
        }
        if(values.isEmpty()){status->setText(QStringLiteral("가져올 계산 에너지가 없습니다. 기존 프로젝트는 원본 파일이 있는 단계를 선택하세요."));return;}
        if(!std::isfinite(reference))reference=values.first().second;
        for(const auto &[i,value]:values) {
            auto *cell=table->item(i,2);
            if(!cell->text().trimmed().isEmpty())continue;
            cell->setText(QString::number((value-reference)*627.509474,'f',6));
            draft[i].presentation["energySource"]=QJsonObject{{"hartree",value},{"referenceHartree",reference},{"path",draft[i].path}};
        }
        status->setText(QStringLiteral("첫 계산값 %1 Eh 기준 · kcal/mol · 기존 입력은 유지됩니다. 서로 비교 가능한 계산인지 확인하세요.").arg(reference,0,'g',12));
    });
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    auto select=[&]{const int i=table->currentRow();canvas->pending=false;if(i<0)return;canvas->image=readImage(draft[i].presentation);canvas->marks=draft[i].presentation["annotations"].toArray();canvas->update();};
    for(int i=0;i<draft.size();++i){auto *name=new QTableWidgetItem(draft[i].name);name->setFlags(name->flags()&~Qt::ItemIsEditable);table->setItem(i,0,name);table->setItem(i,1,new QTableWidgetItem(QString::number(coordinate(draft[i],i),'g',12)));table->setItem(i,2,new QTableWidgetItem(draft[i].presentation["energy"].isDouble()?QString::number(energy(draft[i]),'g',12):QString()));auto *picture=new QTableWidgetItem(readImage(draft[i].presentation).isNull()?QStringLiteral("미삽입"):QStringLiteral("삽입됨"));picture->setFlags(picture->flags()&~Qt::ItemIsEditable);table->setItem(i,3,picture);
        auto *mode=new QComboBox;mode->addItems({QStringLiteral("곡선"),QStringLiteral("평평한 준위"),QStringLiteral("직선"),QStringLiteral("점만 표시")});mode->setCurrentIndex(qBound(0,draft[i].presentation["mode"].toInt(),3));table->setCellWidget(i,4,mode);auto *dash=new QCheckBox;dash->setChecked(draft[i].presentation["dashed"].toBool());table->setCellWidget(i,5,dash);
        auto *barrier=new QCheckBox;barrier->setEnabled(i>0);barrier->setChecked(i>0&&draft[i].presentation["impactBarrier"].toBool());
        barrier->setToolTip(QStringLiteral("이 단계로 진입할 때 충돌 후 반동을 연출합니다."));table->setCellWidget(i,6,barrier);
    }
    connect(table,&QTableWidget::currentCellChanged,&dialog,[&](int,int,int,int){select();});
    canvas->changed=[&]{const int i=table->currentRow();if(i>=0)draft[i].presentation["annotations"]=canvas->marks;};
    connect(type,&QComboBox::currentIndexChanged,&dialog,[&]{canvas->type=type->currentData().toString();canvas->pending=false;canvas->update();});
    auto put=[&](const QImage &image){const int i=table->currentRow();if(i<0||image.isNull()){status->setText(QStringLiteral("사용 가능한 그림이 없습니다. 단계를 선택하고 PNG/JPEG/SVG 또는 클립보드 그림을 삽입하세요."));return;}draft[i].presentation["image"]=encodeImage(image);if(!keepMarks->isChecked())draft[i].presentation["annotations"]=QJsonArray();table->item(i,3)->setText(QStringLiteral("삽입됨"));select();status->clear();};
    connect(insert,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getOpenFileName(&dialog,QStringLiteral("2D 구조 그림 삽입"),{},"Images (*.png *.jpg *.jpeg *.bmp *.webp *.svg)");if(path.isEmpty())return;QImageReader reader(path);const auto size=reader.size();if(!size.isValid()||size.width()>16000||size.height()>16000){status->setText(QStringLiteral("그림 크기를 확인하세요 (최대 16000 px)."));return;}if(size.width()>1600||size.height()>1200)reader.setScaledSize(size.scaled(1600,1200,Qt::KeepAspectRatio));put(reader.read());});
    connect(paste,&QPushButton::clicked,&dialog,[&]{put(QApplication::clipboard()->image());});
    connect(remove,&QPushButton::clicked,&dialog,[&]{const int i=table->currentRow();if(i<0)return;draft[i].presentation.remove("image");draft[i].presentation.remove("annotations");table->item(i,3)->setText(QStringLiteral("미삽입"));select();});
    connect(editMark,&QPushButton::clicked,&dialog,[&]{
        if(canvas->marks.isEmpty())return;
        QDialog settings(&dialog);settings.setWindowTitle(QStringLiteral("2D 주석 편집"));
        auto *form=new QFormLayout(&settings);
        auto *which=new QComboBox;
        for(int i=0;i<canvas->marks.size();++i)which->addItem(QStringLiteral("%1 · %2").arg(i+1).arg(canvas->marks[i].toObject()["type"].toString()));
        form->addRow(QStringLiteral("주석"),which);
        QVector<QDoubleSpinBox*> fields;
        for(const auto &label:{QStringLiteral("시작 X"),QStringLiteral("시작 Y"),QStringLiteral("끝 X"),QStringLiteral("끝 Y"),QStringLiteral("표시 시작"),QStringLiteral("표시 종료")}) {
            auto *field=new QDoubleSpinBox;field->setRange(0,100);field->setDecimals(1);field->setSuffix(" %");
            form->addRow(label,field);fields.push_back(field);
        }
        auto loadMark=[&](int i){
            const auto mark=canvas->marks[i].toObject();const auto points=mark["points"].toArray();
            for(int j=0;j<4;++j)fields[j]->setValue(points.at(j).toDouble()*100);
            fields[4]->setValue(mark["start"].toDouble()*100);fields[5]->setValue(mark["end"].toDouble(1)*100);
        };
        connect(which,&QComboBox::currentIndexChanged,&settings,loadMark);loadMark(0);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *erase=buttons->addButton(QStringLiteral("선택 주석 삭제"),QDialogButtonBox::DestructiveRole);
        form->addRow(buttons);
        connect(erase,&QPushButton::clicked,&settings,[&]{settings.done(2);});
        connect(buttons,&QDialogButtonBox::rejected,&settings,&QDialog::reject);
        connect(buttons,&QDialogButtonBox::accepted,&settings,[&]{if(fields[4]->value()<fields[5]->value())settings.accept();else fields[5]->setFocus();});
        const int result=settings.exec(),i=which->currentIndex();
        if(result==2)canvas->marks.removeAt(i);
        else if(result==QDialog::Accepted) {
            auto mark=canvas->marks[i].toObject();QJsonArray points;
            for(int j=0;j<4;++j)points.append(fields[j]->value()/100);
            mark["points"]=points;mark["start"]=fields[4]->value()/100;mark["end"]=fields[5]->value()/100;
            canvas->marks[i]=mark;
        } else return;
        canvas->changed();canvas->update();
    });
    connect(undo,&QPushButton::clicked,&dialog,[&]{if(!canvas->marks.isEmpty())canvas->marks.removeLast();canvas->pending=false;canvas->changed();canvas->update();});
    connect(clear,&QPushButton::clicked,&dialog,[&]{canvas->marks={};canvas->pending=false;canvas->changed();canvas->update();});
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{double previous=-std::numeric_limits<double>::infinity();for(int i=0;i<draft.size();++i){bool okX=false,okE=false;const double x=table->item(i,1)->text().toDouble(&okX),e=table->item(i,2)->text().toDouble(&okE);if(!okX||!std::isfinite(x)||x<=previous||(!table->item(i,2)->text().trimmed().isEmpty()&&(!okE||!std::isfinite(e)))){status->setText(QStringLiteral("%1단계: 증가하는 유한 좌표와 유효한 에너지를 입력하세요.").arg(i+1));return;}previous=x;draft[i].presentation["coordinate"]=x;if(okE)draft[i].presentation["energy"]=e;else draft[i].presentation.remove("energy");draft[i].presentation["mode"]=qobject_cast<QComboBox*>(table->cellWidget(i,4))->currentIndex();draft[i].presentation["dashed"]=qobject_cast<QCheckBox*>(table->cellWidget(i,5))->isChecked();draft[i].presentation["impactBarrier"]=qobject_cast<QCheckBox*>(table->cellWidget(i,6))->isChecked();}dialog.accept();});
    table->setCurrentCell(qBound(0,selectedStage,int(draft.size())-1),0);select();
    if(dialog.exec()!=QDialog::Accepted)return;
    *m_steps=draft;m_grid=grid->isChecked();m_values=values->isChecked();m_reference=reference->currentIndex();m_enabled=show->isChecked();refresh();emit configurationChanged();
}
}
