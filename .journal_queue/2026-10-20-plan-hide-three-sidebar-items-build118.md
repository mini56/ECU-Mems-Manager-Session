## 2026-10-20 — PLAN DÉSACTIVATION DE TROIS ACCÈS DANS LE MENU — BUILD 118

Demande utilisateur expresse : masquer UNIQUEMENT les accès de navigation aux pages `TEST ECU 1.9`, `IA MEMS`, `ECU ROSCO`, sans effacer les pages ni modifier leur fonctionnalité. Ne pas changer les autres entrées/pages.

Implémentation dans l'unique `navigationorderpatch.cpp` : conserver les 14 vrais widgets et les 14 identités/indices pour ne pas casser les signaux `currentRowChanged -> setCurrentIndex`; rendre invisibles uniquement les trois `QListWidgetItem` de rang 13, 6 et 9. Conserver leur code et les données, et les recréations lors des changements de langue. Ajuster la taille des rangées aux 11 visibles. Test de source garantissant les 11 boutons affichés et les trois cachés; vérification compilation, smoke Windows et installateur.

Version `BUILD 118` nouvelle, branche existante `tmp-ia-clarification-test-1b106ee`, repos `mini56/ECU-Mems-Manager-Session`. Maintenir `MEMSX64` et `main` inchangés, Setup #117 et travaux RAVEMEMS/IA intacts. Installer en parallèle pour essai.
