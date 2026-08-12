#ifndef CAPTUREVIEWER_H
#define CAPTUREVIEWER_H

#include <QDialog>
class QListWidget;
class QLabel;
class QPushButton;
class QListWidgetItem;

class CaptureViewer : public QDialog
{
    Q_OBJECT
public:
    explicit CaptureViewer(QWidget *parent = nullptr);

private slots:
    void refresh();
    void selectionChanged();
    void openSelected();
    void deleteSelected();

private:
    QString selectedPath() const;
    QListWidget *m_list;
    QLabel *m_preview;
    QPushButton *m_open;
    QPushButton *m_delete;
    QPushButton *m_close;
};
#endif
