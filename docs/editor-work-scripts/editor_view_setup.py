from pathlib import Path
p=Path('src/render/MoleculeView.h');s=p.read_text(encoding='utf-8');s=s.replace('#include "model/GeometryEditor.h"','#include "model/GeometryEditor.h"\n#include "model/MoleculeEditController.h"\n#include "model/FragmentLibrary.h"')
s=s.replace('    bool hasGhostBond() const', '''    SelectionModel &selection() { return m_selection; }
    const SelectionModel &selection() const { return m_selection; }
    void setDrawingBondOrder(double order) { m_drawOrder=order; }
    void setBondCentricMode(bool on) { m_bondCentric=on; m_activeBond=-1; }
    void setFragmentPreview(const Fragment &f) { m_insertFragment=f; m_insertPreview=true; m_templatePreview=-1; }
    void setTemplatePreview(int g) { m_templatePreview=g; m_insertPreview=true; }
    void clearInsertionPreview() { m_insertPreview=false; m_templatePreview=-1; if(m_previewOverlay)m_previewOverlay->update(); }
    bool hasGhostBond() const''')
s=s.replace('    void atomPicked(int index);','''    void atomEditRequested(int index);
    void bondEditRequested(int index);
    void deleteRequested(int atom, int bond);
    void attachmentClicked(int center, const QVector3D &direction);
    void groupMoved(const QVector3D &translation);
    void bondManipulated(int first, int second, int kind, double value, bool opposite);
    void atomPicked(int index);''')
s=s.replace('    int pickAtom(const QPoint &point) const;','''    int pickAtom(const QPoint &point) const;
    int pickBond(const QPoint &point) const;
    int pickSite(const QPoint &point) const;''')
s=s.replace('    Molecule m_molecule;', '''    Molecule m_molecule;
    SelectionModel m_selection;
    int m_hoverAtom=-1, m_hoverBond=-1;
    double m_drawOrder=0, m_previewOrder=1;
    bool m_rectangle=false, m_groupDrag=false;
    QVector3D m_groupTranslation;
    bool m_insertPreview=false;
    Fragment m_insertFragment;
    int m_templatePreview=-1;
    QVector3D m_insertPosition, m_insertDirection{1,0,0};
    bool m_bondCentric=false, m_centricDrag=false;
    int m_activeBond=-1, m_centricKind=0;
    double m_centricStart=0, m_centricValue=0;
    QString m_centricText;''')
p.write_text(s,encoding='utf-8')
p=Path('src/app/mainwindow/MainWindow_Window.cpp');s=p.read_text(encoding='utf-8').replace('#include <QActionGroup>','#include <QActionGroup>\n#include <QDateTime>');s=s.replace('    const bool input = isTextInputFocused();','''    const bool input = isTextInputFocused();
    if(!input && m_drawingMode && !(event->modifiers() & (Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier))) {
        if(event->key()==Qt::Key_Escape){m_view->setProperty("pendingFragment",QString());m_view->setProperty("pendingCenter",0);m_view->clearInsertionPreview();event->accept();return;}
        if(event->key()>=Qt::Key_1&&event->key()<=Qt::Key_3){m_drawBondOrder->setCurrentIndex(m_drawBondOrder->findData(double(event->key()-Qt::Key_0)));event->accept();return;}
        if(!event->text().isEmpty()&&event->text()[0].isLetter()){
            qint64 now=QDateTime::currentMSecsSinceEpoch();QString buffer=m_drawBar->property("elementBuffer").toString();
            if(now-m_drawBar->property("elementBufferTime").toLongLong()>700||buffer.size()>=2)buffer.clear();
            QString candidate=buffer+event->text();int z=atomicNumberFor(candidate);
            if(z<1){candidate=event->text();z=atomicNumberFor(candidate);}
            if(z>0)m_drawElement->setCurrentText(elementSymbol(z));
            else if(candidate.compare("A",Qt::CaseInsensitive)==0)m_drawBondOrder->setCurrentIndex(m_drawBondOrder->findData(0.0));
            m_drawBar->setProperty("elementBuffer",candidate);m_drawBar->setProperty("elementBufferTime",now);event->accept();return;
        }
    }''');p.write_text(s,encoding='utf-8')
