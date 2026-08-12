#include "aboutbox.h"
#include <QMessageBox>
#include <QObject>
#include <QString>

void AboutBox::showAbout(QWidget* parent)
{
    const QString text =
        QStringLiteral("<h2>ECU Mems Manager</h2>")
        + QStringLiteral("<p>Version ") + QStringLiteral(APP_VERSION) + QStringLiteral("</p>")
        + QStringLiteral("<p><b>") + QObject::tr("Compatible avec :") + QStringLiteral("</b></p>")
        + QStringLiteral("<ul>")
        + QStringLiteral("<li>Rover MEMS 1.2</li>")
        + QStringLiteral("<li>Rover MEMS 1.3</li>")
        + QStringLiteral("<li>Rover MEMS 1.6</li>")
        + QStringLiteral("</ul>")
        + QStringLiteral("<p><b>") + QObject::tr("Protocole :") + QStringLiteral("</b> ROSCO</p>")
        + QStringLiteral("<p><b>") + QObject::tr("Développement :") + QStringLiteral("</b> Claude Lespagnol</p>");

    QMessageBox::about(parent, QObject::tr("À propos de ECU Mems Manager"), text);
}
