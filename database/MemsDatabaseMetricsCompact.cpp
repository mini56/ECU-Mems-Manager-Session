#include "../i18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QFrame>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QLabel *labelByI18nKey(QWidget *root,int key)
{
    if(!root) return nullptr;
    const QList<QLabel*> labels=root->findChildren<QLabel*>();
    for(QLabel *label:labels){
        if(label && label->property("i18nKey").isValid() && label->property("i18nKey").toInt()==key)
            return label;
    }
    return nullptr;
}

QString metricValue(QWidget *browser,int titleKey)
{
    QLabel *title=labelByI18nKey(browser,titleKey);
    if(!title || !title->parentWidget()) return QStringLiteral("—");
    const QList<QLabel*> labels=title->parentWidget()->findChildren<QLabel*>(QString(),Qt::FindDirectChildrenOnly);
    for(QLabel *label:labels){
        if(label && label!=title && !label->property("i18nKey").isValid())
            return label->text();
    }
    return QStringLiteral("—");
}

void hideMetricCards(QWidget *browser)
{
    if(!browser) return;
    const int keys[]={7168,7169,7170,7171,7172,7173};
    for(int key:keys){
        if(QLabel *title=labelByI18nKey(browser,key))
            if(QWidget *card=title->parentWidget()) card->hide();
    }
}

void showStatistics(QWidget *browser)
{
    if(!browser) return;
    QDialog dialog(browser);
    dialog.setWindowTitle(I18n::text(7374));
    dialog.setMinimumWidth(470);
    dialog.setModal(true);

    QVBoxLayout *root=new QVBoxLayout(&dialog);
    root->setContentsMargins(14,14,14,14);
    root->setSpacing(10);

    QLabel *subtitle=new QLabel(I18n::text(7375),&dialog);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral("color:#aeb9c2;"));
    root->addWidget(subtitle);

    QFrame *panel=new QFrame(&dialog);
    panel->setStyleSheet(QStringLiteral(
        "QFrame{background:#10171d;border:1px solid #34414b;border-radius:6px;}"));
    QGridLayout *grid=new QGridLayout(panel);
    grid->setContentsMargins(12,10,12,10);
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(8);

    const int keys[]={7168,7169,7170,7171,7172,7173};
    for(int i=0;i<6;++i){
        QLabel *name=new QLabel(I18n::text(keys[i]),panel);
        name->setStyleSheet(QStringLiteral("color:#dce3e8;border:0;background:transparent;"));
        QLabel *value=new QLabel(metricValue(browser,keys[i]),panel);
        value->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        QFont font=value->font();
        font.setBold(true);
        font.setPointSizeF(font.pointSizeF()+1.5);
        value->setFont(font);
        value->setStyleSheet(QStringLiteral("color:#ff9828;border:0;background:transparent;"));
        grid->addWidget(name,i,0);
        grid->addWidget(value,i,1);
    }
    grid->setColumnStretch(0,1);
    root->addWidget(panel);

    QPushButton *closeButton=new QPushButton(I18n::text(7376),&dialog);
    closeButton->setMinimumHeight(30);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton{background:#182129;color:#e9eef1;border:1px solid #3b4852;border-radius:4px;padding:4px 12px;}"
        "QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}"));
    QObject::connect(closeButton,&QPushButton::clicked,&dialog,&QDialog::accept);
    root->addWidget(closeButton,0,Qt::AlignRight);

    dialog.exec();
}

class MetricsCompactController:public QObject
{
public:
    explicit MetricsCompactController(QWidget *browser)
        : QObject(browser),m_browser(browser),m_button(nullptr)
    {
        if(!m_browser) return;
        apply();
        m_browser->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        if(watched==m_browser && event){
            if(event->type()==QEvent::Show || event->type()==QEvent::Resize)
                QTimer::singleShot(0,this,[this](){apply();});
            else if(event->type()==QEvent::LanguageChange)
                QTimer::singleShot(0,this,[this](){
                    if(m_button) m_button->setText(I18n::text(7374));
                    hideMetricCards(m_browser);
                });
        }
        return QObject::eventFilter(watched,event);
    }

private:
    void apply()
    {
        if(!m_browser) return;
        hideMetricCards(m_browser);

        if(m_button){
            m_button->show();
            return;
        }

        QLineEdit *search=m_browser->findChild<QLineEdit*>();
        if(!search || !search->parentWidget()) return;
        QWidget *searchBox=search->parentWidget();
        QHBoxLayout *layout=qobject_cast<QHBoxLayout*>(searchBox->layout());
        if(!layout) return;

        m_button=new QPushButton(I18n::text(7374),searchBox);
        m_button->setObjectName(QStringLiteral("databaseStatisticsButton"));
        m_button->setMinimumHeight(30);
        m_button->setMaximumWidth(190);
        m_button->setStyleSheet(QStringLiteral(
            "QPushButton{background:#151e25;color:#e9eef1;border:1px solid #3b4852;border-radius:4px;padding:4px 10px;}"
            "QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}"));

        const int insert=qMax(0,layout->count()-1);
        layout->insertWidget(insert,m_button);
        QObject::connect(m_button,&QPushButton::clicked,this,[this](){showStatistics(m_browser);});
    }

    QPointer<QWidget> m_browser;
    QPointer<QPushButton> m_button;
};

class MetricsCompactInstaller:public QObject
{
public:
    explicit MetricsCompactInstaller(QObject *parent=nullptr):QObject(parent){}

    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QWidget *widget=qobject_cast<QWidget*>(watched);
        if(widget && event && (event->type()==QEvent::Show || event->type()==QEvent::Polish) &&
           widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
            install(widget);
        return QObject::eventFilter(watched,event);
    }

    void scan()
    {
        for(QWidget *widget:QApplication::allWidgets())
            if(widget && widget->objectName()==QStringLiteral("memsDatabaseBrowser"))
                install(widget);
    }

private:
    void install(QWidget *browser)
    {
        if(!browser || browser->property("databaseMetricsCompactInstalled").toBool()) return;
        browser->setProperty("databaseMetricsCompactInstalled",true);
        QPointer<QWidget> guarded(browser);
        QTimer::singleShot(650,this,[guarded](){
            if(guarded) new MetricsCompactController(guarded);
        });
    }
};

void installMetricsCompact()
{
    QCoreApplication *core=QCoreApplication::instance();
    if(!core) return;
    MetricsCompactInstaller *installer=new MetricsCompactInstaller(core);
    core->installEventFilter(installer);
    QTimer::singleShot(1000,installer,[installer](){installer->scan();});
}

}

Q_COREAPP_STARTUP_FUNCTION(installMetricsCompact)
