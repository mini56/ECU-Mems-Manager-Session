#ifndef SESSIONPROGRAMUPDATE_H
#define SESSIONPROGRAMUPDATE_H
#include <QObject>
#include <QPointer>
#include <QUrl>
class QMainWindow;
class QPushButton;

// Only public program releases. Documentary packs and ECU paths are untouched.
class SessionProgramUpdate final : public QObject
{
public:
    explicit SessionProgramUpdate(QMainWindow *window);
private:
    void check(bool interactive);
    void handleManifest(const QByteArray &data,bool interactive);
    void tellUser(const QString &message) const;
    QPointer<QMainWindow> m_window;
    QPointer<QPushButton> m_indicator;
    QUrl m_downloadUrl;
    bool m_checking=false;
};
#endif
