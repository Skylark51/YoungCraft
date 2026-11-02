#include "app/MainWindow.h"
#include "ui/XtbDialog.h"
#include "animation/AnimationController.h"
#include "render/MoleculeView.h"
#include <QCoreApplication>
#include <QDir>
#include <QPointer>
#include <QTabWidget>
#include <QStackedWidget>
namespace youngcraft {
void MainWindow::startXtbCalculation(int method,bool automatic,const QVector<GeometryConstraint> &constraints,int cycles) {
    if(m_view->molecule().isEmpty()){setStatus(QStringLiteral("계산할 구조를 먼저 여세요."));return;}
    stopMMOptimization();
    m_animation->stop();
    m_view->setFrequencyMode(nullptr);
    if(m_drawingMode)setDrawingMode(false);
    if(m_dragAtoms)setAtomDragMode(false);
    if(findChild<XtbDialog *>()){findChild<XtbDialog *>()->raise();return;}
    const Molecule original=m_view->molecule();
    auto restore=std::make_shared<Molecule>(original);
    const QPointer<QWidget> workspace=m_activeDocument->workspace;const QPointer<MoleculeView> view=m_view;
    QString runtime=QDir(QCoreApplication::applicationDirPath()).filePath("xtb_runtime");
#ifdef YOUNGCRAFT_TESTING
    if (!qgetenv("YOUNGCRAFT_XTB_TEST_RUNTIME").isEmpty()) runtime=QString::fromUtf8(qgetenv("YOUNGCRAFT_XTB_TEST_RUNTIME"));
#endif
    auto *dialog=new XtbDialog(original,runtime,this,method,automatic,m_view->geometrySelection());
    if(!constraints.isEmpty() || cycles!=500)dialog->configureOptimization(constraints,cycles);
    connect(dialog,&XtbDialog::previewCoordinates,this,[view](const QVector<QVector3D> &c){if(view)view->setCoordinates(c);});
    connect(dialog,&XtbDialog::restorePreview,this,[view,restore]{if(view){QVector<QVector3D> c;for(const auto &a:restore->atoms)c<<a.position;view->setCoordinates(c);}});
    connect(dialog,&XtbDialog::applyGeometry,this,[this,view,workspace,restore](const Molecule &m){if(!view||!workspace)return;int index=m_documentTabs->indexOf(workspace);if(index>=0)m_documentTabs->setCurrentIndex(index);if(view==m_view){commitEditedMolecule(m,false);*restore=m;}});
    connect(dialog,&XtbDialog::openGeometry,this,[this](const Molecule &m){newDrawingDocument(m);});
    dialog->show();
}
}
