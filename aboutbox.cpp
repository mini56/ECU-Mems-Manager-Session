#include "aboutbox.h"
#include <QMessageBox>
#include <QObject>
#include <QString>
#include "i18n.h"
#define tr I18n::text

void AboutBox::showAbout(QWidget* parent)
{
    const QString text =
        QStringLiteral("<h2>ECU Mems Manager</h2>")
        + QStringLiteral("<p>Version ") + QStringLiteral(APP_VERSION) + QStringLiteral("</p>")
        + QStringLiteral("<p><b>") + I18n::text(6200) /* EN: Compatible with: */ + QStringLiteral("</b></p>")
        + QStringLiteral("<ul>")
        + QStringLiteral("<li>Rover MEMS 1.2</li>")
        + QStringLiteral("<li>Rover MEMS 1.3</li>")
        + QStringLiteral("<li>Rover MEMS 1.6</li>")
        + QStringLiteral("</ul>")
        + QStringLiteral("<p><b>") + I18n::text(6201) /* EN: Protocol: */ + QStringLiteral("</b> ROSCO</p>")
        + QStringLiteral("<p><b>") + I18n::text(6202) /* EN: Development: */ + QStringLiteral("</b> Claude Lespagnol</p>");

    QMessageBox::about(parent, I18n::text(6203) /* EN: About ECU Mems Manager */, text);
}
