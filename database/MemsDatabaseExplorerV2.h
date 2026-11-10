#ifndef MEMS_DATABASE_EXPLORER_V2_H
#define MEMS_DATABASE_EXPLORER_V2_H

class QWidget;

// Initializes the native indexed search on its owning browser widget.
// Returns false without binding any UI signals if the index cannot be opened.
bool initializeMemsDatabaseExplorer(QWidget *browser);

#endif
