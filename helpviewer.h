#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <QDialog>
#include <QScrollBar>
#include <QStringList>
#include <QUrl>

class QEvent;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextBrowser;

class HelpViewer : public QDialog
{
    Q_OBJECT
public:
    explicit HelpViewer(const QString title, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCloseClicked();
    void onAnchorClicked(const QUrl &url);
    void onChapterChanged(int row);
    void onSearchChanged(const QString &text);
    void onSearchActivated();
    void goTop();
    void zoomOutHelp();
    void resetZoom();
    void zoomInHelp();

private:
    void buildUi();
    void retranslateUi();
    void rebuildDocument();
    QString buildDocumentHtml() const;
    QStringList chapterImages() const;
    void openImage(int chapterIndex);
    void setZoomLevel(int level);

    QString m_baseTitle;
    QLineEdit *m_search;
    QListWidget *m_chapters;
    QTextBrowser *m_viewer;
    QPushButton *m_topButton;
    QPushButton *m_zoomOutButton;
    QPushButton *m_zoomResetButton;
    QPushButton *m_zoomInButton;
    QPushButton *m_closeButton;
    int m_zoomLevel;
};

#endif // HELPVIEWER_H
