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

CaptureViewer::CaptureViewer(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Voir les captures"));
    resize(800, 520);

    QHBoxLayout *main = new QHBoxLayout(this);
    m_list = new QListWidget(this);
    m_list->setMinimumWidth(260);
    main->addWidget(m_list);

    QVBoxLayout *right = new QVBoxLayout;
    m_preview = new QLabel(tr("Sélectionnez une capture."), this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(460, 360);
    m_preview->setFrameShape(QFrame::StyledPanel);
    right->addWidget(m_preview, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    m_open = new QPushButton(tr("Ouvrir / Agrandir"), this);
    m_delete = new QPushButton(tr("Supprimer"), this);
    m_close = new QPushButton(tr("Fermer"), this);
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
        m_preview->setText(tr("Aucune capture enregistrée."));
}

void CaptureViewer::selectionChanged()
{
    const QString path = selectedPath();
    if (path.isEmpty()) return;
    QPixmap pix(path);
    if (pix.isNull())
        m_preview->setText(tr("Impossible de lire cette capture."));
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

    if (QMessageBox::question(this, tr("Supprimer la capture"),
                              tr("Supprimer définitivement la capture sélectionnée ?"),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    if (!QFile::remove(path))
    {
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible de supprimer cette capture."));
        return;
    }
    refresh();
}
