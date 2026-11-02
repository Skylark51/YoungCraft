#include "ui/PlotWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QPushButton>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>

namespace youngcraft {
namespace {
bool validExport(const PlotWidget::ExportOptions &o) {
    return std::isfinite(o.widthMm) && std::isfinite(o.heightMm)
        && o.widthMm >= 60 && o.widthMm <= 180 && o.heightMm >= 45 && o.heightMm <= 220
        && o.dpi >= 72 && o.dpi <= 1200
        && (o.widthMm / 25.4 * o.dpi) * (o.heightMm / 25.4 * o.dpi) <= 80000000;
}
void physicalFonts(PlotWidget::StyleOptions &s) {
    s.thickness=std::max(2.0/3.0,s.thickness);
    // Fixed 96-dpi drawing coordinates give point sizes a physical meaning in
    // both vector and raster output, independent of the display's DPI setting.
    for (auto *font : {&s.titleFont, &s.axisFont, &s.tickFont, &s.mechanismFont})
        if (font->pointSizeF() > 0) font->setPixelSize(std::max(6,qRound(font->pointSizeF() * 96.0 / 72.0)));
}
}

QImage PlotWidget::publicationImage(const ExportOptions &o) {
    if (!validExport(o)) return {};
    const QSize pixels(qRound(o.widthMm / 25.4 * o.dpi), qRound(o.heightMm / 25.4 * o.dpi));
    QImage result(pixels, QImage::Format_ARGB32_Premultiplied);
    if (result.isNull()) return {};
    result.fill(Qt::transparent);
    QScopedValueRollback<QSize> sizeGuard(m_exportSize, QSize(qRound(o.widthMm/25.4*96), qRound(o.heightMm/25.4*96)));
    QScopedValueRollback<bool> publicationGuard(m_publicationPaint, true);
    QScopedValueRollback<bool> transparencyGuard(m_transparentPaint, o.transparent);
    QScopedValueRollback<StyleOptions> styleGuard(m_options);
    physicalFonts(m_options);
    // Keep the device's logical DPI at 96 until painting finishes.
    result.setDotsPerMeterX(qRound(96 / 0.0254));
    result.setDotsPerMeterY(qRound(96 / 0.0254));
    QPainter painter(&result);
    painter.scale(double(pixels.width())/m_exportSize.width(), double(pixels.height())/m_exportSize.height());
    paintPlot(painter);
    painter.end();
    result.setDotsPerMeterX(qRound(o.dpi / 0.0254));
    result.setDotsPerMeterY(qRound(o.dpi / 0.0254));
    return result;
}

bool PlotWidget::savePublication(const QString &path, const ExportOptions &o, QString *error) {
    auto fail = [&](const QString &message) { if (error) *error = message; return false; };
    if (error) error->clear();
    if (!validExport(o)) return fail(QStringLiteral("그림 크기 또는 해상도를 확인하세요."));
    const auto suffix = QFileInfo(path).suffix().toLower();
    if (suffix != QStringLiteral("pdf") && suffix != QStringLiteral("png"))
        return fail(QStringLiteral("PDF 또는 PNG 형식으로 저장하세요."));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return fail(file.errorString());
    if (suffix == QStringLiteral("png")) {
        const auto result = publicationImage(o);
        if (result.isNull() || !result.save(&file, "PNG")) return fail(QStringLiteral("PNG 그림을 저장하지 못했습니다."));
    } else {
        QScopedValueRollback<QSize> sizeGuard(m_exportSize, QSize(qRound(o.widthMm/25.4*96), qRound(o.heightMm/25.4*96)));
        QScopedValueRollback<bool> publicationGuard(m_publicationPaint, true);
        QScopedValueRollback<bool> transparencyGuard(m_transparentPaint, o.transparent);
        QScopedValueRollback<StyleOptions> styleGuard(m_options);
        physicalFonts(m_options);
        QPdfWriter writer(&file);
        writer.setResolution(96);
        writer.setPageSize(QPageSize(QSizeF(o.widthMm,o.heightMm), QPageSize::Millimeter));
        writer.setPageMargins(QMarginsF(0,0,0,0));
        writer.setCreator(QStringLiteral("Youngcraft"));
        writer.setTitle(m_options.title.isEmpty() ? QStringLiteral("Scientific plot") : m_options.title);
        QPainter painter;
        if (!painter.begin(&writer)) return fail(QStringLiteral("PDF 그림을 생성하지 못했습니다."));
        painter.scale(double(writer.width())/m_exportSize.width(), double(writer.height())/m_exportSize.height());
        paintPlot(painter);
        painter.end();
    }
    if (!file.commit()) return fail(file.errorString());
    return true;
}

void PlotWidget::showExportDialog() {
    QDialog dialog(this);
    dialog.setObjectName(QStringLiteral("PublicationExportDialog"));
    dialog.setWindowTitle(QStringLiteral("논문용 그림 내보내기"));
    dialog.setStyleSheet(QStringLiteral(
        "QDialog#PublicationExportDialog { background:#f6f7f9; }"
        "QDialog#PublicationExportDialog QLabel { color:#26313e; }"
        "QDialog#PublicationExportDialog QPushButton { padding:6px 14px; }"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18,18,18,18);
    layout->setSpacing(14);
    auto *body = new QHBoxLayout;
    auto *form = new QFormLayout;
    auto *preset = new QComboBox;
    preset->addItems({QStringLiteral("단일 칼럼 · 84.6 mm"), QStringLiteral("이중 칼럼 · 177.8 mm"), QStringLiteral("사용자 지정")});
    auto dimension = [](double value, double min, double max) {
        auto *box = new QDoubleSpinBox;
        box->setRange(min,max); box->setDecimals(1); box->setValue(value); box->setSuffix(QStringLiteral(" mm"));
        return box;
    };
    auto *width = dimension(84.6,60,180), *height = dimension(64,45,220);
    width->setObjectName(QStringLiteral("PublicationWidth"));
    height->setObjectName(QStringLiteral("PublicationHeight"));
    auto *dpi = new QComboBox; dpi->addItems({QStringLiteral("300 dpi"),QStringLiteral("600 dpi"),QStringLiteral("1200 dpi")}); dpi->setCurrentIndex(1);
    auto *format = new QComboBox; format->addItems({QStringLiteral("PDF · 벡터"),QStringLiteral("PNG · 이미지")});
    auto *transparent = new QCheckBox(QStringLiteral("투명 배경"));
    transparent->setChecked(m_options.background.alpha()==0);
    form->addRow(QStringLiteral("판형"),preset);
    form->addRow(QStringLiteral("너비"),width); form->addRow(QStringLiteral("높이"),height);
    form->addRow(QStringLiteral("파일 형식"),format); form->addRow(QStringLiteral("PNG 해상도"),dpi);
    form->addRow(transparent);
    auto *note = new QLabel(QStringLiteral("출력 크기 기준으로 글꼴과 선 굵기를 유지합니다.\n선택 테두리는 출력에서 제외됩니다."));
    note->setWordWrap(true); form->addRow(note);
    body->addLayout(form);
    auto *preview = new QLabel; preview->setObjectName(QStringLiteral("PublicationPreview"));
    preview->setFixedSize(480,360); preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet(QStringLiteral("background:#edf0f3;border:1px solid #d8dce1;"));
    body->addWidget(preview); layout->addLayout(body);
    auto *status = new QLabel; status->setWordWrap(true); layout->addWidget(status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("저장…"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("닫기"));
    auto *copy = buttons->addButton(QStringLiteral("그림 복사"),QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    auto options = [&] { ExportOptions o; o.widthMm=width->value(); o.heightMm=height->value(); o.dpi=300*(1<<dpi->currentIndex()); o.transparent=transparent->isChecked(); return o; };
    auto refresh = [&] {
        auto o=options(); o.dpi=144;
        const auto picture=publicationImage(o);
        preview->setPixmap(QPixmap::fromImage(picture).scaled(464,344,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        const bool valid=validExport(options());
        copy->setEnabled(valid); buttons->button(QDialogButtonBox::Save)->setEnabled(valid);
        status->setText(valid ? QStringLiteral("%1 × %2 px · %3 dpi (PNG)").arg(qRound(width->value()/25.4*options().dpi)).arg(qRound(height->value()/25.4*options().dpi)).arg(options().dpi)
                             : QStringLiteral("그림이 너무 큽니다. 크기 또는 해상도를 줄여 주세요."));
    };
    connect(preset,&QComboBox::currentIndexChanged,&dialog,[&](int index) {
        if (index<2) { width->setValue(index==0?84.6:177.8); height->setValue(index==0?64:110); }
        width->setEnabled(index==2); refresh();
    });
    width->setEnabled(false);
    connect(width,&QDoubleSpinBox::valueChanged,&dialog,refresh);
    connect(height,&QDoubleSpinBox::valueChanged,&dialog,refresh);
    connect(dpi,&QComboBox::currentIndexChanged,&dialog,refresh);
    connect(format,&QComboBox::currentIndexChanged,&dialog,[&](int index) { dpi->setEnabled(index==1); refresh(); });
    dpi->setEnabled(false);
    connect(transparent,&QCheckBox::toggled,&dialog,refresh);
    connect(copy,&QPushButton::clicked,&dialog,[&] {
        const auto result=publicationImage(options());
        if (!result.isNull()) { QApplication::clipboard()->setImage(result); status->setText(QStringLiteral("논문용 그림을 클립보드에 복사했습니다.")); }
    });
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
        const bool pdf=format->currentIndex()==0;
        auto path=QFileDialog::getSaveFileName(&dialog,QStringLiteral("논문용 그림 저장"),pdf?QStringLiteral("Youngcraft-figure.pdf"):QStringLiteral("Youngcraft-figure.png"),pdf?QStringLiteral("PDF (*.pdf)"):QStringLiteral("PNG (*.png)"));
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path+=pdf?QStringLiteral(".pdf"):QStringLiteral(".png");
        QString error;
        status->setText(savePublication(path,options(),&error)?QStringLiteral("저장 완료: %1").arg(path):error);
    });
    refresh(); dialog.exec();
}

void PlotWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    auto *axes=menu.addAction(QStringLiteral("축·표기 설정…"));
    menu.addSeparator();
    auto *publish=menu.addAction(QStringLiteral("논문용 그림 내보내기…"));
    auto *copy=menu.addAction(QStringLiteral("논문용 그림 복사 · 600 dpi"));
    const auto *action=menu.exec(event->globalPos());
    if(action==axes)showAxisSettings();
    else if(action==publish)showExportDialog();
    else if(action==copy)QApplication::clipboard()->setImage(publicationImage(ExportOptions{}));
}

void PlotWidget::showAxisSettings() {

        QDialog dialog(this);dialog.setObjectName(QStringLiteral("PlotAxisSettings"));
        dialog.setWindowTitle(QStringLiteral("그래프 축·표기 설정"));
        auto *form=new QFormLayout(&dialog);
        auto *xTitle=new QLineEdit(m_options.xTitle), *yTitle=new QLineEdit(m_options.yTitle);
        auto input=[](double value, double minimum, double maximum) {
            auto *box=new QDoubleSpinBox;
            box->setDecimals(6); box->setRange(minimum,maximum); box->setValue(value);
            box->setMaximumWidth(240);
            return box;
        };
        const auto range=axisRange();
        auto *xmin=input(range.xMin,-1e100,1e100), *xmax=input(range.xMax,-1e100,1e100);
        xmin->setObjectName("PlotXMin");xmax->setObjectName("PlotXMax");
        auto *ymin=input(range.yMin,-1e100,1e100), *ymax=input(range.yMax,-1e100,1e100);
        ymin->setObjectName("PlotYMin");ymax->setObjectName("PlotYMax");
        auto *xstep=input(m_options.xIncrement,0,1e9), *ystep=input(m_options.yIncrement,0,1e9);
        xstep->setObjectName("PlotXStep");ystep->setObjectName("PlotYStep");
        xstep->setSpecialValueText(QStringLiteral("자동")); ystep->setSpecialValueText(QStringLiteral("자동"));
        auto *grid=new QCheckBox; grid->setChecked(m_options.majorGrid);
        auto *points=new QCheckBox; points->setChecked(m_options.showPoints);
        auto *annotation=new QCheckBox; annotation->setChecked(m_options.showTsAnnotation);
        auto *automatic=new QCheckBox(QStringLiteral("데이터에 맞춰 자동 범위")); automatic->setChecked(!m_manualRange.has_value());automatic->setObjectName("PlotAutoRange");
        form->addRow(QStringLiteral("X축 제목"),xTitle); form->addRow(QStringLiteral("Y축 제목"),yTitle);
        form->addRow(automatic);
        form->addRow(QStringLiteral("X 최솟값"),xmin); form->addRow(QStringLiteral("X 최댓값"),xmax);
        form->addRow(QStringLiteral("Y 최솟값"),ymin); form->addRow(QStringLiteral("Y 최댓값"),ymax);
        form->addRow(QStringLiteral("X 눈금 간격 (0: 자동)"),xstep); form->addRow(QStringLiteral("Y 눈금 간격 (0: 자동)"),ystep);
        form->addRow(QStringLiteral("격자 표시"),grid); form->addRow(QStringLiteral("측정점 표시"),points);
        if (m_scan && !m_heatmap) form->addRow(QStringLiteral("TS 후보 표시"),annotation);
        auto *axisWidth=input(m_options.axisThickness,.2,8), *curveWidth=input(m_options.thickness,.2,10);
        auto *axisSize=input(m_options.axisFont.pointSizeF(),6,36);
        axisWidth->setObjectName("PlotAxisWidth");curveWidth->setObjectName("PlotCurveWidth");axisSize->setObjectName("PlotAxisFontSize");
        form->addRow(QStringLiteral("축 선 두께"),axisWidth);form->addRow(QStringLiteral("곡선 두께"),curveWidth);
        form->addRow(m_options.tickFontFollowsAxis ? QStringLiteral("축 제목·눈금 크기 (pt)") : QStringLiteral("축 제목 크기 (pt)"),axisSize);
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("적용"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
        form->addRow(buttons);
        auto validate=[&] {
            for (auto *box:{xmin,xmax,ymin,ymax}) box->setEnabled(!automatic->isChecked());
            buttons->button(QDialogButtonBox::Ok)->setEnabled(automatic->isChecked() || (xmin->value()<xmax->value() && ymin->value()<ymax->value()));
        };
        for (auto *box:{xmin,xmax,ymin,ymax}) connect(box,&QDoubleSpinBox::valueChanged,&dialog,validate);
        connect(automatic,&QCheckBox::toggled,&dialog,validate);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
        connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        validate();
        if (dialog.exec()==QDialog::Accepted) {
            auto style=m_options;
            style.axisThickness=axisWidth->value();style.thickness=curveWidth->value();style.axisFont.setPointSizeF(axisSize->value());
            style.xTitle=xTitle->text(); style.yTitle=yTitle->text();
            style.xIncrement=xstep->value(); style.yIncrement=ystep->value();
            style.majorGrid=grid->isChecked(); style.showPoints=points->isChecked();
            style.showTsAnnotation=annotation->isChecked();
            setStyleOptions(style);
            if (automatic->isChecked()) clearManualRange();
            else setManualRange({xmin->value(),xmax->value(),ymin->value(),ymax->value()});
        }

}
} // namespace youngcraft
