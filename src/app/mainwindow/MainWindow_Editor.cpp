#include "ui/SidebarButton.h"
#include "app/MainWindow.h"
#include <QPainter>
#include "render/MoleculeView.h"
#include "animation/AnimationController.h"
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTimer>
#include <QStandardPaths>
#include <QUuid>

namespace youngcraft {
void MainWindow::saveLigandTemplate(){
    const Molecule molecule=m_view->molecule();if(molecule.atoms.isEmpty())return;
    QDialog dialog(this);dialog.setWindowTitle(QStringLiteral("템플릿 저장"));QFormLayout form(&dialog);
    QComboBox name;name.setEditable(true);name.addItems({"TBDAP","MDAP","MPDAP","CHDAP","12-MTP (Me3-TPADP)","14-MTP","Me2S-DPADP"});
    QComboBox metal;for(int i=0;i<molecule.atoms.size();++i)if(isMetal(molecule.atoms[i].atomicNumber))metal.addItem(QString("%1 · atom %2").arg(molecule.atoms[i].element).arg(i+1),i);
    if(!metal.count()){setStatus(QStringLiteral("템플릿의 배위 기준이 될 금속을 먼저 넣으세요."));return;}
    const auto selected=m_view->selection().atoms;
    QCheckBox selection(QStringLiteral("선택 원자만 저장 (금속 포함)"));selection.setChecked(selected.size()>1);
    QLabel note(QStringLiteral("용매·기질·산소 fragment을 제외한 ligand와 금속만 선택하세요.\n금속은 dummy 기준점으로 저장하며 기존 주개 결합과 상대 좌표를 유지합니다.\n사용자 템플릿은 다음 실행에서도 ligand 목록에 표시됩니다."));note.setWordWrap(true);
    form.addRow(&note);form.addRow(QStringLiteral("ligand 이름"),&name);form.addRow(QStringLiteral("기준 금속"),&metal);form.addRow(&selection);
    QDialogButtonBox buttons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);form.addRow(&buttons);
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    const QString path=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/lab_fragments/ligands/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".cjson";
    QString error;if(!FragmentLibrary::saveLigand(molecule,metal.currentData().toInt(),selection.isChecked()?selected:QSet<int>{},name.currentText(),path,&error)){setStatus(error,10000);return;}
    setStatus(QStringLiteral("ligand 템플릿 저장: %1").arg(name.currentText()),8000);
}
DrawState MainWindow::drawState() const {
    DrawState s;s.element=atomicNumberFor(m_drawElement->currentText());s.bondOrder=m_drawBondOrder->currentData().toDouble();
    if(auto *q=m_drawBar->findChild<QSpinBox *>("DrawFormalCharge"))s.formalCharge=q->value();
    s.autoHydrogen=m_autoHButton->isChecked();return s;
}
void MainWindow::ensureEditableDocument() {
    if(m_currentFile.isEmpty()&&m_data.frames.isEmpty())return;
    Molecule original=m_view->molecule();
    newDrawingDocument(original);
    setStatus(QStringLiteral("현재 구조의 편집 복사본을 새 탭에 만들었습니다. 원본 계산과 프레임은 보존됩니다."),8000);
}
void MainWindow::setupEditorControls(QGridLayout *tools) {
    auto *layout=qobject_cast<QVBoxLayout *>(m_drawBar->layout());
    auto *arm=new QPushButton(QStringLiteral("원자 / 결합 그리기"),m_drawBar);arm->setObjectName("ArmDrawingTool");arm->setCheckable(true);
    arm->setToolTip(QStringLiteral("원소 또는 그리기 도구를 선택한 뒤 캔버스에 그립니다. 모드 진입만으로는 그려지지 않습니다."));
    makeSidebarIconButton(arm, QStyle::SP_FileDialogContentsView);
    tools->addWidget(arm, 0, 0);
    connect(arm,&QPushButton::clicked,m_drawBar,[view=m_view,element=m_drawElement](bool on){int z=atomicNumberFor(element->currentText());if(on&&z>0)view->setDrawingElement(z);else view->setDrawingArmed(false);});
    connect(m_view,&MoleculeView::drawingToolChanged,arm,&QPushButton::setChecked);
    auto *chain=new QPushButton(QStringLiteral("연속 결합"),m_drawBar);chain->setObjectName("ContinuousDrawingButton");
    chain->setCheckable(true);chain->setChecked(true);m_view->setChainDrawing(true);
    chain->setToolTip(QStringLiteral("클릭으로 사슬 연장 · 기존 원자 연결로 고리 닫기 · 30° 스냅 · Alt: 자유 배치 · Esc/우클릭: 사슬 끝내기"));
    makeSidebarIconButton(chain, QStyle::SP_BrowserReload);
    tools->addWidget(chain, 0, 1);
    connect(chain,&QPushButton::toggled,m_view,&MoleculeView::setChainDrawing);
    auto *newChain=new QPushButton(QStringLiteral("새 사슬 (Esc)"),m_drawBar);newChain->setObjectName("EndDrawingChainButton");
    makeSidebarIconButton(newChain, QStyle::SP_FileDialogNewFolder);
    tools->addWidget(newChain, 0, 2);
    connect(newChain,&QPushButton::clicked,m_view,&MoleculeView::endDrawingChain);
    auto *lab=new QPushButton(QStringLiteral("Lab Ligands…"),m_drawBar);lab->setObjectName("LabLigandsButton");
    makeSidebarIconButton(lab, QStyle::SP_DirOpenIcon);
    QPixmap ligandPix(32,32);ligandPix.fill(Qt::transparent);QPainter ligandPainter(&ligandPix);
    ligandPainter.setRenderHint(QPainter::Antialiasing);ligandPainter.setPen(QPen(QColor("#376688"),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    ligandPainter.drawPolyline(QPolygonF{{5,11},{12,7},{19,11},{19,20},{12,24},{5,20},{5,11}});
    ligandPainter.drawLine(19,11,27,6);ligandPainter.drawLine(19,20,27,25);ligandPainter.end();lab->setIcon(QIcon(ligandPix));
    tools->addWidget(lab, 0, 3);

    auto *charge=new QSpinBox(m_drawBar);charge->setObjectName("DrawFormalCharge");charge->setRange(-4,4);charge->setPrefix("전하: ");layout->addWidget(charge);
    auto *periodic=new QPushButton(QStringLiteral("주기율표"),m_drawBar);periodic->setObjectName("DrawPeriodicTable");
    makeSidebarIconButton(periodic, QStyle::SP_FileDialogListView);
    tools->addWidget(periodic, 2, 0);

    auto *recent=new QComboBox(m_drawBar);recent->setObjectName("DrawRecentElements");recent->addItem(QStringLiteral("최근 원소"));recent->addItems(QSettings().value("drawing/recentElements").toStringList());recent->setMaximumWidth(100);layout->addWidget(recent);
    connect(recent,&QComboBox::textActivated,m_drawElement,[selector=m_drawElement,view=m_view](const QString&s){int z=atomicNumberFor(s);if(z>0){selector->setCurrentText(s);view->setDrawingElement(z);}});
    auto *state=new QLabel(m_drawBar);state->setObjectName("DrawStateSummary");state->setWordWrap(true);layout->addWidget(state);
    auto refresh=[view=m_view,element=m_drawElement,order=m_drawBondOrder,autoH=m_autoHButton,charge,state]{DrawState s;s.element=atomicNumberFor(element->currentText());s.bondOrder=order->currentData().toDouble();s.formalCharge=charge->value();s.autoHydrogen=autoH->isChecked();view->setDrawingBondOrder(s.bondOrder);state->setText(QString("원소: %1 · 결합: %2 · 전하: %3 · 자동 수소: %4").arg(elementSymbol(s.element),s.bondOrder==0?"자동":QString::number(s.bondOrder)).arg(s.formalCharge).arg(s.autoHydrogen?"켜짐":"꺼짐")+(view->drawingArmed()?QStringLiteral(" · 그리기 준비"):QStringLiteral(" · 도구 선택 대기")));};
    connect(m_view,&MoleculeView::drawingToolChanged,m_drawBar,[refresh](bool){refresh();});
    connect(charge,qOverload<int>(&QSpinBox::valueChanged),m_drawBar,[refresh](int){refresh();});
    connect(m_drawBondOrder,qOverload<int>(&QComboBox::currentIndexChanged),m_drawBar,[refresh](int){refresh();});
    connect(m_autoHButton,&QPushButton::toggled,m_drawBar,[refresh](bool){refresh();});
    connect(m_drawElement,&QComboBox::currentTextChanged,m_drawBar,[refresh,recent](const QString&s){refresh();if(atomicNumberFor(s)<1)return;QStringList list=QSettings().value("drawing/recentElements").toStringList();list.removeAll(s);list.prepend(s);while(list.size()>8)list.removeLast();QSettings().setValue("drawing/recentElements",list);QSignalBlocker block(recent);recent->clear();recent->addItem(QStringLiteral("최근 원소"));recent->addItems(list);});refresh();

}
void MainWindow::connectEditorControls() {
    auto *lab=m_drawBar->findChild<QPushButton *>("LabLigandsButton");
    auto *periodic=m_drawBar->findChild<QPushButton *>("DrawPeriodicTable");
    m_documentConnections.push_back(connect(lab,&QPushButton::clicked,this,[this]{openFragmentBrowser(true);}));
    m_documentConnections.push_back(connect(periodic,&QPushButton::clicked,this,[this]{QDialog d(this);d.setWindowTitle(QStringLiteral("주기율표 — 원소 선택"));QGridLayout grid(&d);
        // Standard periodic-table rows, including separated lanthanide / actinide rows.
        const QVector<QVector<int>> rows={{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},{3,4,0,0,0,0,0,0,0,0,0,0,5,6,7,8,9,10},{11,12,0,0,0,0,0,0,0,0,0,0,13,14,15,16,17,18},{19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36},{37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54},{55,56,57,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86},{87,88,89,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118},{0,0,58,59,60,61,62,63,64,65,66,67,68,69,70,71},{0,0,90,91,92,93,94,95,96,97,98,99,100,101,102,103}};
        for(int r=0;r<rows.size();++r)for(int c=0;c<rows[r].size();++c)if(int z=rows[r][c]){auto*b=new QPushButton(elementSymbol(z),&d);b->setFixedSize(38,38);const bool metalloid=QSet<int>{5,14,32,33,51,52,84}.contains(z);
            const bool metal=!metalloid && isMetal(z);
            const QString group=metalloid?QStringLiteral("준금속"):metal?QStringLiteral("금속"):QStringLiteral("비금속");
            const QString color=metalloid?"#eadcf5":metal?"#d7e8fa":"#d9efdf";
            b->setStyleSheet(QString("QPushButton{background:%1;color:#172b3a;border:1px solid #8296a5;border-radius:4px;min-width:28px;min-height:28px;}QPushButton:hover{border:2px solid #275b8a;}QPushButton:focus{border:2px solid #275b8a;}").arg(color));
            b->setToolTip(QString("%1 · %2").arg(z).arg(group));grid.addWidget(b,r,c);connect(b,&QPushButton::clicked,&d,[&,z]{m_drawElement->setCurrentText(elementSymbol(z));m_view->setDrawingElement(z);d.accept();});}grid.addWidget(new QLabel(QStringLiteral("금속: 파랑 · 비금속: 초록 · 준금속: 보라"),&d),9,0,1,18);d.exec();}));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::atomEditRequested,this,[this](int i){
        if(consumeInsertion(i,m_view->molecule().atoms[i].position))return;
        const auto anchor=m_view->molecule().atoms[i].position;
        Molecule edited=m_view->molecule();auto s=drawState();MoleculeEditController::replace(edited,{i},s.element,s.formalCharge);
        int selected=i;if(s.autoHydrogen){selected=0;for(int j=0;j<i;++j)if(!edited.atoms[j].autoHydrogen)++selected;}
        commitEditedMolecule(edited,true);if(selected>=0&&selected<m_view->molecule().atoms.size()){m_view->selection().atoms={selected};m_view->setSelectedAtom(selected);}
        m_view->setDrawingAnchorAt(anchor);
    }));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::bondEditRequested,this,[this](int i){auto edited=m_view->molecule();if(i<0||i>=edited.bonds.size())return;auto &b=edited.bonds[i];double o=drawState().bondOrder;b.order=o==0?(b.order>=3?1:b.order+1):o;b.manual=true;commitEditedMolecule(edited,true);}));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::deleteRequested,this,[this](int atom,int bond){auto edited=m_view->molecule();if(atom>=0)edited.removeAtom(atom);else if(bond>=0&&bond<edited.bonds.size())edited.bonds.removeAt(bond);else return;commitEditedMolecule(edited,true);}));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::attachmentClicked,this,[this](int center,const QVector3D&dir){consumeInsertion(center,m_view->molecule().atoms[center].position,dir);}));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::groupRotated,this,[this](const QQuaternion &q,const QVector3D &center){
        auto edited=m_view->molecule();const auto ids=m_view->selection().atoms;
        for(int i:ids)edited.atoms[i].position=center+q.rotatedVector(edited.atoms[i].position-center);
        for(auto &site:edited.coordinationSites)if(ids.contains(site.center))site.direction=q.rotatedVector(site.direction);
        commitEditedMolecule(edited,false);
    }));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::groupMoved,this,[this](const QVector3D&t){auto edited=m_view->molecule();MoleculeEditController::transform(edited,m_view->selection().atoms,t,{},{ });for(int i:m_view->selection().atoms)if(edited.atoms[i].atomicNumber==1)edited.atoms[i].autoHydrogen=false;commitEditedMolecule(edited,false);}));
    m_documentConnections.push_back(connect(m_view,&MoleculeView::bondManipulated,this,[this](int a,int b,int kind,double value,bool opposite){auto edited=m_view->molecule();QString error;if(MoleculeEditController::bondGeometry(edited,a,b,kind,value,opposite,&error))commitEditedMolecule(edited,false);else setStatus(error,6000);}));
}
void MainWindow::deleteSelection(){
    if(isTextInputFocused())return;auto ids=m_view->selection().atoms;auto bs=m_view->selection().bonds;
    if(ids.isEmpty()&&bs.isEmpty()&&m_view->selectedAtom()>=0)ids.insert(m_view->selectedAtom());
    if(ids.isEmpty()&&bs.isEmpty())return;
    auto edited=m_view->molecule();for(int i=edited.bonds.size()-1;i>=0;--i)if(bs.contains(i))edited.bonds.removeAt(i);
    for(int i=edited.atoms.size()-1;i>=0;--i)if(ids.contains(i))edited.removeAtom(i);commitEditedMolecule(edited,true);
}
void MainWindow::changeSelectionElement(){
    auto ids=m_view->selection().atoms;if(ids.isEmpty()&&m_view->selectedAtom()>=0)ids.insert(m_view->selectedAtom());if(ids.isEmpty())return;
    QDialog d(this);d.setWindowTitle("원소 변경");QFormLayout form(&d);QComboBox element;for(int z=1;z<=118;++z)element.addItem(elementSymbol(z),z);element.setCurrentIndex(drawState().element-1);QSpinBox charge;charge.setRange(-4,4);charge.setValue(drawState().formalCharge);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));form.addRow("원소",&element);form.addRow("형식 전하",&charge);form.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    auto edited=m_view->molecule();MoleculeEditController::replace(edited,ids,element.currentData().toInt(),charge.value());commitEditedMolecule(edited,true);
}
void MainWindow::manipulateSelection(){
    auto ids=m_view->selection().atoms;if(ids.isEmpty()&&m_view->selectedAtom()>=0)ids.insert(m_view->selectedAtom());if(ids.isEmpty()){setStatus(QStringLiteral("이동할 원자를 선택하세요."));return;}
    QDialog d(this);d.setWindowTitle("선택 원자 이동 / 회전");QFormLayout form(&d);QDoubleSpinBox tx,ty,tz,rx,ry,rz;for(auto*b:{&tx,&ty,&tz}){b->setRange(-10000,10000);b->setDecimals(4);b->setSuffix(" Å");}for(auto*b:{&rx,&ry,&rz}){b->setRange(-360,360);b->setSuffix("°");}form.addRow("X축 이동",&tx);form.addRow("Y축 이동",&ty);form.addRow("Z축 이동",&tz);form.addRow("X축 회전",&rx);form.addRow("Y축 회전",&ry);form.addRow("Z축 회전",&rz);QComboBox center;center.addItems({"원점","molecule 중심","선택 원자 중심"});center.setCurrentIndex(2);form.addRow("회전 중심",&center);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));form.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    auto edited=m_view->molecule();QVector3D c;if(center.currentIndex()==1){for(auto a:edited.atoms)c+=a.position;c/=qMax(1,int(edited.atoms.size()));}if(center.currentIndex()==2){for(int i:ids)c+=edited.atoms[i].position;c/=ids.size();}
    MoleculeEditController::transform(edited,ids,{float(tx.value()),float(ty.value()),float(tz.value())},{float(rx.value()),float(ry.value()),float(rz.value())},c);commitEditedMolecule(edited,false);
}
void MainWindow::insertCoordinationDialog(){
    QDialog d(this);d.setWindowTitle("Complex 제작 도우미");QFormLayout form(&d);QComboBox element,geometry;for(int z=1;z<=118;++z)element.addItem(elementSymbol(z),z);element.setCurrentIndex(26);geometry.addItems(FragmentLibrary::geometries());geometry.setCurrentIndex(7);QSpinBox charge;charge.setRange(-4,4);charge.setValue(2);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));form.addRow("원소",&element);form.addRow("형식 전하",&charge);form.addRow("배위 구조",&geometry);form.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
    setDrawingMode(true);m_view->endDrawingChain();m_view->setProperty("pendingFragment",QString());m_view->setProperty("pendingCenter",element.currentData());m_view->setProperty("pendingCharge",charge.value());m_view->setProperty("pendingGeometry",geometry.currentIndex());m_view->setTemplatePreview(geometry.currentIndex());setStatus(QStringLiteral("빈 공간을 클릭하여 배위 중심을 삽입하세요."),0);
}
bool MainWindow::consumeInsertion(int target,const QVector3D&p,const QVector3D&dir){
    QString path=m_view->property("pendingFragment").toString();int z=m_view->property("pendingCenter").toInt();if(path.isEmpty()&&z==0)return false;
    auto edited=m_view->molecule();QString error;
    if(z>0){if(target>=0){setStatus(QStringLiteral("배위 중심은 빈 공간에 삽입하세요."));return true;}FragmentLibrary::coordinationCenter(edited,z,m_view->property("pendingCharge").toInt(),m_view->property("pendingGeometry").toInt(),p);}
    else{Fragment f;if(!FragmentLibrary::read(path,f,&error)||!FragmentLibrary::insert(edited,f,target,p,dir,&error)){setStatus(error,8000);return true;}}
    m_view->setProperty("pendingFragment",QString());m_view->setProperty("pendingCenter",0);m_view->clearInsertionPreview();commitEditedMolecule(edited,true);return true;
}
void MainWindow::createBuildMenu(){
    auto *menu=menuBar()->addMenu("구조 만들기");menu->setObjectName("StructureBuildMenu");auto add=[&](const QString&text,auto fn){auto*a=menu->addAction(text);connect(a,&QAction::triggered,this,fn);return a;};
    add("Complex 제작 도우미...",[this]{insertCoordinationDialog();});add("Fragment 삽입...",[this]{openFragmentBrowser();});
    add("Ligands for BIOCC...",[this]{openFragmentBrowser(true);});
    auto *replaceLigand=menu->addAction(QStringLiteral("ligand 자동 인식/교체..."));
    replaceLigand->setObjectName("ReplaceLigandTemplate");
    connect(replaceLigand,&QAction::triggered,this,&MainWindow::replaceLigandTemplate);
    add("템플릿 저장...",[this]{saveLigandTemplate();});
    add("Complex 템플릿 적용...",[this]{buildLabComplex();});
    add("Scan용 구조 조정...",[this]{arrangeScanApproach();});menu->addSeparator();
    for(auto pair:{qMakePair(QString("수소 보정"),HydrogenAction::Adjust),qMakePair(QString("수소 추가"),HydrogenAction::Add),qMakePair(QString("과잉 수소 제거"),HydrogenAction::RemoveExtra),qMakePair(QString("모든 수소 제거"),HydrogenAction::RemoveAll)}) {
        auto *action=add(pair.first,[this,pair]{auto edited=m_view->molecule();MoleculeEditController::hydrogens(edited,pair.second);commitEditedMolecule(edited,true,false);});
        if(pair.second==HydrogenAction::Adjust)action->setStatusTip(QStringLiteral("결합 차수와 원자가를 기준으로 수소 개수와 배치를 다시 계산합니다. 고정 원자·금속 결합 수소는 유지합니다."));
        if(pair.second==HydrogenAction::Add)action->setStatusTip(QStringLiteral("기존 수소의 개수와 위치를 유지하고, 원자가에 부족한 수소만 추가합니다."));
    }
    for (auto pair : {qMakePair(QStringLiteral("선택 원자 수소 추가"), HydrogenAction::Add),
                      qMakePair(QStringLiteral("선택 원자 수소 보정"), HydrogenAction::Adjust)}) {
        auto *action = add(pair.first, [this, pair] {
            auto selected = m_view->selection().atoms;
            if (selected.isEmpty() && m_view->selectedAtom() >= 0) selected.insert(m_view->selectedAtom());
            bool hasHeavy = false;
            for (int i : selected) if (i >= 0 && i < m_view->molecule().atoms.size() &&
                                      m_view->molecule().atoms[i].atomicNumber != 1) hasHeavy = true;
            if (!hasHeavy) { setStatus(QStringLiteral("수소를 처리할 원자를 먼저 선택하세요.")); return; }
            auto edited = m_view->molecule();
            MoleculeEditController::hydrogens(edited, pair.second, selected);
            commitEditedMolecule(edited, true, false);
            setStatus(QStringLiteral("선택 원자의 수소를 처리했습니다. Ctrl+Z로 되돌릴 수 있습니다."));
        });
        action->setObjectName(pair.second == HydrogenAction::Add ? "AddHydrogensToSelection" : "AdjustHydrogensOnSelection");
        action->setStatusTip(QStringLiteral("선택한 원자에만 적용합니다. 고정 원자와 잠긴 원자, 금속 결합 수소는 유지합니다."));
    }
    for(auto pair:{qMakePair(QStringLiteral("탄소에 붙은 수소 제거"),HydrogenAction::RemoveCarbonBound),qMakePair(QStringLiteral("선택한 수소만 남기기"),HydrogenAction::RemoveExceptSelected)}) {
        auto *action=menu->addAction(pair.first);action->setObjectName(pair.second==HydrogenAction::RemoveCarbonBound?"RemoveCarbonHydrogens":"RemoveUnselectedHydrogens");
        connect(action,&QAction::triggered,this,[this,pair]{
            auto selected=m_view->selection().atoms;if(selected.isEmpty()&&m_view->selectedAtom()>=0)selected.insert(m_view->selectedAtom());
            if(pair.second==HydrogenAction::RemoveExceptSelected) {
                bool hasHydrogen=false;for(int i:selected)if(i>=0&&i<m_view->molecule().atoms.size()&&m_view->molecule().atoms[i].atomicNumber==1)hasHydrogen=true;
                if(!hasHydrogen){setStatus(QStringLiteral("남길 수소를 먼저 선택하세요. Shift/Ctrl로 여러 수소를 선택할 수 있습니다."));return;}
            }
            auto edited=m_view->molecule();const int before=edited.atoms.size();MoleculeEditController::hydrogens(edited,pair.second,selected);
            if(before==edited.atoms.size()){setStatus(QStringLiteral("제거할 수소가 없습니다."));return;}
            const int removed=before-edited.atoms.size();commitEditedMolecule(edited,true,false);setStatus(QStringLiteral("수소 %1개를 제거했습니다. Ctrl+Z로 되돌릴 수 있습니다.").arg(removed));
        });
    }
    menu->addSeparator();add("원소 변경...",[this]{changeSelectionElement();});add("선택 원자 이동 / 회전...",[this]{manipulateSelection();});
    add("선택 원자 사이에 결합 추가",[this]{auto ids=m_view->selection().atoms.values();if(ids.size()!=2){setStatus("원자 두 개를 선택하세요.");return;}auto edited=m_view->molecule();MoleculeEditController::drawBond(edited,ids[0],ids[1],drawState());commitEditedMolecule(edited,true);});
    add("선택 결합 제거",[this]{auto edited=m_view->molecule();auto ids=m_view->selection().atoms;auto bs=m_view->selection().bonds;for(int i=edited.bonds.size()-1;i>=0;--i){auto b=edited.bonds[i];if(bs.contains(i)||(ids.contains(b.first)&&ids.contains(b.second)))edited.bonds.removeAt(i);}commitEditedMolecule(edited,true);});
    add("결합 재계산 / 인식...",[this]{recountBonds(false);});
    auto *centric=menu->addAction("결합 중심 편집");centric->setCheckable(true);connect(centric,&QAction::toggled,this,[this](bool on){if(on){ensureEditableDocument();setDrawingMode(false);setAtomDragMode(false);}m_view->setBondCentricMode(on);setStatus("결합 중심 편집: 결합 클릭 · 끝점 드래그: 각도 · 오른쪽 드래그: 길이 · Ctrl 드래그: 비틀림 · Shift: 15° 단위 · Alt: 반대쪽",0);});
}
}
