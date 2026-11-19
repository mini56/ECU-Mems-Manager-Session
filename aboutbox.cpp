#include "aboutbox.h"
#include <QMessageBox>
#include <QApplication>
#include "nativewindowstyle.h"
#include <QObject>
#include <QString>
#include <QIcon>
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
        + QStringLiteral("<p><b>") + I18n::text(6202) /* EN: Development: */ + QStringLiteral("</b> Claude Lespagnol</p>")
        + QStringLiteral("<p><b>") + I18n::text(6204) /* EN: Special thanks to: */ + QStringLiteral("</b></p>")
        + QStringLiteral("<ul>")
        + QStringLiteral("<li>Leopold Gunther — mems-rosco</li>")
        + QStringLiteral("<li>James Portman — rovermems.com</li>")
        + QStringLiteral("<li>Colin Bourassa — memsgauge / memsreader</li>")
        + QStringLiteral("</ul>")
        + QStringLiteral("<p>") + I18n::text(6205) /* EN: Prior Rover MEMS research and diagnostic tools. */ + QStringLiteral("</p>");

    QMessageBox box(parent);
    box.setWindowTitle(I18n::text(6203) /* EN: About ECU Mems Manager */);
    box.setTextFormat(Qt::RichText);
    box.setText(text);
    box.setStandardButtons(QMessageBox::Ok);
    if (parent && !parent->windowIcon().isNull())
        box.setIconPixmap(parent->windowIcon().pixmap(64, 64));
    box.setStyleSheet(QStringLiteral(
        "QMessageBox QPushButton{background:#ff7a00;color:#101419;border:1px solid #ff9828;border-radius:5px;padding:6px 16px;font-weight:700;min-width:70px;}"
        "QMessageBox QPushButton:hover{background:#ff9828;}"
        "QMessageBox QPushButton:pressed{background:#d96500;}"));
    applyNativeTitleBar(&box,qApp && qApp->property("ecuDarkTheme").toBool());
    box.exec();
}
