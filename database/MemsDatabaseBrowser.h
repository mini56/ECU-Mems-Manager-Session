#ifndef MEMSDATABASEBROWSER_H
#define MEMSDATABASEBROWSER_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QComboBox;
class QTableWidget;
class QTextBrowser;
class QPushButton;
class MemsReferenceDatabase;

class MemsDatabaseBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit MemsDatabaseBrowser(QWidget *parent = nullptr);
    ~MemsDatabaseBrowser() override;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void refreshResults();
    void showSelectedEcu();
    void showGenerationSheet();
    void openGenerationPdf();

private:
    void buildUi();
    void retranslateUi();
    void updateMetrics();
    QLabel *makeLabel(int key, QWidget *parent = nullptr);
    QString selectedPartNumber() const;
    QString selectedSystemFamily() const;
    QString generationFilterToken() const;
    QString renderEcuHtml(const QString &partNumber) const;
    QString renderGenerationXml(const QString &path) const;

private:
    MemsReferenceDatabase *m_database;
    QLineEdit *m_search;
    QComboBox *m_generation;
    QTableWidget *m_results;
    QTextBrowser *m_detail;
    QLabel *m_resultCount;
    QLabel *m_metricValues[6];
    QLabel *m_metricTitles[6];
    QLabel *m_title;
    QLabel *m_subtitle;
    QLabel *m_scopeTitle;
    QLabel *m_updateTitle;
    QLabel *m_updateState;
    QPushButton *m_sheetButton;
    QPushButton *m_pdfButton;
};

#endif
