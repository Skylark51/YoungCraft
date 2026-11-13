#include "app/MainWindow.h"
#include "compute/OptimizationInput.h"
#include "ui/XtbDialog.h"
#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include <QCoreApplication>
#include <QDir>
#include <QPointer>
#include <QTabWidget>
#include <QStackedWidget>
#include <QDockWidget>
#include <QStyle>
#include <QTabBar>
#include <algorithm>
namespace youngcraft {
void MainWindow::startXtbCalculation(int method,bool automatic,const QVector<GeometryConstraint> &constraints,int cycles) {
    if(m_view->molecule().isEmpty()){setStatus(QStringLiteral("계산할 구조를 먼저 여세요."));return;}
    stopMMOptimization();
    m_animation->stop();
    m_view->setFrequencyMode(nullptr);
    if(m_drawingMode)setDrawingMode(false);
    if(m_dragAtoms)setAtomDragMode(false);
    if(auto *existing=findChild<XtbDialog *>()){
        if(existing->calculationActive()) {m_structureTabs->setCurrentWidget(existing);m_structureDock->show();return;}
        m_structureTabs->removeTab(m_structureTabs->indexOf(existing));existing->close();delete existing;
    }
    const Molecule original=m_view->molecule();
    auto restore=std::make_shared<Molecule>(original);
    const QPointer<QWidget> workspace=m_activeDocument->workspace;const QPointer<MoleculeView> view=m_view;
    QString runtime=QDir(QCoreApplication::applicationDirPath()).filePath("xtb_runtime");
#ifdef YOUNGCRAFT_TESTING
    if (!qgetenv("YOUNGCRAFT_XTB_TEST_RUNTIME").isEmpty()) runtime=QString::fromUtf8(qgetenv("YOUNGCRAFT_XTB_TEST_RUNTIME"));
#endif
    auto *dialog=new XtbDialog(original,runtime,this,method,automatic,m_view->geometrySelection());
    dialog->embedInSidebar();
    dialog->setSelectionProvider([view]{QVector<int> ids;if(view){ids=view->selection().atoms.values().toVector();std::sort(ids.begin(),ids.end());}return ids;});
    dialog->setInputValidator([this,view,original]{
        if(!view || view!=m_view){setStatus(QStringLiteral("현재 molecule에서 QM 패널을 다시 여세요."));return false;}
        const bool same=sameOptimizationInput(view->molecule(),original);
        if(!same)setStatus(QStringLiteral("구조가 변경되었습니다. QM 패널을 다시 여세요."));
        return same;
    });
    if(!constraints.isEmpty() || cycles!=500)dialog->configureOptimization(constraints,cycles);
    connect(dialog,&XtbDialog::previewCoordinates,this,[this,view](const QVector<QVector3D> &c){if(view && view==m_view)view->setCoordinates(c);});
    connect(dialog,&XtbDialog::restorePreview,this,[view,restore]{if(view){QVector<QVector3D> c;for(const auto &a:restore->atoms)c<<a.position;view->setCoordinates(c);}});
    connect(dialog,&XtbDialog::applyGeometry,this,[this,view,workspace,restore](const Molecule &m){if(!view||!workspace)return;int index=m_documentTabs->indexOf(workspace);if(index>=0)m_documentTabs->setCurrentIndex(index);if(view==m_view){commitEditedMolecule(m,false);*restore=m;}});
    connect(dialog,&XtbDialog::openGeometry,this,[this](const Molecule &m){newDrawingDocument(m);});
    const int tab=m_structureTabs->addTab(dialog,style()->standardIcon(QStyle::SP_ComputerIcon),{});
    m_structureTabs->setTabToolTip(tab,method==0?QStringLiteral("GFN-FF (MM)"):QStringLiteral("QM / xTB"));
    m_structureTabs->tabBar()->setAccessibleTabName(tab,QStringLiteral("QM / xTB"));
    m_structureTabs->setCurrentIndex(tab);m_structureDock->show();m_structureDock->raise();resizeDocks({m_structureDock},{320},Qt::Horizontal);dialog->show();
}
}
