#include "MemsDocumentaryImageViewer.h"

#include <QColor>
#include <QDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

void MemsDocumentaryImageViewer::show(QWidget *parent,const QString &path)
{
    const QPixmap original(path);
    if(original.isNull()){
        QMessageBox::warning(parent,QStringLiteral("Page constructeur"),
                             QStringLiteral("Impossible de lire l'image :\n%1").arg(path));
        return;
    }
    QDialog dialog(parent);
    dialog.setObjectName(QStringLiteral("memsDocumentaryImageViewer"));
    dialog.setWindowTitle(QStringLiteral("Page constructeur — %1")
                          .arg(QFileInfo(path).fileName()));
    dialog.resize(1000,740);
    QVBoxLayout *layout=new QVBoxLayout(&dialog);
    layout->setContentsMargins(8,8,8,8);
    layout->setSpacing(7);

    QHBoxLayout *actions=new QHBoxLayout;
    QPushButton *smaller=new QPushButton(QStringLiteral("−"),&dialog);
    QPushButton *bigger=new QPushButton(QStringLiteral("+"),&dialog);
    QPushButton *fit=new QPushButton(QStringLiteral("Ajuster"),&dialog);
    QPushButton *actual=new QPushButton(QStringLiteral("100 %"),&dialog);
    actions->addWidget(new QLabel(QFileInfo(path).fileName(),&dialog));
    actions->addStretch(1);
    actions->addWidget(smaller);
    actions->addWidget(bigger);
    actions->addWidget(fit);
    actions->addWidget(actual);
    layout->addLayout(actions);

    QGraphicsScene scene(&dialog);
    scene.addPixmap(original);
    scene.setSceneRect(QRectF(original.rect()));
    QGraphicsView *view=new QGraphicsView(&scene,&dialog);
    view->setObjectName(QStringLiteral("memsDocumentaryImageGraphicsView"));
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setRenderHint(QPainter::SmoothPixmapTransform,true);
    view->setBackgroundBrush(QColor(QStringLiteral("#0a1015")));
    layout->addWidget(view,1);
    QObject::connect(smaller,&QPushButton::clicked,view,[view](){view->scale(0.8,0.8);});
    QObject::connect(bigger,&QPushButton::clicked,view,[view](){view->scale(1.25,1.25);});
    QObject::connect(fit,&QPushButton::clicked,view,[view,&scene](){
        view->fitInView(scene.sceneRect(),Qt::KeepAspectRatio);
    });
    QObject::connect(actual,&QPushButton::clicked,view,[view](){view->resetTransform();});
    QTimer::singleShot(0,&dialog,[view,&scene](){
        view->fitInView(scene.sceneRect(),Qt::KeepAspectRatio);
    });
    dialog.exec();
}
