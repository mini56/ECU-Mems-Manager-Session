#include "captureviewer.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include "i18n.h"
#define tr I18n::text

CaptureViewer::CaptureViewer(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(I18n::text(6300) /* EN: View captures */);
    resize(800, 520);

    QHBoxLayout *main = new QHBoxLayout(this);
    m_list = new QListWidget(this);
    m_list->setMinimumWidth(260);
    main->addWidget(m_list);

    QVBoxLayout *right = new QVBoxLayout;
    m_preview = new QLabel(I18n::text(6301) /* EN: Select a capture. */, this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(460, 360);
    m_preview->setFrameShape(QFrame::StyledPanel);
    right->addWidget(m_preview, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_open = new QPushButton(I18n::text(6302) /* EN: Open / Enlarge */, this);
    m_delete = new QPushButton(I18n::text(6303) /* EN: Delete */, this);
    m_close = new QPushButton(I18n::text(6304) /* EN: Close */, this);
    buttons->addWidget(m_open);
    buttons->addWidget(m_delete);
    buttons->addStretch();
    buttons->addWidget(m_close);
    right->addLayout(buttons);
    main->addLayout(right, 1);

    connect(m_list, &QListWidget::currentItemChanged, this, &CaptureViewer::selectionChanged);
    connect(m_open, &QPushButton::clicked, this, &CaptureViewer::openSelected);
    connect(m_delete, &QPushButton::clicked, this, &CaptureViewer::deleteSelected);
    connect(m_close, &QPushButton::clicked, this, &QDialog::accept);

    refresh();
}

QString CaptureViewer::selectedPath() const
{
    QListWidgetItem *item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void CaptureViewer::refresh()
{
    m_list->clear();
    const QDir dir(QCoreApplication::applicationDirPath() + "/captures");
    const QFileInfoList files = dir.entryInfoList(QStringList() << "*.png" << "*.jpg" << "*.jpeg",
                                                   QDir::Files, QDir::Time);
    for (const QFileInfo &fi : files)
    {
        QListWidgetItem *item = new QListWidgetItem(
            fi.lastModified().toString("dd/MM/yyyy HH:mm:ss") + "  —  " + fi.fileName(), m_list);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
    else
        m_preview->setText(I18n::text(6305) /* EN: No saved capture. */);
}

void CaptureViewer::selectionChanged()
{
    const QString path = selectedPath();
    if (path.isEmpty()) return;
    QPixmap pix(path);
    if (pix.isNull())
        m_preview->setText(I18n::text(6306) /* EN: Unable to read this capture. */);
    else
        m_preview->setPixmap(pix.scaled(m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void CaptureViewer::openSelected()
{
    const QString path = selectedPath();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void CaptureViewer::deleteSelected()
{
    const QString path = selectedPath();
    if (path.isEmpty()) return;

    if (QMessageBox::question(this, I18n::text(6307) /* EN: Delete capture */,
                              I18n::text(6308) /* EN: Permanently delete the selected capture? */,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (!QFile::remove(path))
    {
        QMessageBox::warning(this, I18n::text(6309) /* EN: Error */, I18n::text(6310) /* EN: Unable to delete this capture. */);
        return;
    }
    refresh();
}
