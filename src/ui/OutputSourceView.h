#pragma once

#include "ui/SidebarButton.h"
#include "model/Molecule.h"
#include <QPlainTextEdit>
#include <QApplication>
#include <QTextBlock>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QThread>
#include <QPointer>
#include <QShortcut>
#include <algorithm>

namespace youngcraft {
class SourceText final : public QPlainTextEdit {
    class Gutter final : public QWidget {
    public:
        explicit Gutter(SourceText *text):QWidget(text),m_text(text){}
        void paintEvent(QPaintEvent *event) override {m_text->paintNumbers(event);}
    private:SourceText *m_text;
    };
public:
    explicit SourceText(QWidget *parent):QPlainTextEdit(parent),m_gutter(new Gutter(this)) {
        setObjectName("OutputSourceText");setReadOnly(true);setUndoRedoEnabled(false);setLineWrapMode(NoWrap);
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        connect(this,&QPlainTextEdit::blockCountChanged,this,[this]{updateGutter();});
        connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect &,int){m_gutter->update();});
        updateGutter();
    }
protected:
    void resizeEvent(QResizeEvent *event) override {QPlainTextEdit::resizeEvent(event);updateGutter();}
private:
    void updateGutter(){const int width=12+fontMetrics().horizontalAdvance(QString::number(blockCount()));setViewportMargins(width,0,0,0);m_gutter->setGeometry(contentsRect().left(),contentsRect().top(),width,contentsRect().height());m_gutter->update();}
    void paintNumbers(QPaintEvent *event){
        QPainter p(m_gutter);p.fillRect(event->rect(),palette().window());p.setFont(font());p.setPen(palette().color(QPalette::Text));
        auto block=firstVisibleBlock();int y=qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        while(block.isValid() && y<=event->rect().bottom()){
            const int h=qRound(blockBoundingRect(block).height());
            if(block.isVisible() && y+h>=event->rect().top())p.drawText(0,y,m_gutter->width()-5,fontMetrics().height(),Qt::AlignRight,QString::number(block.blockNumber()+1));
            y+=h;block=block.next();
        }
    }
    Gutter *m_gutter;
};

class OutputSourceView final : public QWidget {
public:
    explicit OutputSourceView(QWidget *parent=nullptr):QWidget(parent) {
        setObjectName("OutputSourceView");auto *layout=new QVBoxLayout(this);layout->setContentsMargins(6,6,6,6);
        m_path=new QLabel(QStringLiteral("원문 파일 없음 · 계산 output을 열어 주세요.")); m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);m_path->setWordWrap(true);layout->addWidget(m_path);
        auto *row=new QHBoxLayout;m_sections=new QComboBox;m_sections->setObjectName("OutputSourceSections");
        m_sections->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_sections->setMinimumContentsLength(16);
        row->addWidget(m_sections,1);
        auto *reload=new QPushButton(QStringLiteral("다시 읽기"));
        makeSidebarIconButton(reload, QStyle::SP_BrowserReload);
        row->addWidget(reload);
        auto *line=new QSpinBox;line->setObjectName("OutputGoToLine");line->setRange(1,100000000);line->setPrefix(QStringLiteral("행 "));row->addWidget(line);
        auto *go=new QPushButton(QStringLiteral("이동"));
        makeSidebarIconButton(go, QStyle::SP_ArrowForward);
        row->addWidget(go);layout->addLayout(row);
        auto *findRow=new QHBoxLayout;m_find=new QLineEdit;m_find->setObjectName("OutputFind");m_find->setPlaceholderText(QStringLiteral("원문 검색 · Enter 다음 / Shift+Enter 이전"));findRow->addWidget(m_find,1);
        auto *previous=new QPushButton(QStringLiteral("이전"));
        makeSidebarIconButton(previous, QStyle::SP_ArrowUp);
        auto *next=new QPushButton(QStringLiteral("다음"));
        makeSidebarIconButton(next, QStyle::SP_ArrowDown);
        findRow->addWidget(previous);findRow->addWidget(next);
        m_case=new QCheckBox(QStringLiteral("대소문자"));findRow->addWidget(m_case);layout->addLayout(findRow);
        m_text=new SourceText(this);layout->addWidget(m_text,1);m_status=new QLabel(QStringLiteral("output 원문을 열어 결과의 출처를 확인하세요."));layout->addWidget(m_status);
        connect(reload,&QPushButton::clicked,this,[this]{m_loaded=false;load();});
        connect(go,&QPushButton::clicked,this,[this,line]{goToLine(line->value());});
        connect(next,&QPushButton::clicked,this,[this]{find(false);});connect(previous,&QPushButton::clicked,this,[this]{find(true);});
        connect(m_find,&QLineEdit::returnPressed,this,[this]{find(QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));});
        connect(m_sections,qOverload<int>(&QComboBox::activated),this,[this](int i){jumpOffset(m_sections->itemData(i).toLongLong());});
        connect(m_text,&QPlainTextEdit::cursorPositionChanged,this,[this]{m_status->setText(QStringLiteral("행 %1 / %2 · %3").arg(m_text->textCursor().blockNumber()+1).arg(m_text->blockCount()).arg(m_context));});
    }
    void setData(const CalculationData &data){
        if(m_file==data.filePath && m_parsedSize==data.index.fileSize && m_offsets==data.index.sectionOffsets){if(isVisible())load();return;}
        m_offsets=data.index.sectionOffsets;
        if(m_file!=data.filePath || m_parsedSize!=data.index.fileSize){m_file=data.filePath;m_loaded=false;++m_generation;m_pending=-1;m_text->clear();m_lineStarts.clear();}
        m_parsedSize=data.index.fileSize;
        m_path->setText(m_file.isEmpty()?QStringLiteral("원문 파일 없음 · 직접 그린 구조에는 계산 output이 없습니다."):m_file);
        m_sections->clear();m_sections->addItem(QStringLiteral("원문 구간으로 이동…"),-1);
        const QVector<QPair<QString,QString>> names={{"frame_geometry",QStringLiteral("구조 / 최적화")},{"frequencies",QStringLiteral("진동수")},{"excited_states",QStringLiteral("TD-DFT 여기 상태")},{"orbital_energies",QStringLiteral("궤도 에너지")},{"irc",QStringLiteral("IRC")},{"mulliken_charges",QStringLiteral("Mulliken 전하")},{"nbo","NBO"},{"pcm","PCM"}};
        for(const auto &entry:names){const auto offsets=m_offsets.value(entry.first);for(int i=0;i<offsets.size();++i)m_sections->addItem(entry.second+QStringLiteral(" %1").arg(i+1),offsets[i]);}
        if(isVisible())load();
    }
    void focusFind(){load();m_find->setFocus();m_find->selectAll();}
    void jumpSection(const QString &key,int index=0){
        const auto values=m_offsets.value(key);
        if(values.isEmpty() || index<0 || index>=values.size()){m_context=QStringLiteral("이 결과의 원문 위치 정보가 없습니다. 구간 선택 또는 검색을 사용하세요.");m_status->setText(m_context);return;}
        m_context=QStringLiteral("파서가 읽은 %1 #%2 · byte %3").arg(key).arg(index+1).arg(values[index]);jumpOffset(values[index]);
    }
    void jumpOffset(qint64 offset){m_pending=offset;if(isVisible())load();if(m_loaded&&offset>=0&&!m_lineStarts.isEmpty()){const auto it=std::upper_bound(m_lineStarts.cbegin(),m_lineStarts.cend(),offset);goToLine(int(it-m_lineStarts.cbegin()));m_pending=-1;}}
protected:
    void showEvent(QShowEvent *event) override {QWidget::showEvent(event);load();}
private:
    void load(){
        if(m_loaded||m_loading||m_file.isEmpty())return;
        if(QFileInfo(m_file).suffix().compare("chk",Qt::CaseInsensitive)==0){m_status->setText(QStringLiteral("CHK는 바이너리 파일입니다. 계산 .out/.log 또는 변환된 .fchk 원문을 여세요."));return;}
        m_loading=true;m_status->setText(QStringLiteral("원문을 읽는 중…"));const auto path=m_file;const auto generation=m_generation;
        QPointer<OutputSourceView> guard(this);
        auto *thread=QThread::create([guard,path,generation]{
            QFile file(path);QString error,text;QVector<qint64> starts; qint64 size=0;
            if(!file.open(QIODevice::ReadOnly))error=file.errorString();
            else {const auto bytes=file.readAll();size=bytes.size();if(file.error()!=QFileDevice::NoError)error=file.errorString();
                else if(bytes.contains('\0'))error=QStringLiteral("바이너리 데이터는 텍스트 원문으로 표시할 수 없습니다.");
                else {starts.push_back(0);for(qint64 i=0;i<bytes.size();++i)if(bytes[i]=='\n')starts.push_back(i+1);text=QString::fromUtf8(bytes);}}
            if(!guard)return;
            QMetaObject::invokeMethod(guard,[guard,generation,text=std::move(text),starts=std::move(starts),error,size]()mutable{
                if(!guard)return;auto *self=guard.data();self->m_loading=false;
                if(generation!=self->m_generation){if(self->isVisible())self->load();return;}
                if(!error.isEmpty()){self->m_text->clear();self->m_status->setText(QStringLiteral("원문을 읽지 못했습니다: ")+error);return;}
                self->m_text->setPlainText(text);self->m_lineStarts=std::move(starts);self->m_loaded=true;
                self->m_context=(self->m_parsedSize>0 && size!=self->m_parsedSize)?QStringLiteral("주의: 분석 후 파일 크기가 바뀌었습니다. 위치를 재확인하세요."):QStringLiteral("원문 읽기 완료");
                self->m_status->setText(self->m_context);if(self->m_pending>=0)self->jumpOffset(self->m_pending);
            },Qt::QueuedConnection);
        });
        connect(thread,&QThread::finished,thread,&QObject::deleteLater);thread->start();
    }
    void goToLine(int line){auto block=m_text->document()->findBlockByNumber(qBound(0,line-1,m_text->blockCount()-1));if(!block.isValid())return;QTextCursor cursor(block);cursor.select(QTextCursor::LineUnderCursor);m_text->setTextCursor(cursor);m_text->centerCursor();}
    void find(bool backwards){
        if(!m_loaded){load();return;}if(m_find->text().isEmpty())return;
        m_context=QStringLiteral("검색: ")+m_find->text();
        QTextDocument::FindFlags flags;if(backwards)flags|=QTextDocument::FindBackward;if(m_case->isChecked())flags|=QTextDocument::FindCaseSensitively;
        bool found=m_text->find(m_find->text(),flags);
        if(!found){auto cursor=m_text->textCursor();cursor.movePosition(backwards?QTextCursor::End:QTextCursor::Start);m_text->setTextCursor(cursor);found=m_text->find(m_find->text(),flags);}
        if(!found)m_status->setText(QStringLiteral("검색 결과 없음: ")+m_find->text());else m_text->centerCursor();
    }
    SourceText *m_text;QLineEdit *m_find;QCheckBox *m_case;QComboBox *m_sections;QLabel *m_path,*m_status;
    QString m_file,m_context;QMap<QString,QVector<qint64>> m_offsets;QVector<qint64> m_lineStarts;
    qint64 m_parsedSize=0,m_pending=-1;int m_generation=0;bool m_loaded=false,m_loading=false;
};
} // namespace youngcraft
