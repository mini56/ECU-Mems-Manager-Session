#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H
class QMainWindow;
class QWidget;
// Native live overview: owns its cards, sampling, and responsive layout.
void buildOverviewPage(QMainWindow *window);
void fitOverviewPage(QWidget *page);
#endif
