#ifndef TOOLTIPBUBBLEMANAGER_H
#define TOOLTIPBUBBLEMANAGER_H

#include <QObject>
#include <QPixmap>
class QWidget;

class TooltipBubbleManager : public QObject
{
    Q_OBJECT
public:
    explicit TooltipBubbleManager(QWidget *root);
    static void install(QWidget *root);
    static QPixmap helpBubblePixmap(bool darkTheme);
    static constexpr int helpIconSize() { return 14; }
    static constexpr int helpHitSize() { return 18; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void registerWidget(QWidget *widget);
    void ensureBubble(QWidget *widget);
};
#endif
