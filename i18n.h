#ifndef I18N_H
#define I18N_H

#include <QObject>
#include <QHash>
#include <QString>

class QApplication;
class QWidget;

class I18n : public QObject
{
    Q_OBJECT
public:
    static bool load(const QString &languageCode);
    static QString text(const char *source);
    static QString text(const QString &source);
    static QString text(int key);
    static QString language();
    static void install(QApplication *app);
    static void apply(QWidget *root);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit I18n(QObject *parent = nullptr);
    static I18n *instance();
    bool loadDictionary(const QString &fileName);
    QString lookup(const QString &source) const;
    QString lookupKey(int key) const;
    void applyObject(QObject *object);

    QString m_language;
    QHash<QString, QString> m_dictionary;
    QHash<int, QString> m_keyDictionary;
    bool m_applying = false;
};

#endif
