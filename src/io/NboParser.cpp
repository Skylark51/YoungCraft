#include "io/NboParser.h"
#include "io/ParserCommon.h"
#include <QFile>
#include <QRegularExpression>
namespace youngcraft {
NboData NboParser::parse(const QString &path) {
    NboData data;
    QFile file(path); if (!file.open(QIODevice::ReadOnly)) return data;
    enum Mode { None, Charges, Orbitals, Interactions, Wiberg } mode = None;
    int section = 0, blanks = 0; bool rows = false;
    QVector<int> columns;
    const QString number = QStringLiteral(R"([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[EeDd][+-]?\d+)?)");
    const QRegularExpression charge("^\\s*([A-Za-z]{1,2})\\s+(\\d+)\\s+("+number+")\\s+("+number+")\\s+("+number+")\\s+("+number+")\\s+("+number+")");
    const QRegularExpression orbital("^\\s*(\\d+)\\.\\s*\\(\\s*("+number+")\\)\\s*(.+)$");
    const QRegularExpression interaction("^\\s*(\\d+\\.\\s*.+?)\\s+(\\d+\\.\\s*.+?)\\s+("+number+")\\s+("+number+")\\s+("+number+")\\s*$");
    const QRegularExpression matrixRow(QStringLiteral(R"(^\s*(\d+)\.\s*([A-Za-z]{1,2})\s+(.*)$)"));
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        const QString trimmed = line.trimmed();
        const QString lower = trimmed.toLower();
        Mode next = None;
        if (lower.contains("summary of natural population analysis")) next = Charges;
        else if (lower.contains("second order perturbation theory analysis")) next = Interactions;
        else if (lower.contains("wiberg bond index matrix")) next = Wiberg;
        else if (lower.contains("(occupancy)") && lower.contains("bond orbital")) next = Orbitals;
        if (next != None) { mode = next; ++section; rows = false; blanks = 0; columns.clear(); }
        if (mode == None) continue;
        if (data.excerpt.size() < 2000000) data.excerpt += line;
        if (lower.contains("leave link") || lower.contains("normal termination") || lower.contains("natural bond orbitals (summary)") || lower.contains("wiberg bond index, totals")) { mode=None; continue; }
        if (trimmed.isEmpty()) { ++blanks; if (rows && mode != Wiberg && blanks > 2) mode=None; continue; }
        blanks=0;
        if (mode == Charges) {
            auto m=charge.match(line);
            if (m.hasMatch()) { data.charges.push_back({section,{m.captured(2),m.captured(1),m.captured(3),m.captured(4),m.captured(5),m.captured(6),m.captured(7)}}); rows=true; }
            else if (rows && trimmed.startsWith("* Total *")) mode=None;
        } else if (mode == Orbitals) {
            auto m=orbital.match(line);
            if (m.hasMatch()) { data.orbitals.push_back({section,{m.captured(1),m.captured(2),m.captured(3)}}); rows=true; }
        } else if (mode == Interactions) {
            auto m=interaction.match(line);
            if (m.hasMatch()) { data.interactions.push_back({section,{m.captured(1),m.captured(2),m.captured(3),m.captured(4),m.captured(5)}}); rows=true; }
        } else if (mode == Wiberg) {
            auto fields=parser::columns(line);
            if (fields.isEmpty()) continue;
            if (fields.first().compare("Atom",Qt::CaseInsensitive)==0) {
                columns.clear(); for (int i=1;i<fields.size();++i) { bool ok=false; int c=fields[i].toInt(&ok); if(ok)columns.push_back(c); } continue;
            }
            auto m=matrixRow.match(line);
            if (m.hasMatch() && !columns.isEmpty()) {
                const int atom=m.captured(1).toInt(); const auto values=parser::columns(m.captured(3));
                for (int i=0;i<qMin(values.size(),columns.size());++i) { bool ok=false; double value=parser::toDouble(values[i],&ok);
                    if(ok && atom<columns[i]) data.wiberg.push_back({section,{QString::number(atom),QString::number(columns[i]),QString::number(value,'g',10)}});
                } rows=true;
            } else if (rows && !trimmed.startsWith('-') && !trimmed.startsWith("=")) {
                bool allInts=true; for(const auto &f:fields) { bool ok=false; f.toInt(&ok); allInts &= ok; }
                if(allInts) { columns.clear(); for(const auto &f:fields) columns.push_back(f.toInt()); }
                else mode=None;
            }
        }
    }
    return data;
}
}
