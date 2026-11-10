#ifndef IAMEMSDOCUMENTARYPIPELINE_H
#define IAMEMSDOCUMENTARYPIPELINE_H

class QWidget;
class QLineEdit;
class QPushButton;
class QTextBrowser;

// Connects the IA tab to the documentary identity resolution pipeline.
// Owned by the tab for the entire duration of its lifetime.
void connectIaMemsDocumentaryPipeline(QWidget *tab, QLineEdit *question,
                                     QPushButton *sendButton, QTextBrowser *transcript);

#endif
