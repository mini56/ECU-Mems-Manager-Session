#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

static QWidget *realPage(QWidget *tab)
{
    if (!tab) return nullptr;
    if (QScrollArea *scroll=qobject_cast<QScrollArea*>(tab)) return scroll->widget();
    return tab;
}

static QString subtitleFor(const QString &title)
{
    const QString t=title.toLower();
    if (t.contains(QStringLiteral("régl"))) return QStringLiteral("CONFIGURATION ECU ET PARAMÈTRES DE FONCTIONNEMENT");
    if (t.contains(QStringLiteral("toutes les mesures"))) return QStringLiteral("MESURES ECU EN TEMPS RÉEL");
    if (t.contains(QStringLiteral("erreur"))) return QStringLiteral("DÉFAUTS, ANOMALIES ET ÉTATS SURVEILLÉS");
    if (t.contains(QStringLiteral("actionneur"))) return QStringLiteral("COMMANDES ET TESTS DES ACTIONNEURS");
    if (t.contains(QStringLiteral("toutes les données"))) return QStringLiteral("DONNÉES BRUTES ET VALEURS DÉCODÉES");
    if (t.contains(QStringLiteral("interactif"))) return QStringLiteral("COMMANDES MANUELLES ET MODE TECHNIQUE");
    if (t.contains(QStringLiteral("ecu / rosco"))) return QStringLiteral("SESSION TECHNIQUE ECU / ROSCO");
    if (t.contains(QStringLiteral("diagnostic"))) return QStringLiteral("CONTRÔLE AUTOMATIQUE ET RAPPORT DE SANTÉ");
    if (t.contains(QStringLiteral("enregistreur"))) return QStringLiteral("ENREGISTREMENT DES SESSIONS DE DIAGNOSTIC");
    return QStringLiteral("ECU MEMS MANAGER");
}

static void decoratePage(QWidget *page, const QString &title)
{
    if (!page || page->property("modernPageComposed").toBool()) return;
    if (page->objectName()==QStringLiteral("overview_tab") ||
        QString::fromLatin1(page->metaObject()->className())==QStringLiteral("AnalysisTab") ||
        page->objectName()==QStringLiteral("recorder_tab")) {
        page->setProperty("modernPageComposed", true);
        return;
    }

    QBoxLayout *box=qobject_cast<QBoxLayout*>(page->layout());
    if (!box) {
        page->setProperty("modernPageComposed", true);
        return;
    }

    box->setContentsMargins(14,12,14,12);
    box->setSpacing(qMax(8,box->spacing()));

    QFrame *hero=new QFrame(page);
    hero->setObjectName(QStringLiteral("modernPageHero"));
    hero->setAttribute(Qt::WA_StyledBackground,true);
    hero->setStyleSheet(QStringLiteral(
        "#modernPageHero{background:#10161c;border:1px solid #2b343d;border-radius:4px;}"
        "#modernPageHero QLabel{background:transparent;border:0;}"
    ));
    QVBoxLayout *heroLayout=new QVBoxLayout(hero);
    heroLayout->setContentsMargins(14,8,14,8);
    heroLayout->setSpacing(1);

    QLabel *mainTitle=new QLabel(title.toUpper(),hero);
    QFont titleFont=mainTitle->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(10.5);
    mainTitle->setFont(titleFont);
    mainTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;"));

    QLabel *subtitle=new QLabel(subtitleFor(title),hero);
    QFont subFont=subtitle->font();
    subFont.setPointSizeF(7.8);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing,0.4);
    subtitle->setFont(subFont);
    subtitle->setStyleSheet(QStringLiteral("color:#8e99a3;background:transparent;"));

    heroLayout->addWidget(mainTitle);
    heroLayout->addWidget(subtitle);
    box->insertWidget(0,hero);
    page->setProperty("modernPageComposed", true);
}

class PageComposer : public QObject
{
public:
    explicit PageComposer(QObject *parent=nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if ((event->type()==QEvent::Show || event->type()==QEvent::Polish) && watched) {
            QMainWindow *window=qobject_cast<QMainWindow*>(watched);
            if (window && window->objectName()==QStringLiteral("MainWindow") &&
                !window->property("pageComposerInstalled").toBool()) {
                QTabWidget *tabs=window->findChild<QTabWidget*>(QStringLiteral("Tab_main"));
                if (tabs) {
                    window->setProperty("pageComposerInstalled",true);
                    QTimer::singleShot(250,window,[window,tabs](){
                        for (int i=0;i<tabs->count();++i) {
                            QWidget *page=realPage(tabs->widget(i));
                            decoratePage(page,tabs->tabText(i));
                        }
                    });
                }
            }
        }
        return QObject::eventFilter(watched,event);
    }
};

void installPageComposer()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if (!app) return;
    PageComposer *composer=new PageComposer(app);
    app->installEventFilter(composer);
}

}

Q_COREAPP_STARTUP_FUNCTION(installPageComposer)
