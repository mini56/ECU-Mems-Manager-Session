#ifndef DIAGNOSTICPANEL_H
#define DIAGNOSTICPANEL_H

#include <QWidget>
#include <QString>
#include "rosco.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;

class DiagnosticPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DiagnosticPanel(QWidget *parent = nullptr);
    void updateData(const mems_data *data);
    void setEcuId(const QByteArray &id);

private slots:
    void captureReference();
    void clearReference();
    void exportReport();

private:
    struct Snapshot {
        bool valid = false;
        mems_data data{};
        QString timestamp;
    };

    QLabel *m_status;
    QLabel *m_score;
    QTableWidget *m_checks;
    QPlainTextEdit *m_report;
    QPushButton *m_capture;
    QPushButton *m_clear;
    QPushButton *m_export;
    Snapshot m_reference;
    mems_data m_last{};
    bool m_haveData = false;
    QByteArray m_ecuId;

    void rebuild(const mems_data &d);
    void addCheck(const QString &name, const QString &value,
                  const QString &state, const QString &advice);
    QString buildReport() const;
    static QString hexByte(quint8 v);
};

#endif
