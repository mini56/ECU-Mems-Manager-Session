#ifndef SESSIONINFORMATIONMESSAGE_H
#define SESSIONINFORMATIONMESSAGE_H

#include "nativewindowstyle.h"
#include <QApplication>
#include <QColor>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>

// Common visual presentation for the two user-facing information dialogs.
// Content, translations and actions stay with their original callers.
namespace SessionInformationMessage {

inline void configure(QMessageBox &box)
{
    const bool dark = qApp && qApp->property("ecuDarkTheme").toBool();
    box.setObjectName(QStringLiteral("sessionInformationMessageBox"));

    QPalette palette = box.palette();
    const QColor surface(dark ? QStringLiteral("#090e13")
                              : QStringLiteral("#ffffff"));
    const QColor foreground(dark ? QStringLiteral("#e7ecef")
                                 : QStringLiteral("#202936"));
    palette.setColor(QPalette::Window, surface);
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Text, foreground);
    box.setPalette(palette);

    box.setStyleSheet(dark
        ? QStringLiteral(
            "QMessageBox{background:#090e13;color:#e7ecef;}"
            "QMessageBox QLabel{color:#e7ecef;background:transparent;}"
            "QMessageBox QPushButton{background:#ff7a00;color:#101419;"
            "border:1px solid #ff9828;border-radius:5px;"
            "padding:6px 16px;font-weight:700;min-width:72px;}"
            "QMessageBox QPushButton:hover{background:#ff9828;}"
            "QMessageBox QPushButton:pressed{background:#d96500;}")
        : QStringLiteral(
            "QMessageBox{background:#ffffff;color:#202936;}"
            "QMessageBox QLabel{color:#202936;background:transparent;}"
            "QMessageBox QPushButton{background:#ff8a1c;color:#101419;"
            "border:1px solid #d96500;border-radius:5px;"
            "padding:6px 16px;font-weight:700;min-width:72px;}"
            "QMessageBox QPushButton:hover{background:#ffad55;}"
            "QMessageBox QPushButton:pressed{background:#d96500;}"));

    // Reuse the native Qt information glyph, recolored in SESSION orange.
    const QPixmap native = box.style()
        ->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32, 32);
    if (!native.isNull()) {
        QPixmap colored(native.size());
        colored.fill(Qt::transparent);
        QPainter painter(&colored);
        painter.drawPixmap(0, 0, native);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(colored.rect(), QColor(QStringLiteral("#ff8a1c")));
        painter.end();
        box.setIconPixmap(colored);
    }

    // Sets the native DWM dark title bar on supported Windows versions.
    // The helper already handles unsupported Windows/other platforms.
    applyNativeTitleBar(&box, dark);
}

inline void information(QWidget *parent, const QString &title,
                        const QString &message)
{
    QMessageBox box(QMessageBox::Information, title, message,
                    QMessageBox::Ok, parent);
    configure(box);
    box.exec();
}

} // namespace SessionInformationMessage

#endif // SESSIONINFORMATIONMESSAGE_H
