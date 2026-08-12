#ifndef DESKTOPSHORTCUT_H
#define DESKTOPSHORTCUT_H

class DesktopShortcut
{
public:
    // Vérifie/crée le raccourci seulement si la fonction a été demandée dans Options.
    static void ensureIfEnabled();
    static bool create();
};

#endif
