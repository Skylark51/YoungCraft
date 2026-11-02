#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>

namespace youngcraft {
class GaussianConnection final : public QObject {
    Q_OBJECT
public:
    static GaussianConnection *instance();
    void connectNow(bool force = false);
    QString text() const { return m_text; }
    bool connected() const { return m_connected; }
signals:
    void changed(const QString &text, bool connected);
private:
    explicit GaussianConnection(QObject *parent);
    void publish(const QString &text, bool connected);
    QProcess m_process;
    QTimer m_timeout;
    QByteArray m_output;
    QString m_text, m_signature;
    bool m_connected = false;
};
}
