#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QUrl>

class QShowEvent;

// The help is an offline, navigable part of the application's UI.
// Its sections are read from the bundled HTML, not duplicated in code.
class HelpViewer : public QDialog
{
    Q_OBJECT
public:
    explicit HelpViewer(const QString &title, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onCloseClicked();
    void onAnchorClicked(const QUrl &url);

private:
    void updateAppearance(bool dark);
    void loadDocument();

    QVBoxLayout *m_vbox=nullptr;
    QTextBrowser *m_viewer=nullptr;
    QListWidget *m_navigation=nullptr;
    QPushButton *m_closeButton=nullptr;
};

#endif // HELPVIEWER_H
