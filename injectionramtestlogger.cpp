#include "injectionramtestlogger.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QRegularExpression>

namespace InjectionRamTestLog {

QString directory()
{
    QString base=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(base.isEmpty()) base=QDir::homePath();
    const QString path=QDir(base).filePath(QStringLiteral("ECU MEMS Manager/InjectionTests"));
    QDir().mkpath(path);
    return path;
}

bool isFinalLog(const QString &text)
{
    const QString trimmed=text.trimmed();
    static const QRegularExpression header(QStringLiteral("^TEST (?:DYNAMIQUE )?RAM TEMPS INJECTION — ([A-Z0-9]{8})(?:\\r?\\n|$)"));
    return header.match(trimmed).hasMatch();
}

bool writeFinalText(const QString &text,const QString &destinationDirectory,QString *savedPath)
{
    if(savedPath) savedPath->clear();
    if(!isFinalLog(text) || destinationDirectory.isEmpty())
        return false;
    if(!QDir().mkpath(destinationDirectory))
        return false;

    const QString suffix=QUuid::createUuid().toString(QUuid::Id128).left(8);
    static const QRegularExpression header(QStringLiteral("^TEST (?:DYNAMIQUE )?RAM TEMPS INJECTION — ([A-Z0-9]{8})(?:\\r?\\n|$)"));
    const QString firmware=header.match(text.trimmed()).captured(1);
    const QString fileName=QStringLiteral("Injection_RAM_%1_%2_%3.txt")
        .arg(firmware,QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz")),suffix);
    const QString path=QDir(destinationDirectory).filePath(fileName);
    QSaveFile file(path);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Text))
        return false;

    QByteArray bytes=text.toUtf8();
    if(!bytes.endsWith('\n')) bytes.append('\n');
    if(file.write(bytes)!=bytes.size())
    {
        file.cancelWriting();
        return false;
    }
    if(!file.commit())
        return false;
    if(savedPath) *savedPath=path;
    return true;
}

void beginPhase(QPlainTextEdit *output)
{
    if(output)
        output->setProperty("injectionLogSaveAttempted",false);
}

void saveFinishedPhase(QPlainTextEdit *output)
{
    if(!output || output->property("injectionLogSaveAttempted").toBool())
        return;
    const QString text=output->toPlainText();
    if(!isFinalLog(text))
        return;
    output->setProperty("injectionLogSaveAttempted",true);

    QString savedPath;
    if(!writeFinalText(text,directory(),&savedPath))
    {
        output->appendPlainText(QStringLiteral(
            "\nERREUR : impossible d'enregistrer automatiquement le journal du test."));
        return;
    }

    output->setProperty("injectionLogSavedPath",savedPath);
    output->appendPlainText(QStringLiteral("\nFichier enregistré automatiquement :\n%1")
                            .arg(QDir::toNativeSeparators(savedPath)));
}

void addFolderButton(QGroupBox *session,QGridLayout *grid)
{
    if(!session || !grid ||
       session->findChild<QPushButton*>(QStringLiteral("injectionTestFolderButton")))
        return;
    const QString folder=directory();
    QPushButton *button=new QPushButton(QStringLiteral("Ouvrir le dossier des tests"),session);
    button->setObjectName(QStringLiteral("injectionTestFolderButton"));
    button->setMinimumHeight(30);
    button->setToolTip(QDir::toNativeSeparators(folder));
    grid->addWidget(button,grid->rowCount(),0,1,4);
    QObject::connect(button,&QPushButton::clicked,session,[](){
        QDesktopServices::openUrl(QUrl::fromLocalFile(directory()));
    });
}

} // namespace InjectionRamTestLog
