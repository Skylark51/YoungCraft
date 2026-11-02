#include "ui/StyleEditorDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace youngcraft {

QWidget *StyleEditorDialog::makeIsosurfaceTab() {
    auto *page = new QWidget;
    auto *layout = new QFormLayout(page);
    m_showPositiveSurface = new QCheckBox(QStringLiteral("양의 등가면 표시"));
    m_showPositiveSurface->setChecked(m_style.showPositiveSurface);
    m_showNegativeSurface = new QCheckBox(QStringLiteral("음의 등가면 표시"));
    m_showNegativeSurface->setChecked(m_style.showNegativeSurface);
    m_isoValue = new QDoubleSpinBox;
    m_isoValue->setRange(0.000001, 1000.0);
    m_isoValue->setDecimals(6);
    m_isoValue->setSingleStep(0.01);
    m_isoValue->setValue(m_style.isoValue);
    m_isoOpacity = new QDoubleSpinBox;
    m_isoOpacity->setRange(0.05, 1.0);
    m_isoOpacity->setSingleStep(0.05);
    m_isoOpacity->setValue(m_style.isoOpacity);
    layout->addRow(m_showPositiveSurface);
    layout->addRow(m_showNegativeSurface);
    layout->addRow(QStringLiteral("등값(isovalue)"), m_isoValue);
    layout->addRow(QStringLiteral("불투명도"), m_isoOpacity);
    layout->addRow(new QLabel(QStringLiteral("색상은 양수 파랑, 음수 빨강 기본값으로 표시합니다.")));
    layout->addRow(new QLabel(QStringLiteral("CUBE를 열면 백그라운드에서 등가면을 생성합니다.")));
    return page;
}

QWidget *StyleEditorDialog::makeSimpleTab(const QString &message) {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    auto *label = new QLabel(message);
    label->setWordWrap(true);
    layout->addWidget(label);
    layout->addStretch();
    return page;
}

} // namespace youngcraft
