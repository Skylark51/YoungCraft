#include "ui/ReactionCoordinateWidget.h"
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QImageReader>
#include <QJsonArray>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
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
        p.save();
        if(type=="form"||type=="break") {
            const bool forming=type=="form";QColor color=forming?QColor(19,184,119):QColor(242,126,45);
            color.setAlphaF(editing?.8:(forming?.15+.75*smooth(u):.9-.75*smooth(u)));
            QPen pen(color,editing?8:8+5*std::sin(pi*u),forming?Qt::SolidLine:Qt::DashLine,Qt::RoundCap);
            p.setPen(pen);p.drawLine(from,to);
        } else {
            const QPointF control=(from+to)/2+QPointF(0,-rect.height()*.22);
            QPainterPath path(from);path.quadTo(control,to);p.setBrush(Qt::NoBrush);p.setPen(QPen(QColor(100,100,180,140),1.5,Qt::DashLine));p.drawPath(path);
            const QPointF center=path.pointAtPercent(editing?.5:smooth(u));
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
    setObjectName("ReactionCoordinatePreview");setMinimumHeight(290);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);hide();
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
    double time=(*m_steps)[0].frames.size()>1?(*m_steps)[0].seconds:0;times.push_back(time);
    for(int i=1;i<m_steps->size();++i){time+=(*m_steps)[i].seconds;times.push_back(time);}return times;
}
ReactionCoordinateWidget::Position ReactionCoordinateWidget::position(double seconds) const {
    Position result;result.time=seconds;const auto times=stageTimes();if(times.isEmpty())return result;
    if(seconds<=times.first())return result;
    int to=1;while(to<times.size()-1&&seconds>times[to])++to;
    if(times.size()<2)return result;
    result.from=to-1;result.to=to;double u=qBound(0.,(seconds-times[to-1])/(times[to]-times[to-1]),1.);
    // A timed hesitation/rebound for an uphill barrier; endpoint times stay unchanged.
    const bool high=hasEnergy((*m_steps)[to])&&hasEnergy((*m_steps)[to-1])
        &&energy((*m_steps)[to])-energy((*m_steps)[to-1])>=m_barrier;
    if(high&&u>.28&&u<.68){const double v=(u-.28)/.4;u=.28+.18*std::sin(2*pi*v);result.collision=true;}
    else if(high&&u>=.68)u=.28+.72*(u-.68)/.32;
    result.fraction=u;result.time=times[to-1]+u*(times[to]-times[to-1]);return result;
}
double ReactionCoordinateWidget::mappedTime(double seconds) const {return active()?position(seconds).time:seconds;}
QJsonObject ReactionCoordinateWidget::settings() const {return {{"enabled",m_enabled},{"barrier",m_barrier}};}
void ReactionCoordinateWidget::restoreSettings(const QJsonObject &s){m_enabled=s["enabled"].toBool();m_barrier=qBound(.1,s["barrier"].toDouble(20),100000.);refresh();}
void ReactionCoordinateWidget::paintEvent(QPaintEvent *){QPainter p(this);draw(p,rect(),m_time);}
QImage ReactionCoordinateWidget::imageAtTime(QSize size,double seconds) const {
    QImage result(size,QImage::Format_ARGB32_Premultiplied);result.fill(Qt::transparent);QPainter p(&result);draw(p,QRectF(QPointF(),QSizeF(size)),seconds);return result;
}
void ReactionCoordinateWidget::draw(QPainter &p,const QRectF &bounds,double seconds) const {
    if(!active())return;
    p.save();p.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);p.fillRect(bounds,QColor(250,251,255));
    const double scale=qMin(bounds.width()/900.,bounds.height()/420.);
    p.translate(bounds.topLeft()+QPointF((bounds.width()-900*scale)/2,(bounds.height()-420*scale)/2));p.scale(scale,scale);
    p.fillRect(QRectF(0,0,900,420),QColor(250,251,255));
    const auto pos=position(seconds);const auto &a=(*m_steps)[pos.from],&b=(*m_steps)[pos.to];
    const bool showEnergy=hasEnergies(),showImages=hasImages();
    const double u=pos.fraction;const int mode=b.presentation["mode"].toInt();
    auto yFraction=[](int shape,double t){if(shape==0)return smooth(t);if(shape==1)return smooth(qBound(0.,(t-.22)/.56,1.));return t;};
    const double e=energy(a)+(energy(b)-energy(a))*yFraction(mode,u);
    const bool currentEnergy=(hasEnergy(a)&&hasEnergy(b))||(u<=0&&hasEnergy(a))||(u>=1&&hasEnergy(b));
    const double displayedEnergy=u>=1?energy(b):(u<=0?energy(a):e);
    const double x=coordinate(a,pos.from)+(coordinate(b,pos.to)-coordinate(a,pos.from))*u;
    p.setFont(QFont("Arial",11,QFont::Bold));p.setPen(QColor(64,54,120));
    const QString heading=showEnergy?(currentEnergy?QStringLiteral("Energetics   ΔE = %1 kcal/mol").arg(displayedEnergy,0,'f',2):QStringLiteral("Energetics   ΔE 미입력 구간")):QStringLiteral("2D 단계 애니메이션");
    p.drawText(QRectF(18,10,850,26),Qt::AlignLeft,heading+QStringLiteral("    ·    coordinate = %1").arg(x,0,'f',2));
    p.setFont(QFont("Arial",9));p.setPen(QColor(100,102,120));p.drawText(QRectF(18,37,550,20),Qt::AlignLeft,showEnergy?QStringLiteral("사용자 지정 에너지 · 입력한 에너지 구간만 표시"):QStringLiteral("결합 / 전자 이동 모식도"));
    const double xmin=coordinate(m_steps->first(),0),xmax=coordinate(m_steps->last(),m_steps->size()-1);
    if(showEnergy) {
    double low=std::numeric_limits<double>::infinity(),high=-low;
    for(const auto &s:*m_steps)if(hasEnergy(s)){low=qMin(low,energy(s));high=qMax(high,energy(s));}
    const double pad=qMax(2.,(high-low)*.22);low-=pad;high+=pad;
    const QRectF graph(65,85,showImages?480:770,260);
    auto screen=[&](double cx,double cy){return QPointF(graph.left()+(cx-xmin)/(xmax-xmin)*graph.width(),graph.bottom()-(cy-low)/(high-low)*graph.height());};
    p.setPen(QPen(QColor(195,199,215),1));
    for(int tick=0;tick<5;++tick){const double v=low+(high-low)*tick/4;const auto at=screen(xmin,v);p.drawLine(at,QPointF(graph.right(),at.y()));p.drawText(QRectF(0,at.y()-10,58,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(v,'f',1));}
    p.setPen(QPen(QColor(94,95,112),1.5));p.drawLine(graph.bottomLeft(),graph.topLeft());p.drawLine(graph.bottomLeft(),graph.bottomRight());
    for(int i=1;i<m_steps->size();++i){const auto &left=(*m_steps)[i-1],&right=(*m_steps)[i];if(!hasEnergy(left)||!hasEnergy(right))continue;QPainterPath path;const int shape=right.presentation["mode"].toInt();
        for(int k=0;k<=100;++k){const double t=k/100.;const auto pt=screen(coordinate(left,i-1)+(coordinate(right,i)-coordinate(left,i-1))*t,energy(left)+(energy(right)-energy(left))*yFraction(shape,t));if(k==0)path.moveTo(pt);else path.lineTo(pt);}
        p.setPen(QPen(QColor(112,104,167),2.7,right.presentation["dashed"].toBool()?Qt::DashLine:Qt::SolidLine));p.drawPath(path);
    }
    for(int i=0;i<m_steps->size();++i){const auto &step=(*m_steps)[i];const auto at=screen(coordinate(step,i),hasEnergy(step)?energy(step):low);p.setBrush(Qt::white);p.setPen(QPen(QColor(125,119,160),1.5));if(hasEnergy(step))p.drawEllipse(at,3.5,3.5);
        p.drawText(QRectF(at.x()-42,graph.bottom()+9,84,32),Qt::AlignHCenter|Qt::TextWordWrap,(*m_steps)[i].name.left(24));}
    const auto current=screen(x,currentEnergy?displayedEnergy:low);p.setPen(QPen(QColor(133,104,225,125),1.5,Qt::DashLine));p.drawLine(QPointF(current.x(),graph.bottom()),QPointF(current.x(),graph.top()));
    if(currentEnergy) {
    p.setBrush(QColor(132,102,235,35));p.setPen(Qt::NoPen);p.drawEllipse(current,19,19);p.setBrush(QColor(113,77,228));p.drawEllipse(current,7,7);
    const double barY=screen(xmin,displayedEnergy).y();p.setPen(QPen(QColor(113,77,228),5,Qt::SolidLine,Qt::RoundCap));p.drawLine(QPointF(23,graph.bottom()),QPointF(23,barY));p.setBrush(QColor(113,77,228));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(23,barY),6,6);
    }
    p.setPen(QColor(70,65,100));p.drawText(QRectF(graph.left(),390,graph.width(),22),Qt::AlignCenter,QStringLiteral("Reaction coordinate"));
    if(pos.collision){const QPointF hit=current+QPointF(12,-7);p.setPen(QPen(QColor(242,151,59),2.5));for(int k=0;k<7;++k){double angle=2*pi*k/7;p.drawLine(hit+QPointF(std::cos(angle)*12,std::sin(angle)*12),hit+QPointF(std::cos(angle)*22,std::sin(angle)*22));}}
    } else {
        // With artwork alone the horizontal tracker represents progress, never energy.
        const double progress=(x-xmin)/(xmax-xmin);p.setPen(QPen(QColor(190,185,211),3));p.drawLine(QPointF(65,350),QPointF(835,350));
        p.setBrush(QColor(113,77,228));p.setPen(Qt::NoPen);p.drawEllipse(QPointF(65+770*progress,350),7,7);
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

void ReactionCoordinateWidget::edit() {
    if(m_steps->isEmpty())return;
    QDialog dialog(this);dialog.setObjectName("ReactionCoordinateEditor");dialog.setWindowTitle(QStringLiteral("반응 좌표 · 2D / 에너지 설정"));dialog.resize(1050,760);
    auto draft=*m_steps;auto *layout=new QVBoxLayout(&dialog);
    auto *help=new QLabel(QStringLiteral("2D 그림 또는 에너지(kcal/mol) 중 하나만 입력해도 사용할 수 있습니다. 둘 다 입력하면 함께 움직입니다. 비어 있는 에너지 값은 그래프에서 연결하지 않습니다. 좌표는 단계 순서대로 증가해야 합니다.\n결합·전자 효과는 도착 단계 그림에서 시작점과 끝점을 차례로 클릭해 지정합니다. 오른쪽 클릭은 지정 취소입니다."));help->setWordWrap(true);layout->addWidget(help);
    auto *table=new QTableWidget(draft.size(),6);table->setObjectName("ReactionCoordinateStages");table->setHorizontalHeaderLabels({QStringLiteral("단계"),QStringLiteral("좌표"),QStringLiteral("ΔE (kcal/mol)"),QStringLiteral("2D 그림"),QStringLiteral("구간 모양"),QStringLiteral("파선")});table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->setSelectionMode(QAbstractItemView::SingleSelection);layout->addWidget(table,1);
    auto *canvas=new AnnotationCanvas;layout->addWidget(canvas,2);
    auto *row=new QHBoxLayout;layout->addLayout(row);
    auto *insert=new QPushButton(QStringLiteral("2D 그림 삽입…")),*paste=new QPushButton(QStringLiteral("그림 붙여넣기")),*remove=new QPushButton(QStringLiteral("그림 제거"));row->addWidget(insert);row->addWidget(paste);row->addWidget(remove);
    auto *type=new QComboBox;type->addItem(QStringLiteral("결합 형성 · 녹색"),"form");type->addItem(QStringLiteral("결합 분해 · 주황색"),"break");type->addItem(QStringLiteral("전자 밀도 이동"),"density");type->addItem(QStringLiteral("오비탈 이동"),"orbital");row->addWidget(type);
    auto *undo=new QPushButton(QStringLiteral("효과 하나 취소")),*clear=new QPushButton(QStringLiteral("효과 지우기"));row->addWidget(undo);row->addWidget(clear);
    auto *settingsRow=new QHBoxLayout;layout->addLayout(settingsRow);auto *show=new QCheckBox(QStringLiteral("반응 좌표 강조 · 2D / 에너지 동기화 사용"));show->setChecked(m_enabled);settingsRow->addWidget(show);
    settingsRow->addWidget(new QLabel(QStringLiteral("충돌 장벽 ΔE ≥")));auto *barrier=new QDoubleSpinBox;barrier->setRange(.1,100000);barrier->setValue(m_barrier);barrier->setSuffix(" kcal/mol");settingsRow->addWidget(barrier);
    auto *status=new QLabel;status->setWordWrap(true);layout->addWidget(status);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    auto select=[&]{const int i=table->currentRow();canvas->pending=false;if(i<0)return;canvas->image=readImage(draft[i].presentation);canvas->marks=draft[i].presentation["annotations"].toArray();canvas->update();};
    for(int i=0;i<draft.size();++i){auto *name=new QTableWidgetItem(draft[i].name);name->setFlags(name->flags()&~Qt::ItemIsEditable);table->setItem(i,0,name);table->setItem(i,1,new QTableWidgetItem(QString::number(coordinate(draft[i],i),'g',12)));table->setItem(i,2,new QTableWidgetItem(draft[i].presentation["energy"].isDouble()?QString::number(energy(draft[i]),'g',12):QString()));auto *picture=new QTableWidgetItem(readImage(draft[i].presentation).isNull()?QStringLiteral("미삽입"):QStringLiteral("삽입됨"));picture->setFlags(picture->flags()&~Qt::ItemIsEditable);table->setItem(i,3,picture);
        auto *mode=new QComboBox;mode->addItems({QStringLiteral("곡선"),QStringLiteral("평평한 준위"),QStringLiteral("직선")});mode->setCurrentIndex(qBound(0,draft[i].presentation["mode"].toInt(),2));table->setCellWidget(i,4,mode);auto *dash=new QCheckBox;dash->setChecked(draft[i].presentation["dashed"].toBool());table->setCellWidget(i,5,dash);}
    connect(table,&QTableWidget::currentCellChanged,&dialog,[&](int,int,int,int){select();});
    canvas->changed=[&]{const int i=table->currentRow();if(i>=0)draft[i].presentation["annotations"]=canvas->marks;};
    connect(type,&QComboBox::currentIndexChanged,&dialog,[&]{canvas->type=type->currentData().toString();canvas->pending=false;canvas->update();});
    auto put=[&](const QImage &image){const int i=table->currentRow();if(i<0||image.isNull()){status->setText(QStringLiteral("사용 가능한 그림이 없습니다. 단계를 선택하고 PNG/JPEG/SVG 또는 클립보드 그림을 삽입하세요."));return;}draft[i].presentation["image"]=encodeImage(image);draft[i].presentation["annotations"]=QJsonArray();table->item(i,3)->setText(QStringLiteral("삽입됨"));select();status->clear();};
    connect(insert,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getOpenFileName(&dialog,QStringLiteral("2D 구조 그림 삽입"),{},"Images (*.png *.jpg *.jpeg *.bmp *.webp *.svg)");if(path.isEmpty())return;QImageReader reader(path);const auto size=reader.size();if(!size.isValid()||size.width()>16000||size.height()>16000){status->setText(QStringLiteral("그림 크기를 확인하세요 (최대 16000 px)."));return;}if(size.width()>1600||size.height()>1200)reader.setScaledSize(size.scaled(1600,1200,Qt::KeepAspectRatio));put(reader.read());});
    connect(paste,&QPushButton::clicked,&dialog,[&]{put(QApplication::clipboard()->image());});
    connect(remove,&QPushButton::clicked,&dialog,[&]{const int i=table->currentRow();if(i<0)return;draft[i].presentation.remove("image");draft[i].presentation.remove("annotations");table->item(i,3)->setText(QStringLiteral("미삽입"));select();});
    connect(undo,&QPushButton::clicked,&dialog,[&]{if(!canvas->marks.isEmpty())canvas->marks.removeLast();canvas->pending=false;canvas->changed();canvas->update();});
    connect(clear,&QPushButton::clicked,&dialog,[&]{canvas->marks={};canvas->pending=false;canvas->changed();canvas->update();});
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{double previous=-std::numeric_limits<double>::infinity();for(int i=0;i<draft.size();++i){bool okX=false,okE=false;const double x=table->item(i,1)->text().toDouble(&okX),e=table->item(i,2)->text().toDouble(&okE);if(!okX||!std::isfinite(x)||x<=previous||(!table->item(i,2)->text().trimmed().isEmpty()&&(!okE||!std::isfinite(e)))){status->setText(QStringLiteral("%1단계: 증가하는 유한 좌표와 유효한 에너지를 입력하세요.").arg(i+1));return;}previous=x;draft[i].presentation["coordinate"]=x;if(okE)draft[i].presentation["energy"]=e;else draft[i].presentation.remove("energy");draft[i].presentation["mode"]=qobject_cast<QComboBox*>(table->cellWidget(i,4))->currentIndex();draft[i].presentation["dashed"]=qobject_cast<QCheckBox*>(table->cellWidget(i,5))->isChecked();}dialog.accept();});
    table->setCurrentCell(0,0);select();
    if(dialog.exec()!=QDialog::Accepted)return;
    *m_steps=draft;m_barrier=barrier->value();m_enabled=show->isChecked();refresh();emit configurationChanged();
}
}
