#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <QDialog>
#include <QString>
#include <QUrl>
#include <QVector>

class QLineEdit;
class QListWidget;
class QPushButton;
class QTextBrowser;

class HelpViewer : public QDialog
{
    Q_OBJECT
public:
    explicit HelpViewer(const QString title, QWidget *parent = nullptr);

private slots:
    void onCloseClicked();
    void onAnchorClicked(const QUrl &url);
    void onChapterChanged(int row);
    void onSearchTextChanged(const QString &text);
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void onTopClicked();

private:
    QString buildHelpHtml() const;
    void resetViewerToTop();

    QLineEdit *m_searchEdit;
    QListWidget *m_chapterList;
    QTextBrowser *m_viewer;
    QPushButton *m_closeButton;
    QPushButton *m_topButton;
    QPushButton *m_zoomOutButton;
    QPushButton *m_zoomResetButton;
    QPushButton *m_zoomInButton;
    QVector<QString> m_anchors;
    int m_zoomSteps;
};

#endif // HELPVIEWER_H
