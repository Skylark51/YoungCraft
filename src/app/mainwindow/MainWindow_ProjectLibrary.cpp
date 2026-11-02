#include "app/MainWindow.h"
#include "ui/PlotWidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <cmath>
#include <functional>
#include <limits>
#include <utility>

namespace youngcraft {
namespace {

constexpr double kHartreeToKcal = 627.509474;
constexpr double kMissing = std::numeric_limits<double>::quiet_NaN();

struct PathStep {
    QString label;
    QString role;
    QString file;
    int multiplicity = 0;
    double electronic = kMissing;
    double gibbs = kMissing;
    bool terminated = false;
};

struct ResearchProject {
    QString title;
    QVector<PathStep> steps;
    int reference = 0;
};

QString absoluteFile(const QString &projectFile, const QString &entry) {
    return QDir(QFileInfo(projectFile).absolutePath()).absoluteFilePath(entry);
}
QString relativeFile(const QString &projectFile, const QString &file) {
    return QDir(QFileInfo(projectFile).absolutePath()).relativeFilePath(file);
}
QString formattedEnergy(double value, int decimals = 6) {
    return std::isfinite(value) ? QString::number(value, 'f', decimals) : QStringLiteral("—");
}
QString csvCell(QString value) {
    value.replace('"', QStringLiteral("\"\""));
    return QStringLiteral("\"") + value + QStringLiteral("\"");
}
QString suggestedRole(const QString &name) {
    const QString s = name.toLower();
    if (s.contains("ts") || s.contains("transition")) return QStringLiteral("TS");
    if (s.contains("int") || s.contains("intermediate")) return QStringLiteral("Intermediate");
    if (s.contains("product") || s.contains("prod")) return QStringLiteral("Product");
    if (s.contains("reactant") || s.contains("reagent") || s.contains("-rc")) return QStringLiteral("Reactant");
    return QStringLiteral("Unassigned");
}
double numericValue(QString raw) {
    raw.replace('D', 'E').replace('d', 'E');
    bool valid = false;
    const double value = raw.toDouble(&valid);
    return valid && std::isfinite(value) ? value : kMissing;
}

// Read just the data needed for a reaction profile. Full output parsing is
// deferred until a file is opened in the normal molecular workspace.
PathStep inspectOutput(const QString &path) {
    PathStep step;
    step.file = QFileInfo(path).absoluteFilePath();
    step.label = QFileInfo(path).completeBaseName();
    step.role = suggestedRole(step.label);
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) return step;
    static const QRegularExpression scf(
        QStringLiteral(R"(SCF Done:\s+E\([^)]+\)\s*=\s*([-+]?\d+(?:\.\d+)?(?:[DdEe][-+]?\d+)?))"));
    static const QRegularExpression freeEnergy(
        QStringLiteral(R"(Sum of electronic and thermal Free Energies=\s*([-+]?\d+(?:\.\d+)?(?:[DdEe][-+]?\d+)?))"));
    static const QRegularExpression multiplicity(
        QStringLiteral(R"(Multiplicity\s*=\s*(\d+))"));
    while (!input.atEnd()) {
        const QString line = QString::fromLatin1(input.readLine());
        if (line.contains(QStringLiteral("SCF Done:"))) {
            const auto match = scf.match(line);
            if (match.hasMatch()) step.electronic = numericValue(match.captured(1));
        } else if (line.contains(QStringLiteral("Sum of electronic and thermal Free Energies="))) {
            const auto match = freeEnergy.match(line);
            if (match.hasMatch()) step.gibbs = numericValue(match.captured(1));
        } else if (line.contains(QStringLiteral("Multiplicity"))) {
            const auto match = multiplicity.match(line);
            if (match.hasMatch()) step.multiplicity = match.captured(1).toInt();
        } else if (line.contains(QStringLiteral("Normal termination of Gaussian"))) {
            step.terminated = true;
        }
    }
    return step;
}

class ProjectLibraryDialog final : public QDialog {
public:
    explicit ProjectLibraryDialog(MainWindow *window)
        : QDialog(window), m_window(window) {
        setObjectName(QStringLiteral("ProjectLibraryDialog"));
        setWindowTitle(QStringLiteral("프로젝트 라이브러리 · Reaction Pathways"));
        resize(1160, 760);

        auto *root = new QHBoxLayout(this);
        auto *left = new QWidget(this);
        auto *leftLayout = new QVBoxLayout(left);
        leftLayout->addWidget(new QLabel(QStringLiteral("연구 프로젝트"), left));
        m_library = new QListWidget(left);
        m_library->setObjectName(QStringLiteral("ProjectLibraryList"));
        leftLayout->addWidget(m_library, 1);
        auto *createButton = new QPushButton(QStringLiteral("새 프로젝트"), left);
        auto *attachButton = new QPushButton(QStringLiteral("기존 프로젝트 등록"), left);
        auto *forgetButton = new QPushButton(QStringLiteral("목록에서 제거"), left);
        leftLayout->addWidget(createButton);
        leftLayout->addWidget(attachButton);
        leftLayout->addWidget(forgetButton);
        left->setMinimumWidth(225);
        left->setMaximumWidth(295);

        auto *right = new QWidget(this);
        auto *rightLayout = new QVBoxLayout(right);
        m_title = new QLabel(QStringLiteral("프로젝트를 선택하거나 새로 만드세요."), right);
        m_title->setWordWrap(true);
        QFont titleFont = m_title->font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        m_title->setFont(titleFont);
        rightLayout->addWidget(m_title);

        auto *tools = new QHBoxLayout;
        auto *filesButton = new QPushButton(QStringLiteral("계산 파일 추가"), right);
        auto *folderButton = new QPushButton(QStringLiteral("폴더에서 가져오기"), right);
        auto *removeButton = new QPushButton(QStringLiteral("단계 제거"), right);
        auto *upButton = new QPushButton(QStringLiteral("▲"), right);
        auto *downButton = new QPushButton(QStringLiteral("▼"), right);
        auto *referenceButton = new QPushButton(QStringLiteral("기준점 지정"), right);
        auto *openButton = new QPushButton(QStringLiteral("3D로 열기"), right);
        for (auto *button : {filesButton, folderButton, removeButton, upButton,
                             downButton, referenceButton, openButton}) tools->addWidget(button);
        rightLayout->addLayout(tools);

        m_table = new QTableWidget(right);
        m_table->setObjectName(QStringLiteral("ProjectPathwayTable"));
        m_table->setColumnCount(8);
        m_table->setHorizontalHeaderLabels({QStringLiteral("#"), QStringLiteral("Species"),
            QStringLiteral("역할"), QStringLiteral("M"), QStringLiteral("E / a.u."),
            QStringLiteral("G / a.u."), QStringLiteral("상대 에너지"), QStringLiteral("파일")});
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_table->setAlternatingRowColors(true);

        auto *plotSection = new QWidget(right);
        auto *plotLayout = new QVBoxLayout(plotSection);
        auto *plotTools = new QHBoxLayout;
        plotTools->addWidget(new QLabel(QStringLiteral("에너지 기준:"), plotSection));
        m_energyMode = new QComboBox(plotSection);
        m_energyMode->setObjectName(QStringLiteral("ProjectEnergyMode"));
        m_energyMode->addItems({QStringLiteral("Electronic (SCF)"), QStringLiteral("Gibbs (frequency)")});
        plotTools->addWidget(m_energyMode);
        plotTools->addStretch();
        auto *exportButton = new QPushButton(QStringLiteral("SI 테이블 CSV 저장"), plotSection);
        plotTools->addWidget(exportButton);
        plotLayout->addLayout(plotTools);
        m_plot = new PlotWidget(plotSection);
        m_plot->setObjectName(QStringLiteral("ProjectPathwayPlot"));
        m_plot->setMinimumHeight(220);
        plotLayout->addWidget(m_plot, 1);
        m_note = new QLabel(QStringLiteral("에너지 값은 파일의 최종 SCF 또는 thermal Gibbs 값입니다. 서로 다른 계산 수준은 직접 확인하세요."), plotSection);
        m_note->setWordWrap(true);
        plotLayout->addWidget(m_note);

        auto *split = new QSplitter(Qt::Vertical, right);
        split->addWidget(m_table);
        split->addWidget(plotSection);
        split->setStretchFactor(0, 3);
        split->setStretchFactor(1, 2);
        rightLayout->addWidget(split, 1);
        root->addWidget(left);
        root->addWidget(right, 1);

        connect(createButton, &QPushButton::clicked, this, [this] { createProject(); });
        connect(attachButton, &QPushButton::clicked, this, [this] { attachProject(); });
        connect(forgetButton, &QPushButton::clicked, this, [this] { forgetProject(); });
        connect(filesButton, &QPushButton::clicked, this, [this] { addFiles(); });
        connect(folderButton, &QPushButton::clicked, this, [this] { addFolder(); });
        connect(removeButton, &QPushButton::clicked, this, [this] { removeStep(); });
        connect(upButton, &QPushButton::clicked, this, [this] { moveStep(-1); });
        connect(downButton, &QPushButton::clicked, this, [this] { moveStep(1); });
        connect(referenceButton, &QPushButton::clicked, this, [this] { setReference(); });
        connect(openButton, &QPushButton::clicked, this, [this] { openStep(); });
        connect(exportButton, &QPushButton::clicked, this, [this] { exportCsv(); });
        connect(m_library, &QListWidget::currentRowChanged, this, [this] { loadSelectedProject(); });
        connect(m_energyMode, qOverload<int>(&QComboBox::currentIndexChanged), this,
                [this] { refreshTable(); });
        connect(m_table, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
            if (m_rebuilding || !item || item->column() != 1) return;
            if (item->row() < 0 || item->row() >= m_project.steps.size()) return;
            m_project.steps[item->row()].label = item->text().trimmed();
            saveProject();
        });
        connect(m_plot, &PlotWidget::pointClicked, this, [this](int index) {
            if (index >= 0 && index < m_plotRows.size()) m_table->selectRow(m_plotRows[index]);
        });
        loadLibrary();
    }

private:
    int selectedStep() const { return m_table->currentRow(); }
    QStringList registeredProjects() const {
        return QSettings().value(QStringLiteral("projects/library")).toStringList();
    }
    void setRegisteredProjects(const QStringList &paths) {
        QSettings().setValue(QStringLiteral("projects/library"), paths);
    }
    void loadLibrary(const QString &preferred = {}) {
        QSignalBlocker block(m_library);
        m_library->clear();
        int preferredRow = -1;
        for (const QString &path : registeredProjects()) {
            auto *entry = new QListWidgetItem(QFileInfo(path).completeBaseName(), m_library);
            entry->setData(Qt::UserRole, path);
            if (!QFileInfo::exists(path)) entry->setText(entry->text() + QStringLiteral(" (파일 없음)"));
            if (path == preferred) preferredRow = m_library->count() - 1;
        }
        if (preferredRow < 0 && m_library->count()) preferredRow = 0;
        if (preferredRow >= 0) m_library->setCurrentRow(preferredRow);
        loadSelectedProject();
    }
    void registerProject(const QString &path) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        QStringList paths = registeredProjects();
        if (!paths.contains(absolute)) paths.append(absolute);
        setRegisteredProjects(paths);
        loadLibrary(absolute);
    }
    bool readProject(const QString &path, ResearchProject *project) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;
        QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) return false;
        const QJsonObject object = doc.object();
        if (object.value(QStringLiteral("schema")).toInt() != 1) return false;
        project->title = object.value(QStringLiteral("title")).toString(QFileInfo(path).completeBaseName());
        project->reference = object.value(QStringLiteral("reference")).toInt();
        project->steps.clear();
        for (const QJsonValue &value : object.value(QStringLiteral("steps")).toArray()) {
            const QJsonObject stepObject = value.toObject();
            PathStep step;
            step.label = stepObject.value(QStringLiteral("label")).toString();
            step.role = stepObject.value(QStringLiteral("role")).toString(QStringLiteral("Unassigned"));
            step.file = absoluteFile(path, stepObject.value(QStringLiteral("file")).toString());
            step.multiplicity = stepObject.value(QStringLiteral("multiplicity")).toInt();
            step.electronic = stepObject.value(QStringLiteral("electronic")).isDouble()
                ? stepObject.value(QStringLiteral("electronic")).toDouble() : kMissing;
            step.gibbs = stepObject.value(QStringLiteral("gibbs")).isDouble()
                ? stepObject.value(QStringLiteral("gibbs")).toDouble() : kMissing;
            step.terminated = stepObject.value(QStringLiteral("terminated")).toBool();
            project->steps.append(step);
        }
        if (project->steps.isEmpty()) project->reference = 0;
        else project->reference = qBound(0, project->reference, int(project->steps.size() - 1));
        return true;
    }
    bool saveProject() {
        if (m_projectFile.isEmpty()) return false;
        QJsonObject object;
        object.insert(QStringLiteral("schema"), 1);
        object.insert(QStringLiteral("title"), m_project.title);
        object.insert(QStringLiteral("reference"), m_project.reference);
        QJsonArray steps;
        for (const PathStep &step : std::as_const(m_project.steps)) {
            QJsonObject row;
            row.insert(QStringLiteral("label"), step.label);
            row.insert(QStringLiteral("role"), step.role);
            row.insert(QStringLiteral("file"), relativeFile(m_projectFile, step.file));
            row.insert(QStringLiteral("multiplicity"), step.multiplicity);
            if (std::isfinite(step.electronic)) row.insert(QStringLiteral("electronic"), step.electronic);
            if (std::isfinite(step.gibbs)) row.insert(QStringLiteral("gibbs"), step.gibbs);
            row.insert(QStringLiteral("terminated"), step.terminated);
            steps.append(row);
        }
        object.insert(QStringLiteral("steps"), steps);
        QSaveFile output(m_projectFile);
        if (!output.open(QIODevice::WriteOnly) ||
            output.write(QJsonDocument(object).toJson(QJsonDocument::Indented)) < 0 ||
            !output.commit()) {
            QMessageBox::warning(this, QStringLiteral("저장 오류"),
                                 QStringLiteral("프로젝트 파일을 저장하지 못했습니다:\n%1").arg(m_projectFile));
            return false;
        }
        return true;
    }
    void loadSelectedProject() {
        m_projectFile.clear();
        m_project = {};
        auto *entry = m_library->currentItem();
        if (entry) {
            const QString path = entry->data(Qt::UserRole).toString();
            ResearchProject loaded;
            if (readProject(path, &loaded)) {
                m_projectFile = path;
                m_project = loaded;
            } else {
                m_title->setText(QStringLiteral("프로젝트를 읽을 수 없습니다: %1").arg(path));
            }
        }
        refreshTable();
    }
    void createProject() {
        const QString path = QFileDialog::getSaveFileName(this,
            QStringLiteral("새 연구 프로젝트"), {}, QStringLiteral("YoungCraft project (*.ycproject)"));
        if (path.isEmpty()) return;
        QString finalPath = path;
        if (!finalPath.endsWith(QStringLiteral(".ycproject"), Qt::CaseInsensitive))
            finalPath += QStringLiteral(".ycproject");
        if (QFileInfo::exists(finalPath)) {
            QMessageBox::information(this, QStringLiteral("이미 존재함"),
                QStringLiteral("기존 프로젝트는 '기존 프로젝트 등록'으로 열어 주세요."));
            return;
        }
        m_projectFile = QFileInfo(finalPath).absoluteFilePath();
        m_project = {};
        m_project.title = QFileInfo(finalPath).completeBaseName();
        if (saveProject()) registerProject(m_projectFile);
    }
    void attachProject() {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("프로젝트 등록"), {},
            QStringLiteral("YoungCraft project (*.ycproject)"));
        if (path.isEmpty()) return;
        ResearchProject check;
        if (!readProject(path, &check)) {
            QMessageBox::warning(this, QStringLiteral("파일 오류"), QStringLiteral("지원하지 않는 프로젝트 파일입니다."));
            return;
        }
        registerProject(path);
    }
    void forgetProject() {
        if (!m_library->currentItem()) return;
        const QString path = m_library->currentItem()->data(Qt::UserRole).toString();
        QStringList paths = registeredProjects();
        paths.removeAll(path);
        setRegisteredProjects(paths);
        loadLibrary();
    }
    void importFiles(const QStringList &paths) {
        if (m_projectFile.isEmpty()) return;
        int added = 0;
        for (const QString &path : paths) {
            const QFileInfo info(path);
            if (!info.isFile()) continue;
            const QString absolute = info.absoluteFilePath();
            bool exists = false;
            for (const PathStep &step : std::as_const(m_project.steps)) {
                if (QFileInfo(step.file).absoluteFilePath().compare(absolute, Qt::CaseInsensitive) == 0) {
                    exists = true; break;
                }
            }
            if (exists) continue;
            m_project.steps.append(inspectOutput(absolute));
            ++added;
        }
        if (added && saveProject()) refreshTable();
        if (!added) m_note->setText(QStringLiteral("추가된 파일이 없습니다. 이미 등록된 파일인지 확인하세요."));
    }
    void addFiles() {
        if (m_projectFile.isEmpty()) return;
        importFiles(QFileDialog::getOpenFileNames(this, QStringLiteral("Gaussian 출력 선택"),
            QFileInfo(m_projectFile).absolutePath(), QStringLiteral("Gaussian output (*.out *.log)")));
    }
    void addFolder() {
        if (m_projectFile.isEmpty()) return;
        const QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("계산 결과 폴더 선택"),
            QFileInfo(m_projectFile).absolutePath());
        if (folder.isEmpty()) return;
        QStringList found;
        QDirIterator it(folder, {QStringLiteral("*.out"), QStringLiteral("*.log")},
                        QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && found.size() < 500) found.append(it.next());
        found.sort(Qt::CaseInsensitive);
        importFiles(found);
        if (found.size() == 500)
            QMessageBox::information(this, QStringLiteral("가져오기 제한"),
                QStringLiteral("최대 500개 파일만 가져왔습니다. 나머지는 파일 선택으로 추가하세요."));
    }
    void removeStep() {
        const int row = selectedStep();
        if (row < 0 || row >= m_project.steps.size()) return;
        m_project.steps.removeAt(row);
        if (row < m_project.reference) --m_project.reference;
        else if (row == m_project.reference) m_project.reference = 0;
        saveProject(); refreshTable(qMin(row, int(m_project.steps.size() - 1)));
    }
    void moveStep(int delta) {
        const int row = selectedStep();
        const int target = row + delta;
        if (row < 0 || target < 0 || target >= m_project.steps.size()) return;
        m_project.steps.swapItemsAt(row, target);
        if (m_project.reference == row) m_project.reference = target;
        else if (m_project.reference == target) m_project.reference = row;
        saveProject(); refreshTable(target);
    }
    void setReference() {
        const int row = selectedStep();
        if (row < 0 || row >= m_project.steps.size()) return;
        m_project.reference = row;
        saveProject(); refreshTable(row);
    }
    void openStep() {
        const int row = selectedStep();
        if (row < 0 || row >= m_project.steps.size()) return;
        const QString file = m_project.steps[row].file;
        if (!QFileInfo::exists(file)) {
            QMessageBox::warning(this, QStringLiteral("파일 없음"), QStringLiteral("계산 파일을 찾을 수 없습니다:\n%1").arg(file));
            return;
        }
        m_window->openFile(file);
    }
    double valueOf(const PathStep &step) const {
        return m_energyMode->currentIndex() == 0 ? step.electronic : step.gibbs;
    }
    void refreshTable(int selectRow = -1) {
        m_rebuilding = true;
        QSignalBlocker block(m_table);
        m_table->setRowCount(0);
        m_title->setText(m_projectFile.isEmpty()
            ? QStringLiteral("프로젝트를 선택하거나 새로 만드세요.")
            : m_project.title + QStringLiteral("   ·   ") + m_projectFile);
        m_plotRows.clear();
        QVector<QPointF> profile;
        const double reference = m_project.reference < m_project.steps.size()
            ? valueOf(m_project.steps[m_project.reference]) : kMissing;
        const QStringList roles = {QStringLiteral("Reactant"), QStringLiteral("TS"),
            QStringLiteral("Intermediate"), QStringLiteral("Product"), QStringLiteral("Unassigned")};
        for (int i = 0; i < m_project.steps.size(); ++i) {
            const PathStep &step = m_project.steps[i];
            m_table->insertRow(i);
            auto cell = [this,i](int col, const QString &text, bool editable = false) {
                auto *item = new QTableWidgetItem(text);
                if (!editable) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(i, col, item);
            };
            cell(0, QString::number(i + 1) + (i == m_project.reference ? QStringLiteral(" ★") : QString()));
            cell(1, step.label, true);
            auto *role = new QComboBox(m_table);
            role->addItems(roles);
            const int roleIndex = roles.indexOf(step.role);
            role->setCurrentIndex(roleIndex < 0 ? roles.size() - 1 : roleIndex);
            connect(role, qOverload<int>(&QComboBox::currentIndexChanged), this, [this,i,role](int) {
                if (m_rebuilding || i >= m_project.steps.size()) return;
                m_project.steps[i].role = role->currentText();
                saveProject();
            });
            m_table->setCellWidget(i, 2, role);
            cell(3, step.multiplicity > 0 ? QString::number(step.multiplicity) : QStringLiteral("—"));
            cell(4, formattedEnergy(step.electronic));
            cell(5, formattedEnergy(step.gibbs));
            const double energy = valueOf(step);
            const double delta = (std::isfinite(energy) && std::isfinite(reference))
                ? (energy - reference) * kHartreeToKcal : kMissing;
            cell(6, formattedEnergy(delta, 2));
            cell(7, QFileInfo(step.file).fileName());
            m_table->item(i, 7)->setToolTip(step.file);
            if (std::isfinite(delta)) {
                profile.append(QPointF(i + 1, delta));
                m_plotRows.append(i);
            }
        }
        m_rebuilding = false;
        if (selectRow >= 0 && selectRow < m_table->rowCount()) m_table->selectRow(selectRow);
        PlotWidget::StyleOptions options = m_plot->styleOptions();
        options.title = QStringLiteral("Reaction pathway");
        options.xTitle = QStringLiteral("Reaction step");
        options.yTitle = m_energyMode->currentIndex() == 0
            ? QStringLiteral("ΔE (kcal mol⁻¹)") : QStringLiteral("ΔG (kcal mol⁻¹)");
        m_plot->setStyleOptions(options);
        m_plot->setSeries(profile, options.xTitle, options.yTitle);
        if (profile.isEmpty()) m_plot->setEmptyMessage(
            QStringLiteral("선택한 에너지 종류의 기준점과 데이터가 필요합니다."));
        m_note->setText(m_energyMode->currentIndex() == 0
            ? QStringLiteral("SCF 최종 전자 에너지로 계산한 상대 ΔE입니다. 방법·용매가 동일한지 확인하세요.")
            : QStringLiteral("Gaussian thermal Gibbs 합계의 상대 ΔG입니다. SP + frequency 조합은 별도 검증이 필요합니다."));
    }
    void exportCsv() {
        if (m_projectFile.isEmpty()) return;
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("SI 에너지 표 내보내기"),
            QDir(QFileInfo(m_projectFile).absolutePath()).filePath(m_project.title + QStringLiteral("-pathway.csv")),
            QStringLiteral("CSV (*.csv)"));
        if (path.isEmpty()) return;
        QSaveFile output(path);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("저장 실패"), output.errorString());
            return;
        }
        QTextStream stream(&output);
        stream << "Species,Role,Multiplicity,E (Hartree),G (Hartree),Relative (kcal/mol),Energy source,File\n";
        const double reference = m_project.steps.isEmpty() ? kMissing : valueOf(m_project.steps[m_project.reference]);
        for (const PathStep &step : std::as_const(m_project.steps)) {
            const double energy = valueOf(step);
            const double relative = (std::isfinite(reference) && std::isfinite(energy))
                ? (energy - reference) * kHartreeToKcal : kMissing;
            stream << csvCell(step.label) << ',' << csvCell(step.role) << ','
                   << step.multiplicity << ','
                   << (std::isfinite(step.electronic) ? QString::number(step.electronic,'f',10) : QString()) << ','
                   << (std::isfinite(step.gibbs) ? QString::number(step.gibbs,'f',10) : QString()) << ','
                   << (std::isfinite(relative) ? QString::number(relative,'f',3) : QString()) << ','
                   << csvCell(m_energyMode->currentText()) << ',' << csvCell(step.file) << '\n';
        }
        stream.flush();
        if (!output.commit())
            QMessageBox::warning(this, QStringLiteral("저장 실패"), output.errorString());
    }

    MainWindow *m_window = nullptr;
    QListWidget *m_library = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_note = nullptr;
    PlotWidget *m_plot = nullptr;
    QComboBox *m_energyMode = nullptr;
    QString m_projectFile;
    ResearchProject m_project;
    QVector<int> m_plotRows;
    bool m_rebuilding = false;
};

} // namespace

void MainWindow::openProjectLibrary() {
    ProjectLibraryDialog dialog(this);
    dialog.exec();
}

} // namespace youngcraft
