#ifndef TOOLTIPBUBBLEMANAGER_H
#define TOOLTIPBUBBLEMANAGER_H

#include <QObject>
class QWidget;

class TooltipBubbleManager : public QObject
{
    Q_OBJECT
public:
    explicit TooltipBubbleManager(QWidget *root);
    static void install(QWidget *root);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void registerWidget(QWidget *widget);
    void ensureBubble(QWidget *widget);
};
#endif
