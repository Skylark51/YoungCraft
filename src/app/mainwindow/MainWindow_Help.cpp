#include "app/MainWindow.h"
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSettings>
#include <QSplitter>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace youngcraft {
namespace {
struct HelpTopic { int category; QString ko,en,descriptionKo,descriptionEn,shortcut,action; };
const QVector<QPair<QString,QString>> categories={
    {QStringLiteral("파일 · 프로젝트"),"Files and projects"},
    {QStringLiteral("선택 · 화면 조작"),"Selection and navigation"},
    {QStringLiteral("그리기 · 원자 · 결합"),"Drawing, atoms and bonds"},
    {QStringLiteral("Fragments / ligands"),"Fragments / ligands"},
    {QStringLiteral("거리 · 각도 · 이면각"),"Distance, angle and dihedral"},
    {QStringLiteral("MM · QM 최적화"),"MM and QM optimization"},
    {QStringLiteral("계산 결과 · 스펙트럼"),"Results and spectra"},
    {QStringLiteral("Orbitals · 전자 구조"),"Orbitals and electronic structure"},
    {QStringLiteral("캡처 · 내보내기"),"Capture and export"},
    {QStringLiteral("애니메이션 제작"),"Animation authoring"},
    {QStringLiteral("표시 · 테마 · 패널"),"Display, themes and panels"}
};
QVector<HelpTopic> helpTopics() {
    QVector<HelpTopic> topics;
    auto add=[&](int category,const char *ko,const char *en,const char *dk,const char *de,const char *shortcut="",const char *action=""){
        topics.append({category,QString::fromUtf8(ko),QString::fromUtf8(en),QString::fromUtf8(dk),QString::fromUtf8(de),QString::fromUtf8(shortcut),QString::fromUtf8(action)});
    };
    add(0,"새 molecule","New molecule","새 탭에서 molecule을 그릴 수 있습니다.","You can draw a molecule in a new tab.","Ctrl+N","NewStructure");
    add(0,"파일 열기","Open file","구조 파일과 계산 결과를 열 수 있습니다.","You can open structure files and calculation outputs.","Ctrl+O","OpenStructureFile");
    add(0,"프로젝트 · Reaction Pathway","Projects and reaction pathways","계산 파일을 프로젝트와 반응 경로별로 관리할 수 있습니다.","You can organize calculations into projects and reaction pathways.","Ctrl+Shift+P","ProjectLibraryAction");
    add(0,"저장","Save","현재 구조를 XYZ로 저장하거나 이미지를 내보낼 수 있습니다.","You can save the current structure as XYZ or export an image.","Ctrl+S","SaveStructureImage");
    add(0,"최근 파일 · 여러 파일","Recent and multiple files","최근 파일을 다시 열거나 여러 파일을 탭으로 열 수 있습니다.","You can reopen recent files or open several files in separate tabs.");
    add(0,"새 창 · 탭 분리","New window and detached tabs","새 창을 열거나 현재 탭을 별도 창으로 옮길 수 있습니다.","You can open a new window or move a tab to another window.","Ctrl+Shift+N");
    add(0,"탭 닫기","Close tab","현재 문서 탭을 닫을 수 있습니다.","You can close the current document tab.","Ctrl+W");
    add(0,"다음 탭","Next tab","다음 문서 탭으로 전환할 수 있습니다.","You can switch to the next document tab.","Ctrl+Tab");
    add(1,"원자 선택","Select atoms","원자를 차례로 클릭해 개수 제한 없이 선택하고 다시 클릭해 해제할 수 있습니다.","You can select any number of atoms by clicking and click again to deselect.");
    add(1,"영역 선택","Rectangle selection","빈 곳에서 드래그하여 영역 안의 원자를 선택할 수 있습니다.","You can drag from empty space to select atoms inside a rectangle.");
    add(1,"선택 추가 · 반전","Add and toggle selection","Shift로 선택을 추가하고 Ctrl로 선택 여부를 바꿀 수 있습니다.","You can hold Shift to add atoms or Ctrl to toggle selection.","Shift / Ctrl + click");
    add(1,"연결된 원자 선택","Select connected atoms","원자를 두 번 클릭해 연결된 molecule을 선택할 수 있습니다.","You can double-click an atom to select its connected molecule.","Double-click");
    add(1,"전체 선택","Select all","현재 보이는 원자를 모두 선택할 수 있습니다.","You can select all visible atoms.","Ctrl+A","SelectAllVisibleAtoms");
    add(1,"원소 선택 · 확장 · 반전","Element selection, expansion and inversion","원소별로 선택하거나 결합 이웃으로 선택을 넓히고 반전할 수 있습니다.","You can select by element, expand to bonded neighbors, or invert the selection.");
    add(1,"선택 해제","Clear selection","빈 곳을 클릭하거나 빈 곳을 우클릭해 선택을 해제할 수 있습니다.","You can click or right-click empty space to clear the selection.");
    add(1,"화면 회전","Rotate view","오른쪽 버튼을 누르고 드래그하여 시점을 회전할 수 있습니다.","You can drag with the right mouse button to rotate the view.","Right-drag");
    add(1,"화면 이동","Pan view","가운데 버튼 또는 Ctrl과 오른쪽 버튼으로 화면을 이동할 수 있습니다.","You can pan with the middle button or Ctrl and the right button.","Middle-drag / Ctrl+Right-drag");
    add(1,"확대 · 축소","Zoom","마우스 휠로 molecule을 확대하거나 축소할 수 있습니다.","You can use the mouse wheel to zoom in or out.","Mouse wheel");
    add(1,"베스트 뷰","Best view","molecule 전체가 보이도록 화면을 맞출 수 있습니다.","You can fit the entire molecule into the viewport.","Enter","BestStructureView");
    add(1,"중앙 정렬","Center view","molecule을 화면 중앙에 배치할 수 있습니다.","You can center the molecule in the viewport.","Space","CenterStructureView");
    add(1,"회전 중심 갱신","Update rotation center","현재 원자를 기준으로 회전 중심을 갱신할 수 있습니다.","You can update the rotation center from the current atoms.","Num5");
    add(2,"원자 · 결합 그리기","Draw atoms and bonds","원소를 고른 뒤 빈 곳을 클릭하거나 드래그해 원자와 결합을 만들 수 있습니다.","You can choose an element and click or drag to create atoms and bonds.");
    add(2,"연속 그리기","Continuous drawing","빈 곳을 연속 클릭하여 사슬을 연장할 수 있습니다.","You can click empty space repeatedly to extend a chain.");
    add(2,"원소 단축키","Element shortcuts","원자 위에 커서를 놓고 c, n, o 등을 입력해 원소를 바꿀 수 있습니다.","You can hover over an atom and type c, n, o, or another element symbol to replace it.","C / N / O / …");
    add(2,"염소로 변경","Replace with chlorine","원자 위에서 c를 빠르게 두 번 누르거나 cl을 입력해 Cl로 바꿀 수 있습니다.","You can type c twice quickly, or type cl, over an atom to change it to Cl.","C, C / C, L");
    add(2,"주기율표","Periodic table","금속·비금속·준금속 색상을 참고해 원소를 선택할 수 있습니다.","You can choose elements using the metal, nonmetal and metalloid colors.");
    add(2,"결합 차수","Bond order","단일·이중·삼중 결합과 방향족·자동 결합 차수를 지정할 수 있습니다.","You can choose single, double, triple, aromatic or automatic bond orders.","1 / 2 / 3 / A");
    add(2,"자유 배치","Free placement","그리기 중 Alt를 눌러 결합 길이와 각도 스냅을 해제할 수 있습니다.","You can hold Alt while drawing to bypass bond length and angle snapping.","Alt");
    add(2,"사슬 종료 · 도구 해제","End chain and disarm drawing","Esc로 사슬을 끝내고 빈 곳 우클릭으로 그리기 도구를 해제할 수 있습니다.","You can press Esc to end a chain or right-click empty space to disarm drawing.","Esc");
    add(2,"형식 전하 · 자동 수소","Formal charge and automatic hydrogens","원자 전하를 지정하고 편집 시 부족한 수소를 자동 보정할 수 있습니다.","You can set formal charges and automatically reconcile hydrogens when editing.");
    add(2,"실행 취소","Undo","직전 구조 편집을 되돌릴 수 있습니다.","You can undo the previous structure edit.","Ctrl+Z");
    add(2,"다시 실행","Redo","실행 취소한 구조 편집을 다시 적용할 수 있습니다.","You can redo a structure edit.","Ctrl+Y / Ctrl+Shift+Z");
    add(2,"선택 삭제","Delete selection","선택한 원자와 결합을 삭제할 수 있습니다.","You can delete selected atoms and bonds.","Delete");
    add(2,"선택 결합 추가 · 삭제","Toggle selected bond","선택한 두 원자 사이의 결합을 추가하거나 삭제할 수 있습니다.","You can add or remove the bond between two selected atoms.","Ctrl+B");
    add(2,"결합 재계산","Recalculate bonds","좌표에서 결합을 다시 인식하거나 모든 결합을 지울 수 있습니다.","You can infer bonds again from coordinates or clear all bonds.");
    add(2,"원자 이동","Move atoms","이동 도구를 켜고 선택한 원자를 드래그할 수 있습니다.","You can enable the move tool and drag selected atoms.","Ctrl+D");
    add(2,"Fragment · molecule 이동과 회전","Move and rotate fragments or molecules","정밀 편집 도구로 가지 각도·비틀림과 molecule 이동·회전을 조절할 수 있습니다.","You can adjust branch angles and torsions, or move and rotate a molecule with precision tools.");
    add(3,"Fragments / ligands 삽입","Insert fragments / ligands","템플릿을 배치하거나 기존 원자·배위 자리에 연결할 수 있습니다.","You can place templates or connect them to atoms and coordination sites.");
    add(3,"Lab ligands · 금속 배위","Lab ligands and metal coordination","금속 중심·ligand·산소·보조 ligand를 조합할 수 있습니다.","You can combine metal centers, ligands, oxygen groups and coligands.");
    add(3,"Ligand 인식 · 교체","Recognize and replace ligands","금속에 연결된 ligand를 확인하고 다른 템플릿으로 교체할 수 있습니다.","You can identify metal-bound ligands and replace them with another template.");
    add(3,"사용자 ligand 저장","Save a custom ligand","선택한 ligand와 기준 금속 위치를 템플릿으로 저장할 수 있습니다.","You can save a selected ligand and its metal reference as a reusable template.");
    add(3,"Fragment 복사","Copy fragment","선택한 원자와 결합을 구조 fragment로 복사할 수 있습니다.","You can copy selected atoms and bonds as a structure fragment.","Ctrl+C","CopyStructureFragment");
    add(3,"Fragment 붙여넣기","Paste fragment","복사한 fragment를 현재 molecule에 추가할 수 있습니다.","You can add a copied fragment to the current molecule.","Ctrl+V","PasteStructureFragment");
    add(3,"Fragment 추출","Extract fragments","결합 그래프로 분리된 fragment를 새 탭으로 열 수 있습니다.","You can open disconnected fragments in separate tabs.");
    add(4,"거리 · 각도 · 이면각 측정","Measure distance, angle and dihedral","앞에서 선택한 2·3·4개 원자의 거리·각도·이면각을 확인할 수 있습니다.","You can read the distance, angle and dihedral of the first two, three and four selected atoms.");
    add(4,"측정값 직접 편집","Edit a measured value","측정 초기화 왼쪽의 편집 버튼에서 수치를 입력하고 적용할 수 있습니다.","You can enter and apply a value using the edit button left of measurement reset.");
    add(4,"이동 방향","Movement direction","첫 원자 쪽·끝 원자 쪽 또는 양쪽 fragment를 움직일 수 있습니다.","You can move the first side, the last side, or both connected fragments.");
    add(4,"측정 표시 · 스타일","Measurement visibility and style","측정값을 화면에 표시하고 글꼴과 색상을 바꿀 수 있습니다.","You can show measurements in the viewport and change their font and color.");
    add(4,"Scan 시작 구조","Prepare a scan approach","반응 원자 두 개를 선택해 fragment 방향과 시작 거리를 맞출 수 있습니다.","You can select two reaction atoms to arrange fragment orientations and starting separation.");
    add(4,"결합 중심 편집","Bond-centric editing","결합 끝점을 드래그해 각도를, Ctrl로 비틀림을, 오른쪽 버튼으로 거리를 바꿀 수 있습니다.","You can drag bond ends for angles, hold Ctrl for torsions, or use the right button for distances.","Ctrl / Alt + drag");
    add(5,"MM 최적화","MM optimization","상단 MM 아이콘에서 오른쪽 패널을 열고 힘장과 반복 횟수를 지정할 수 있습니다.","You can open the right panel from the top MM icon and choose a force field and iteration limit.","","MMOptimizationAction");
    add(5,"금속 MM · GFN-FF","Metal MM and GFN-FF","Auto로 금속 구조를 GFN-FF에 전달하거나 GFN-FF를 직접 선택할 수 있습니다.","You can use Auto for GFN-FF metal optimization, or select GFN-FF directly.");
    add(5,"QM · xTB","QM and xTB","오른쪽 패널에서 GFN1·GFN2-xTB, 전하, multiplicity와 용매를 지정할 수 있습니다.","You can choose GFN1 or GFN2-xTB, charge, multiplicity and solvent in the right panel.","","QMOptimizationAction");
    add(5,"선택 원자 고정","Freeze selected atoms","드래그로 원자를 선택한 뒤 고정 버튼을 눌러 나머지 원자만 최적화할 수 있습니다.","You can select atoms by dragging, freeze them, and optimize the remaining atoms.");
    add(5,"고정 유지 · 해제","Retain and release fixed atoms","정밀 편집의 자물쇠 도구로 고정을 저장하고 고정 해제로 풀 수 있습니다.","You can retain fixed atoms with the precision-edit lock tool and release them with unfreeze.");
    add(5,"거리 · 각도 · 이면각 조건","Geometry restraints","Constraint에서 목표 거리·각도·이면각을 지정할 수 있습니다.","You can add target distance, angle and dihedral restraints in Constraint.");
    add(5,"결합 거리 유지","Preserve bond distances","초기 결합 길이에 조화 restraint를 적용해 구조를 정리할 수 있습니다.","You can relax a structure with harmonic restraints on its initial bond lengths.");
    add(5,"진행 확인 · 중단 · 복원","Monitor, stop and restore","에너지 그래프를 확인하고 MM을 일시정지·중단하거나 시작 구조로 복원할 수 있습니다.","You can monitor energy, pause or stop MM, and restore the starting geometry.");
    add(5,"QM 결과 적용 · 저장","Apply and save QM results","최적화 결과를 현재 구조에 적용하거나 새 탭으로 열고 XYZ로 저장할 수 있습니다.","You can apply an optimized geometry, open it in a new tab, or save it as XYZ.");
    add(6,"Opt · Scan · IRC","Opt, scan and IRC","그래프에서 단계를 골라 에너지와 해당 구조를 확인할 수 있습니다.","You can select graph steps to inspect their energies and geometries.","Left / Right");
    add(6,"Freq · 진동 벡터","Frequencies and vibration vectors","진동 모드를 선택하고 재생 속도·변위·벡터 길이·색 대비를 조절할 수 있습니다.","You can choose modes and adjust playback, displacement, vector length and contrast.","P");
    add(6,"TD-DFT 스펙트럼","TD-DFT spectra","전자 전이와 broadened 스펙트럼을 확인하고 그래프 스타일을 바꿀 수 있습니다.","You can inspect electronic transitions and broadened spectra, and change graph styling.");
    add(6,"원자 속성 · XYZ","Atomic properties and XYZ","원자별 계산 속성과 좌표를 확인하고 복사할 수 있습니다.","You can inspect and copy calculated atomic properties and coordinates.");
    add(6,"출력 검색","Search output","계산 원문을 열고 원하는 문자열을 검색할 수 있습니다.","You can open the calculation output and search for text.","Ctrl+F");
    add(6,"검색 결과 이동","Navigate search matches","검색 입력란에서 Enter로 다음 결과, Shift+Enter로 이전 결과로 이동할 수 있습니다.","You can press Enter in the search field for the next match or Shift+Enter for the previous match.","Enter / Shift+Enter");
    add(6,"계산 출력 탐색","Browse calculation output","원문 출력에서 계산 구간을 찾아 이동하고 검색할 수 있습니다.","You can navigate calculation sections and search the original output.");
    add(6,"화학식 · Molecular mass","Formula and molecular mass","전체 molecule의 화학식과 질량을 확인할 수 있습니다.","You can inspect the formula and mass of the entire molecule.");
    add(7,"Orbitals · FMO","Orbitals and FMO","HOMO·LUMO 등 orbital과 에너지 준위를 확인할 수 있습니다.","You can inspect orbitals such as HOMO and LUMO and their energy levels.");
    add(7,"MEP · NBO · Wiberg","MEP, NBO and Wiberg","계산 자료의 정전기 퍼텐셜·자연 결합·결합 지수 정보를 확인할 수 있습니다.","You can inspect electrostatic potential, natural bonding and bond indices from available data.");
    add(7,"Cube · 등밀도면","Cube and isosurfaces","Cube 자료를 불러와 등밀도면 값·색상·투명도를 바꿀 수 있습니다.","You can load cube data and adjust isovalues, colors and opacity.");
    add(7,"원격 계산 · 캐시","Remote calculations and cache","연결한 서버에서 orbital 자료를 만들고 캐시 결과를 다시 사용할 수 있습니다.","You can generate orbital data on a connected server and reuse cached results.");
    add(8,"이미지 복사","Copy image","현재 molecule 이미지를 클립보드로 복사할 수 있습니다.","You can copy the current molecule image to the clipboard.","Ctrl+I","CopyStructureImage");
    add(8,"고해상도 그림","High-resolution image","해상도·배경·출력 형식을 지정해 이미지를 저장할 수 있습니다.","You can export an image with a chosen resolution, background and format.","Ctrl+Q");
    add(8,"열린 탭 비교 캡처","Compare open tabs","여러 탭의 구조와 설명을 한 이미지에 배치할 수 있습니다.","You can arrange structures and captions from multiple tabs in one image.","","OpenMultiCapture");
    add(8,"IRC · TS 맞춤 캡처","Custom IRC and TS capture","IRC·TS 구조를 골라 반응 경로 캡처를 만들 수 있습니다.","You can choose IRC and TS structures to create a reaction-path capture.","Ctrl+Alt+I","CustomStructureCapture");
    add(8,"TS 애니메이션 복사","Copy TS animation","선택한 진동 모드의 애니메이션을 복사할 수 있습니다.","You can copy an animation of the selected vibration mode.","Ctrl+Shift+I","CopyTSAnimation");
    add(8,"좌표 · Gaussian 입력 · Z-matrix","Coordinates, Gaussian input and Z-matrix","편집한 구조의 좌표와 계산 입력 자료를 만들 수 있습니다.","You can create coordinates and calculation input from the edited structure.");
    add(9,"애니메이션 창","Animation editor","상단 슬래시 오른쪽 아이콘에서 메커니즘 애니메이션 제작 창을 열 수 있습니다.","You can open the mechanism animation editor from the icon after the top-toolbar slash.","","OpenMechanismAnimation");
    add(9,"전체 화면 재생","Fullscreen playback","애니메이션 재생 창을 전체 화면으로 전환할 수 있습니다.","You can toggle fullscreen in the animation player.","F11");
    add(9,"구간 · 구조 · 재생","Stages, structures and playback","구조를 배치하고 구간 길이와 재생 범위를 지정할 수 있습니다.","You can arrange structures and set stage durations and playback ranges.");
    add(9,"카메라 · 강조 · 설명","Camera, emphasis and captions","장면별 시점과 강조 원자·설명을 조절할 수 있습니다.","You can adjust camera views, emphasized atoms and captions for each scene.");
    add(9,"프로젝트 · 영상 · PowerPoint","Projects, video and PowerPoint","제작 프로젝트를 저장하고 애니메이션과 PowerPoint로 내보낼 수 있습니다.","You can save animation projects and export animations and PowerPoint presentations.");
    add(10,"표시 스타일 · 원자 번호","Display styles and atom numbers","표시 방식·원자 번호·원자와 결합 크기를 바꿀 수 있습니다.","You can change rendering styles, atom numbers, and atom and bond sizes.");
    add(10,"수소 표시 · VdW","Hydrogen visibility and VdW","탄소에 결합한 수소를 숨기거나 반데르발스 구를 표시할 수 있습니다.","You can hide carbon-bound hydrogens or display van der Waals spheres.");
    add(10,"선택 원자 강조","Focus on selected atoms","주변 원자를 숨기거나 흐리게 표시하고 강조 상태를 복원할 수 있습니다.","You can hide or dim surrounding atoms and restore their display.");
    add(10,"뷰 동기화","Synchronize views","원자 수가 같은 탭의 회전·이동·확대 설정을 함께 바꿀 수 있습니다.","You can synchronize rotation, panning and zoom between tabs with equal atom counts.");
    add(10,"테마 · 배경 패턴","Themes and background patterns","팔레트와 배경 패턴을 선택할 수 있습니다.","You can choose a palette and background pattern.");
    add(10,"패널 표시 · 복원","Show and restore panels","상단 아이콘으로 좌우 패널을 표시하고 기본 배치를 복원할 수 있습니다.","You can show side panels from the top icons and restore their default arrangement.");
    add(10,"도움말 언어","Help language","상단 도움말에서 한국어와 영어 설명을 전환할 수 있습니다.","You can switch between Korean and English in the help window.","F1","YoungcraftHelpAction");
    return topics;
}
}
void MainWindow::showHelp() {
    if(auto *existing=findChild<QDialog *>("YoungcraftHelp")){existing->show();existing->raise();existing->activateWindow();return;}
    auto *dialog=new QDialog(this);dialog->setObjectName("YoungcraftHelp");dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->resize(940,650);
    auto *layout=new QVBoxLayout(dialog);auto *top=new QHBoxLayout;layout->addLayout(top);
    auto *search=new QLineEdit(dialog);search->setObjectName("HelpSearch");top->addWidget(search,1);
    auto *language=new QComboBox(dialog);language->setObjectName("HelpLanguage");language->addItems({QStringLiteral("한국어"),"English"});
    language->setCurrentIndex(QSettings().value("help/language",0).toInt()==1?1:0);top->addWidget(language);
    auto *split=new QSplitter(dialog);layout->addWidget(split,1);
    auto *tree=new QTreeWidget(split);tree->setObjectName("HelpCategories");tree->setHeaderHidden(true);tree->setMinimumWidth(220);
    auto *content=new QTextBrowser(split);content->setObjectName("HelpContent");content->setOpenExternalLinks(false);split->setSizes({320,600});
    const auto topics=helpTopics();
    const auto title=[this](const HelpTopic &topic,bool english){
        QString keys=topic.shortcut;
        if(!topic.action.isEmpty())if(auto *action=findChild<QAction *>(topic.action)){
            QStringList actual;for(const auto &shortcut:action->shortcuts())if(!shortcut.isEmpty())actual<<shortcut.toString(QKeySequence::NativeText);
            if(!actual.isEmpty())keys=actual.join(" / ");
        }
        return (english?topic.en:topic.ko)+(keys.isEmpty()?QString():QStringLiteral(" (%1)").arg(keys));
    };
    auto render=[topics,language,content,title](QTreeWidgetItem *item){
        if(!item){content->clear();return;}
        const bool en=language->currentIndex()==1;const int category=item->data(0,Qt::UserRole).toInt();
        const int selected=item->data(0,Qt::UserRole+1).toInt();
        QString html=QStringLiteral("<html><body style='font-family:Segoe UI,Malgun Gothic;font-size:11pt;color:#233747;background:#fcfdff;padding:12px'><h2>%1</h2>")
            .arg((en?categories[category].second:categories[category].first).toHtmlEscaped());
        for(int i=0;i<topics.size();++i)if(topics[i].category==category && (selected<0 || selected==i))
            html+=QStringLiteral("<h3 style='font-size:11pt;margin-bottom:4px'>%1</h3><p style='margin-top:0;margin-bottom:18px'>%2</p>")
                .arg(title(topics[i],en).toHtmlEscaped(),(en?topics[i].descriptionEn:topics[i].descriptionKo).toHtmlEscaped());
        content->setHtml(html+"</body></html>");
    };
    auto refresh=[=]{
        const bool en=language->currentIndex()==1;
        int oldCategory=0,oldTopic=-1;if(auto *current=tree->currentItem()){oldCategory=current->data(0,Qt::UserRole).toInt();oldTopic=current->data(0,Qt::UserRole+1).toInt();}
        tree->clear();dialog->setWindowTitle(en?QStringLiteral("Youngcraft Help"):QStringLiteral("Youngcraft 도움말"));
        search->setPlaceholderText(en?QStringLiteral("Search features or shortcuts"):QStringLiteral("기능 · 단축키 검색"));
        search->setAccessibleName(search->placeholderText());language->setAccessibleName(en?QStringLiteral("Help language"):QStringLiteral("도움말 언어"));
        const auto query=search->text().trimmed();QTreeWidgetItem *chosen=nullptr;
        for(int c=0;c<categories.size();++c){
            auto *root=new QTreeWidgetItem(tree,{en?categories[c].second:categories[c].first});root->setData(0,Qt::UserRole,c);root->setData(0,Qt::UserRole+1,-1);
            const bool categoryMatch=categories[c].first.contains(query,Qt::CaseInsensitive)||categories[c].second.contains(query,Qt::CaseInsensitive);
            if(c==oldCategory && oldTopic<0)chosen=root;
            int visible=0;
            for(int i=0;i<topics.size();++i)if(topics[i].category==c){
                const auto &topic=topics[i];if(!categoryMatch && !(topic.ko+topic.en+topic.descriptionKo+topic.descriptionEn+topic.shortcut).contains(query,Qt::CaseInsensitive))continue;
                auto *child=new QTreeWidgetItem(root,{title(topic,en)});child->setData(0,Qt::UserRole,c);child->setData(0,Qt::UserRole+1,i);++visible;
                if(c==oldCategory && i==oldTopic)chosen=child;
            }
            root->setHidden(!visible);root->setExpanded(!query.isEmpty());
        }
        if(!chosen || chosen->isHidden() || (chosen->parent() && chosen->parent()->isHidden())){
            chosen=nullptr;for(int c=0;c<tree->topLevelItemCount();++c)if(!tree->topLevelItem(c)->isHidden()){chosen=tree->topLevelItem(c);break;}
        }
        if(chosen)tree->setCurrentItem(chosen);render(chosen);
    };
    connect(tree,&QTreeWidget::currentItemChanged,dialog,[render](QTreeWidgetItem *item,QTreeWidgetItem *){render(item);});
    connect(language,&QComboBox::currentIndexChanged,dialog,[refresh](int index){QSettings().setValue("help/language",index);refresh();});
    connect(search,&QLineEdit::textChanged,dialog,[refresh](const QString &){refresh();});
    refresh();dialog->show();
}
}
