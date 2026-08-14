#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

namespace {

static QIcon navIcon(int kind)
{
    QPixmap pm(22,22);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing,true);
    QPen pen(QColor("#ff8a1c"),1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF r(3.5,3.5,15,15);
    switch(kind%10) {
    case 0: // gauge
        p.drawArc(r,35*16,290*16); p.drawLine(QPointF(11,11),QPointF(15.5,7.5)); p.drawEllipse(QPointF(11,11),1.2,1.2); break;
    case 1: // sliders
        p.drawLine(4,6,18,6); p.drawLine(4,11,18,11); p.drawLine(4,16,18,16);
        p.drawEllipse(QPointF(8,6),1.8,1.8); p.drawEllipse(QPointF(14,11),1.8,1.8); p.drawEllipse(QPointF(10,16),1.8,1.8); break;
    case 2: // table
        p.drawRoundedRect(r,2,2); p.drawLine(4,8,18,8); p.drawLine(9,4,9,18); p.drawLine(14,4,14,18); break;
    case 3: // warning
        p.drawPolygon(QPolygonF()<<QPointF(11,3.5)<<QPointF(19,18)<<QPointF(3,18)); p.drawLine(11,8,11,13); p.drawPoint(11,16); break;
    case 4: // actuator / bolt
        p.drawPolyline(QPolygonF()<<QPointF(12,3)<<QPointF(6.5,11)<<QPointF(11,11)<<QPointF(8.5,19)<<QPointF(16,9.5)<<QPointF(12,9.5)<<QPointF(12,3)); break;
    case 5: // data
        p.drawRoundedRect(r,2,2); p.drawLine(6,8,16,8); p.drawLine(6,12,16,12); p.drawLine(6,16,13,16); break;
    case 6: // terminal
        p.drawRoundedRect(r,2,2); p.drawPolyline(QPolygonF()<<QPointF(6,8)<<QPointF(9,11)<<QPointF(6,14)); p.drawLine(11,14,16,14); break;
    case 7: // chart
        p.drawLine(4,18,18,18); p.drawLine(4,18,4,4); p.drawPolyline(QPolygonF()<<QPointF(5,15)<<QPointF(8,11)<<QPointF(11,13)<<QPointF(15,7)<<QPointF(18,9)); break;
    case 8: // ECU chip
        p.drawRoundedRect(QRectF(6,6,10,10),2,2); for(int i=0;i<3;i++){p.drawLine(3,8+i*3,6,8+i*3);p.drawLine(16,8+i*3,19,8+i*3);} break;
    default: // diagnostic shield
        p.drawPolygon(QPolygonF()<<QPointF(11,3.5)<<QPointF(17,6)<<QPointF(16,13)<<QPointF(11,18.5)<<QPointF(6,13)<<QPointF(5,6)); p.drawPolyline(QPolygonF()<<QPointF(7.5,11)<<QPointF(10,13.5)<<QPointF(14.5,8.5)); break;
    }
    return QIcon(pm);
}

static void polishNavigation(QMainWindow *w)
{
    QListWidget *nav=w?w->findChild<QListWidget*>(QStringLiteral("uiRebuildNav")):nullptr;
    if(!nav) return;
    nav->setIconSize(QSize(18,18));
    QFont f=nav->font(); f.setPointSizeF(8.5); f.setWeight(QFont::DemiBold); nav->setFont(f);
    int widest=0;
    QFontMetrics fm(f);
    for(int i=0;i<nav->count();++i) {
        QListWidgetItem *it=nav->item(i);
        if(!it) continue;
        it->setIcon(navIcon(i));
        it->setSizeHint(QSize(0,qMax(34,fm.height()+15)));
        widest=qMax(widest,fm.horizontalAdvance(it->text()));
    }
    nav->setFixedWidth(qBound(158,widest+58,230));
    nav->setStyleSheet(QStringLiteral(
        "#uiRebuildNav{background:#0a1015;color:#bfc8ce;border:0;border-right:1px solid #26313a;padding:7px 0;}"
        "#uiRebuildNav::item{margin:1px 6px 1px 0;padding:4px 9px;border-left:3px solid transparent;border-radius:0 4px 4px 0;}"
        "#uiRebuildNav::item:hover{background:#131c23;color:#f3f6f8;}"
        "#uiRebuildNav::item:selected{background:#211a14;color:#ff9b3b;border-left:3px solid #ff7a00;}"));
}

static void polishHeader(QMainWindow *w)
{
    if(!w) return;
    QFrame *header=w->findChild<QFrame*>(QStringLiteral("uiRebuildHeader"));
    if(!header) return;
    const int h=qMax(46,w->fontMetrics().lineSpacing()*3+8);
    header->setFixedHeight(qMin(h,60));
    header->setStyleSheet(QStringLiteral(
        "#uiRebuildHeader{background:#070c10;border:0;border-bottom:1px solid #35414a;}"
        "#uiRebuildHeader QLabel{background:transparent;border:0;}"
        "#m_ecuIdLabel,#m_communicationsStatusLabel{color:#dbe2e7;font-weight:700;padding:3px 8px;}"
        "#m_connectButton{background:#ff7a00;color:#101418;border:1px solid #ffa041;border-radius:4px;font-weight:800;padding:5px 14px;}"
        "#m_connectButton:hover{background:#ff8d21;}"
        "#m_disconnectButton{background:#171f26;color:#dbe2e7;border:1px solid #35414a;border-radius:4px;font-weight:700;padding:5px 14px;}"
        "#m_disconnectButton:hover{border-color:#ff7a00;color:#ff9b3b;}"
        "#m_connectButton:disabled,#m_disconnectButton:disabled{background:#12191f;color:#5c6871;border-color:#26313a;}"));
}

static void polishStatus(QMainWindow *w)
{
    QFrame *bar=w?w->findChild<QFrame*>(QStringLiteral("uiRebuildStatus")):nullptr;
    if(!bar) return;
    bar->setFixedHeight(qMax(31,w->fontMetrics().height()+14));
    bar->setStyleSheet(QStringLiteral(
        "#uiRebuildStatus{background:#070c10;border-top:1px solid #2b3640;}"
        "#uiRebuildStatus QLabel{background:transparent;color:#bfc8ce;border-right:1px solid #253039;padding:0 8px;}"
        "#uiRebuildStatus QLineEdit{background:#0a1117;color:#dfe5e9;border:1px solid #34414b;border-radius:4px;padding:3px 7px;}"
        "#uiRebuildStatus QPushButton{background:#101820;color:#dce3e8;border:1px solid #394650;border-radius:4px;padding:3px 9px;font-weight:700;}"
        "#uiRebuildStatus QPushButton:hover{border-color:#ff7a00;color:#ff9b3b;}"
        "#m_startLoggingButton{border-color:#8a4a17;color:#ff9b3b;}"
        "#m_stopLoggingButton{color:#aab5bc;}"));
}

static void polishCards(QMainWindow *w)
{
    if(!w) return;
    const char *names[]={"errorsStored","errorsLive","actuatorListCard","actuatorIacCard","settingsMetrics","settingsStates","settingsAdjust"};
    for(const char *name:names) {
        if(QFrame *f=w->findChild<QFrame*>(QString::fromLatin1(name))) {
            f->setStyleSheet(QStringLiteral("#%1{background:#0d141a;border:1px solid #2e3a44;border-radius:6px;}").arg(QString::fromLatin1(name)));
        }
    }
}

static void applyPolish(QMainWindow *w)
{
    if(!w) return;
    if(w->menuBar()) w->menuBar()->setFixedHeight(qMax(23,w->fontMetrics().height()+7));
    polishNavigation(w);
    polishHeader(w);
    polishStatus(w);
    polishCards(w);
}

class VisualPolishInstaller : public QObject
{
public: explicit VisualPolishInstaller(QObject *p=nullptr):QObject(p) {}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override
    {
        QMainWindow *w=qobject_cast<QMainWindow*>(watched);
        if(!w || w->objectName()!=QStringLiteral("MainWindow")) return QObject::eventFilter(watched,event);
        if((event->type()==QEvent::Show || event->type()==QEvent::Polish) && !w->property("visualPolishScheduled").toBool()) {
            w->setProperty("visualPolishScheduled",true);
            QTimer::singleShot(980,w,[w](){applyPolish(w);});
        } else if(event->type()==QEvent::Resize && w->property("visualPolishScheduled").toBool()) {
            QTimer::singleShot(0,w,[w](){applyPolish(w);});
        }
        return QObject::eventFilter(watched,event);
    }
};

void installVisualPolish()
{
    QApplication *app=qobject_cast<QApplication*>(QCoreApplication::instance());
    if(app) app->installEventFilter(new VisualPolishInstaller(app));
}

}

Q_COREAPP_STARTUP_FUNCTION(installVisualPolish)
