#pragma once
#include "render/MoleculeView.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <functional>

namespace youngcraft {
inline QWidget *captureRotationControls(const QVector<MoleculeView *> &views, QWidget *parent,
                                       std::function<void()> reset) {
    auto *bar=new QWidget(parent);auto *row=new QHBoxLayout(bar);row->setContentsMargins(0,0,0,0);
    row->addWidget(new QLabel(QStringLiteral("회전 (화면 축)")));
    auto *axis=new QComboBox;axis->setObjectName("CaptureRotationAxis");
    axis->addItems({QStringLiteral("자유 회전"),QStringLiteral("X축 ↕"),QStringLiteral("Y축 ↔"),QStringLiteral("Z축 ↔")});row->addWidget(axis);
    auto *speed=new QDoubleSpinBox;speed->setObjectName("CaptureRotationSpeed");speed->setRange(.1,2);speed->setSingleStep(.1);speed->setValue(.6);speed->setSuffix(QStringLiteral(" × 감도"));row->addWidget(speed);
    auto apply=[views,axis,speed]{for(auto *v:views)v->setCaptureRotation(axis->currentIndex()==0?-2:axis->currentIndex()-1,float(speed->value()));};
    QObject::connect(axis,qOverload<int>(&QComboBox::currentIndexChanged),bar,[apply]{apply();});
    QObject::connect(speed,qOverload<double>(&QDoubleSpinBox::valueChanged),bar,[apply]{apply();});apply();
    auto *step=new QDoubleSpinBox;step->setObjectName("CaptureRotationStep");step->setRange(.1,90);step->setValue(5);step->setSuffix(QStringLiteral(" °"));row->addWidget(step);
    for(int sign:{-1,1}) {
        auto *button=new QPushButton(sign<0?QStringLiteral("− 회전"):QStringLiteral("+ 회전"));button->setObjectName(sign<0?"CaptureRotateMinus":"CaptureRotatePlus");row->addWidget(button);
        QObject::connect(button,&QPushButton::clicked,bar,[views,axis,step,sign]{
            if(views.isEmpty())return;
            QVector3D direction;direction[axis->currentIndex()==0?1:axis->currentIndex()-1]=1;
            auto *active=views.first();for(auto *view:views)if(view->isVisible()){active=view;break;}
            auto camera=active->cameraState();camera.rotation=(QQuaternion::fromAxisAndAngle(direction,float(step->value()*sign))*camera.rotation).normalized();
            active->setCameraState(camera);emit active->cameraChanged();
        });
    }
    auto *restore=new QPushButton(QStringLiteral("시작 뷰 복원"));restore->setObjectName("CaptureResetView");row->addWidget(restore);
    QObject::connect(restore,&QPushButton::clicked,bar,[reset]{reset();});row->addStretch();
    bar->setToolTip(QStringLiteral("우클릭 드래그: 회전 · Ctrl+우클릭 또는 가운데 버튼: 이동 · 휠: 확대/축소. 자유 회전의 ± 버튼은 화면 Y축입니다."));
    return bar;
}
}
