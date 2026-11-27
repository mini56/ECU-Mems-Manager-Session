#ifndef ECUPAGES_H
#define ECUPAGES_H
class QMainWindow;
namespace Mems16Na { class SessionEvidence; }
unsigned updateAdditionalErrors(QMainWindow *window,bool connected,const Mems16Na::SessionEvidence *evidence);
void updateGaugeTestAvailability(QMainWindow *window,bool connected,const Mems16Na::SessionEvidence *evidence);
// Native Errors and Actuators pages; retains the original ECU controls.
void buildEcuControlPages(QMainWindow *window);
#endif
