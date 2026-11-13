#include "app/MainWindow.h"
#include "analysis/AnalysisEngine.h"
#include <algorithm>
#include "model/StructureTools.h"
#include "render/MoleculeView.h"
#include "animation/AnimationController.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace youngcraft {
namespace {
constexpr auto fragmentMime = "application/x-youngcraft-fragment-v1";
bool saveText(QWidget *parent, const QString &text, const QString &filter, QString *error) {
    const auto path = QFileDialog::getSaveFileName(parent, QStringLiteral("구조 데이터 저장"), {}, filter);
    if (path.isEmpty()) return false;
    QSaveFile file(path); const auto bytes = text.toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        *error = file.errorString(); return false;
    }
    return true;
}
void textDialog(QWidget *parent, const QString &title, const QString &text, const QString &filter) {
    QDialog dialog(parent); dialog.setObjectName("StructureTextDialog"); dialog.setWindowTitle(title); dialog.resize(720,540);
    QVBoxLayout layout(&dialog);
    QPlainTextEdit preview; preview.setObjectName("StructureTextPreview"); preview.setReadOnly(true); preview.setPlainText(text); layout.addWidget(&preview);
    QLabel status; status.setWordWrap(true); layout.addWidget(&status);
    QDialogButtonBox buttons(QDialogButtonBox::Close);
    auto *copy = buttons.addButton(QStringLiteral("복사"),QDialogButtonBox::ActionRole);
    auto *save = buttons.addButton(QStringLiteral("저장..."),QDialogButtonBox::ActionRole);
    layout.addWidget(&buttons);
    QObject::connect(copy,&QPushButton::clicked,&dialog,[&]{QApplication::clipboard()->setText(text);status.setText(QStringLiteral("텍스트를 복사했습니다."));});
    QObject::connect(save,&QPushButton::clicked,&dialog,[&]{QString error;if(saveText(&dialog,text,filter,&error))status.setText(QStringLiteral("저장했습니다."));else status.setText(error);});
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.exec();
}
}
QSet<int> MainWindow::selectedStructureAtoms() const {
    auto ids = StructureTools::selection(m_view->molecule(),m_view->selection().atoms);
    // Bond selection is also a usable fragment selection.
    for (int i : m_view->selection().bonds) if (i >= 0 && i < m_view->molecule().bonds.size()) {
        const auto &b = m_view->molecule().bonds[i]; ids.insert(b.first); ids.insert(b.second);
    }
    return ids;
}
void MainWindow::applyAtomSelection(const QSet<int> &ids) {
    const auto selection=ids;
    clearGeometrySelection(); m_view->selection().clear();
    m_view->setSelectedAtom(-1);
    m_view->selection().atoms = StructureTools::selection(m_view->molecule(),selection);
    emit m_view->atomSelectionChanged();
    m_view->update();
    setStatus(QStringLiteral("원자 %1개 선택 · Shift: 추가 · Ctrl: 선택 반전 · 빈 곳 드래그: 영역 선택").arg(m_view->selection().atoms.size()));
}
void MainWindow::copyStructureFragment() {
    if (isTextInputFocused()) { if(auto *line=qobject_cast<QLineEdit *>(QApplication::focusWidget()))line->copy();else if(auto *plain=qobject_cast<QPlainTextEdit *>(QApplication::focusWidget()))plain->copy();else if(auto *rich=qobject_cast<QTextEdit *>(QApplication::focusWidget()))rich->copy();return; }
    const auto ids = selectedStructureAtoms();
    if (ids.isEmpty()) { setStatus(QStringLiteral("복사할 원자 또는 결합을 먼저 선택하세요.")); return; }
    const auto fragment = StructureTools::fragment(m_view->molecule(),ids);
    auto *mime = new QMimeData; mime->setData(fragmentMime,StructureTools::encode(fragment)); mime->setText(StructureTools::xyz(fragment));
    QApplication::clipboard()->setMimeData(mime);
    setStatus(QStringLiteral("원자 %1개와 내부 결합 %2개를 복사했습니다. Ctrl+V로 붙여넣으세요.").arg(fragment.atoms.size()).arg(fragment.bonds.size()));
}
void MainWindow::pasteStructureFragment() {
    if (isTextInputFocused()) { if(auto *line=qobject_cast<QLineEdit *>(QApplication::focusWidget()))line->paste();else if(auto *plain=qobject_cast<QPlainTextEdit *>(QApplication::focusWidget()))plain->paste();else if(auto *rich=qobject_cast<QTextEdit *>(QApplication::focusWidget()))rich->paste();return; }
    const auto *mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasFormat(fragmentMime)) { setStatus(QStringLiteral("클립보드에 Youngcraft molecule fragment이 없습니다. 선택 fragment을 먼저 복사하세요.")); return; }
    Molecule fragment; QString error;
    if (!StructureTools::decode(mime->data(fragmentMime),fragment,&error)) { setStatus(error); return; }
    m_animation->stop(); m_view->setFrequencyMode(nullptr);
    auto edited = m_view->molecule(); const int base = edited.atoms.size(); QVector3D offset;
    // Place beside the current structure, retaining the copied internal geometry.
    if (!edited.isEmpty()) {
        float maxX=edited.atoms.first().position.x(),minX=fragment.atoms.first().position.x();
        QVector3D sourceCenter,targetCenter;
        for(const auto &a:edited.atoms){maxX=qMax(maxX,a.position.x());targetCenter+=a.position;}
        for(const auto &a:fragment.atoms){minX=qMin(minX,a.position.x());sourceCenter+=a.position;}
        targetCenter/=edited.atoms.size();sourceCenter/=fragment.atoms.size();
        offset={maxX-minX+3.0f,targetCenter.y()-sourceCenter.y(),targetCenter.z()-sourceCenter.z()};
    }
    if (!StructureTools::append(edited,fragment,offset,&error)) { setStatus(error); return; }
    commitEditedMolecule(edited,true,false);
    QSet<int> pasted;for(int i=base;i<edited.atoms.size();++i)pasted.insert(i);
    applyAtomSelection(pasted);m_view->bestView();
    setStatus(QStringLiteral("원자 %1개를 붙여넣었습니다. 실행 취소(Ctrl+Z)로 복원할 수 있습니다.").arg(fragment.atoms.size()));
}
void MainWindow::extractStructureFragments() {
    const auto molecule=m_view->molecule();
    if(molecule.isEmpty()){setStatus(QStringLiteral("molecule 구조를 먼저 여세요."));return;}
    const auto groups=StructureTools::components(molecule);
    QDialog dialog(this);dialog.setObjectName("FragmentExtractor");dialog.setWindowTitle(QStringLiteral("molecule fragment 추출기"));dialog.resize(580,420);QVBoxLayout layout(&dialog);
    QLabel help(QStringLiteral("현재 결합 그래프로 분리된 fragment입니다. 추출할 fragment을 선택하세요.\n부분 fragment의 총전하는 형식 전하 합을 사용하며 스핀 다중도는 후속 계산에서 확인하세요."));help.setWordWrap(true);layout.addWidget(&help);
    QListWidget list;list.setObjectName("ExtractFragmentList");list.setSelectionMode(QAbstractItemView::ExtendedSelection);
    QVector<QSet<int>> choices;
    const auto selected=selectedStructureAtoms();
    if(!selected.isEmpty()){choices<<selected;list.addItem(QStringLiteral("현재 선택 영역 — 원자 %1개").arg(selected.size()));}
    for(int i=0;i<groups.size();++i){choices<<groups[i];list.addItem(QStringLiteral("분리 fragment %1 — 원자 %2개").arg(i+1).arg(groups[i].size()));}
    layout.addWidget(&list);if(selected.isEmpty())list.selectAll();else{list.setCurrentRow(0);list.item(0)->setSelected(true);}
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("새 탭으로 추출"));buttons.button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));layout.addWidget(&buttons);
    connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    QVector<Molecule> fragments;for(auto *item:list.selectedItems())fragments<<StructureTools::fragment(molecule,choices[list.row(item)]);
    for(int i=0;i<fragments.size();++i){newDrawingDocument(fragments[i]);setDrawingMode(false);m_documentTabs->setTabText(m_documentTabs->currentIndex(),QStringLiteral("추출 fragment %1").arg(i+1));}
    setStatus(QStringLiteral("%1개 fragment을 새 탭으로 추출했습니다.").arg(fragments.size()));
}
void MainWindow::showMolecularInformation() {
    const auto &molecule=m_view->molecule();
    const auto formula=AnalysisEngine::molecularFormula(molecule);
    QDialog dialog(this);dialog.setObjectName("MolecularInformationDialog");
    dialog.setWindowTitle(QStringLiteral("화학식 · molecular mass"));
    QVBoxLayout layout(&dialog);QFormLayout form;
    QLabel formulaLabel(formula.isEmpty()?QStringLiteral("—"):formula);
    formulaLabel.setWordWrap(true);formulaLabel.setObjectName("MolecularFormula");formulaLabel.setTextInteractionFlags(Qt::TextSelectableByMouse);
    const bool knownMass=std::all_of(molecule.atoms.cbegin(),molecule.atoms.cend(),[](const Atom &atom){return atom.atomicNumber==0||atomicMass(atom.atomicNumber)>0;});
    QLabel massLabel(knownMass&&!formula.isEmpty()?QStringLiteral("%1 g/mol").arg(AnalysisEngine::molecularMass(molecule),0,'f',4):QStringLiteral("—"));
    massLabel.setObjectName("MolecularMass");massLabel.setTextInteractionFlags(Qt::TextSelectableByMouse);
    form.addRow(QStringLiteral("전체 화학식"),&formulaLabel);form.addRow(QStringLiteral("전체 molecular mass"),&massLabel);
    layout.addLayout(&form);
    QLabel note(formula.isEmpty()?QStringLiteral("molecule 구조를 먼저 여세요."):!knownMass?QStringLiteral("표준 원자량이 없는 원소는 동위원소 질량 지정이 필요합니다."):QStringLiteral("숨긴 원자를 포함한 전체 구조 기준"));
    note.setWordWrap(true);
    layout.addWidget(&note);
    QDialogButtonBox buttons(QDialogButtonBox::Ok);buttons.button(QDialogButtonBox::Ok)->setText(QStringLiteral("확인"));
    layout.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    dialog.exec();
}

void MainWindow::showZMatrix() {
    QString error;const auto text=StructureTools::zMatrix(m_view->molecule(),&error);
    if(text.isEmpty()){setStatus(error.isEmpty()?QStringLiteral("molecule 구조를 먼저 여세요."):error,10000);return;}
    textDialog(this,QStringLiteral("Z-matrix · 거리 Å / 각도 °"),text,QStringLiteral("Z-matrix (*.zmat);;텍스트 (*.txt)"));
}
void MainWindow::createGaussianInput() {
    const auto molecule=m_view->molecule();if(molecule.isEmpty()){setStatus(QStringLiteral("molecule 구조를 먼저 여세요."));return;}
    QDialog dialog(this);dialog.setObjectName("GaussianInputDialog");dialog.setWindowTitle(QStringLiteral("Gaussian 입력파일 생성"));dialog.resize(760,640);QVBoxLayout layout(&dialog);QFormLayout form;
    QLineEdit route(m_data.route.isEmpty()?QStringLiteral("#p B3LYP/6-31G(d) opt"):m_data.route.simplified());route.setObjectName("GaussianRoute");
    QSpinBox charge,multiplicity;charge.setObjectName("GaussianCharge");multiplicity.setObjectName("GaussianMultiplicity");charge.setRange(-1000,1000);charge.setValue(molecule.charge);multiplicity.setRange(1,1000);multiplicity.setValue(molecule.multiplicity);
    form.addRow(QStringLiteral("계산 경로 (직접 확인)"),&route);form.addRow(QStringLiteral("총전하"),&charge);form.addRow(QStringLiteral("스핀 다중도"),&multiplicity);layout.addLayout(&form);
    QLabel help(QStringLiteral("현재 화면 좌표로 독립적인 Cartesian 입력을 생성합니다. 기본 경로는 예시입니다. 금속 착물의 전하·스핀·방법·기저함수를 확인하세요. Gen/GenECP 등 추가 구역은 지원하지 않습니다."));help.setWordWrap(true);layout.addWidget(&help);
    QPlainTextEdit preview;preview.setObjectName("GaussianInputPreview");preview.setReadOnly(true);layout.addWidget(&preview);QLabel status;status.setWordWrap(true);layout.addWidget(&status);
    QDialogButtonBox buttons(QDialogButtonBox::Close);auto *copy=buttons.addButton(QStringLiteral("복사"),QDialogButtonBox::ActionRole);auto *save=buttons.addButton(QStringLiteral("GJF 저장..."),QDialogButtonBox::ActionRole);layout.addWidget(&buttons);
    auto refresh=[&]{QString error;auto text=StructureTools::gaussianInput(molecule,route.text(),charge.value(),multiplicity.value(),&error);preview.setPlainText(text);status.setText(error);copy->setEnabled(!text.isEmpty());save->setEnabled(!text.isEmpty());};
    connect(&route,&QLineEdit::textChanged,&dialog,[&]{refresh();});connect(&charge,qOverload<int>(&QSpinBox::valueChanged),&dialog,[&]{refresh();});connect(&multiplicity,qOverload<int>(&QSpinBox::valueChanged),&dialog,[&]{refresh();});
    connect(copy,&QPushButton::clicked,&dialog,[&]{QApplication::clipboard()->setText(preview.toPlainText());status.setText(QStringLiteral("입력파일을 복사했습니다."));});
    connect(save,&QPushButton::clicked,&dialog,[&]{QString error;if(saveText(&dialog,preview.toPlainText(),QStringLiteral("Gaussian 입력 (*.gjf);;Gaussian 입력 (*.com)"),&error))status.setText(QStringLiteral("입력파일을 저장했습니다."));else status.setText(error);});
    connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);refresh();dialog.exec();
}
}
