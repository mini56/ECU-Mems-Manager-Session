#ifndef UISHELL_H
#define UISHELL_H
class QMainWindow;
class MainWindow;
bool ensureMappedInjectionPage(MainWindow *window);
bool uiShellNavigationReady(QMainWindow *window, bool requireVisible);
// The MainWindow owns the UI shell lifecycle; this module builds and sizes it.
void buildUiShell(QMainWindow *window);
void resizeUiShell(QMainWindow *window);
void retranslateUiShell(QMainWindow *window);
void refreshUiShellTheme(QMainWindow *window);
#endif
