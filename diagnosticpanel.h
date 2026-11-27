#ifndef DIAGNOSTICPANEL_H
#define DIAGNOSTICPANEL_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "rosco.h"
#include "mems16na_session_evidence.h"
#include "mems217_passive.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;

class DiagnosticPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DiagnosticPanel(QWidget *parent = nullptr);
    void updateData(const mems_data *data, const Mems16Na::SessionEvidence *na = nullptr);
    void endNaSession();
    void update2JData(const QVariantMap &values, const QStringList &faults,
                      const QString &family = QStringLiteral("MEMS 2J"));
    void setEcuId(const QByteArray &id);
    // The IA consumes the same report, not a competing set of diagnostic rules.
    QString naDiagnosticReport() const { return m_naEvidence.identifiedProfile()?buildNaReport():QString(); }

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
    Snapshot m_naReference;
    Mems16Na::SessionEvidence m_naEvidence;
    Mems217::Observation m_passive;
    void addPassiveChecks();
    void rebuildNa(const mems_data &d);
    void restoreNaChecksStyle();
    QString m_naOriginalChecksStyle;
    bool m_naStyleActive = false;
    QString buildNaReport() const;
    mems_data m_last{};
    bool m_haveData = false;
    bool m_showing2J = false;
    QString m_structuredEcuName = QStringLiteral("MEMS 2J");
    QVariantMap m_mems2jValues;
    QStringList m_mems2jFaults;
    QVariantMap m_2jReference;
    QString m_2jReferenceTime;
    QByteArray m_ecuId;

    void rebuild(const mems_data &d);
    void addCheck(const QString &name, const QString &value,
                  const QString &state, const QString &advice);
    QString buildReport() const;
    static QString hexByte(quint8 v);
};

#endif
