# Test actif du circuit de cliquetis EC — extension de la preuve
27 novembre 2026. Recherche hors ligne des fichiers du dépôt, aucun envoi ECU, aucune modification Manager/DLL.

## Matrice de présence — 43 HEX relus, 36 entrées EC
Méthode : décodage base64 des fichiers .hex binaires, base ROM 2000h, repérage unique de la table étendue par les entrées D0 D0 / F0 F0 / F9 F9, lecture des entrées de 4 octets jusqu'à table suivante ou limite de scan de 240 octets. Tous les 43 fichiers ont une signature de table unique. Contrôle des 24 premiers octets du handler pour EKDMF005/006 et TAP7F017, 52 pour les autres positifs. Le compteur antérieur V1.30 (36/123) est retrouvé en nombre de positifs, mais les 80 autres HEX n'ont pas été relus dans ce lot. Ce n'est pas une homologation sur matériel.

| Profil | Handler EC |
|---|---|
| EKDMF005 | 3E81 |
| EKDMF006 | 3E81 |
| TAEDP000 | 3DBB |
| TAEDP001 | 3DD3 |
| TAP7F017 | 40F0 |
| TAP7P000 | 3DC8 |
| TAP7P003 | 3DB7 |
| TAP7P004 | 3DC8 |
| TAP7P005 | 4059 |
| TAP7P006 | 4064 |
| TAP8F019 | 3DCF |
| TAP8F022 | 3DCF |
| TAP8F023 | 3E5B |
| TAP8F027 | 3E5B |
| TAP8P000 | 3D78 |
| TAP8P001 | 3DC8 |
| TAP8P002 | 3D7E |
| TAP8P003 | 3DC8 |
| TAP8P005 | 3DC8 |
| TAP8P006 | 4059 |
| TAP8P007 | 4064 |
| TAPDF008 | 3E5F |
| TAPDF010 | 424E |
| TAPDF011 | 41BC |
| TAPDF012 | 41B8 |
| TAPDF013 | 426E |
| TAPDP000 | 3DD3 |
| TAPDP001 | 3DD3 |
| TAPDP002 | 3DD1 |
| TAPDP003 | 41B8 |
| TBP7F016 | 40F0 |
| TBP7P000 | 3DC8 |
| TBP7P003 | 3DB7 |
| TBP7P004 | 3DC8 |
| TBP7P005 | 4059 |
| TBP7P006 | 4064 |

Absents dans les 7 HEX relus : EKDMF000/002/003/004/007/008/009. Toutes les entrées positives présentent le même contrôle initial de régime nul (adresse RAM 005E, sauf TAP8P000 0064), modification bit 7 de la copie de port, et écriture au registre matériel. La totalité de la routine et de la reprise asynchrone n'a été désassemblée que sur les profils TAP7F017/TBP7F016.

## Temporisation — 7 profils
XF1 dknk_rosco_time, facteur 0,001333 ms : mot 40000 pour TAP7F017/TBP7F016 (8700+18), EKDMF005 (8400+16), TAP8P000 (7400+16), TAP7P000 (8400+16), TAPDF010/TAP7P005 (8600+18). 53,32 ms nominaux, pas un timeout de communication ni une durée totale.

## Retour et seuils
TAP7F017 ASM 42AE–42EA : régime non nul à la seconde phase -> retour 0 ; sinon conversion canal 0E, 0 brut -> 1, 255 brut -> 254, autres valeurs inchangées. 42D4–42EA rétablit bit 7 et termine l'état interne 0C. Aucune comparaison à un seuil de bon état dans cette routine. Ne pas assimiler 1/254 à une tension mesurée exacte ou une panne ; 0 n'est pas un verdict HS. La routine de régulation normale 2448–253E utilise également canal 0E et calcule différence intégrateur/référence. Les seuils dynamiques knk_thres_map ne sont pas des seuils d'autotest arrêté. La routine F4 de remise à zéro 3B67–3B83 ne rétablit pas explicitement ce bit matériel : ne pas présumer qu'elle suffit en cas de transaction interrompue. Rétablissement normal documenté, récupération sur interruption non validée. Aucune procédure d'essai véhicule autorisée.

## Documentation externe
https://rovermems.com/diagnostics/technical/ : EC non renseignée dans le tableau consulté. https://james-portman.github.io/rover-mems/ECU_Rover_MEMS.pdf (section Knock sensor) décrit le circuit KCP, le capteur piézoélectrique et la gestion du cliquetis sur certains MPi, sans seuils de retour EC. Ces sources ne valident pas de verdict EC.

## Intégration future
Candidat : test circuit de cliquetis, moteur arrêté, profils exacts ; afficher valeur brute et test non concluant tant que seuils et récupération ne sont pas validés. Ne pas afficher OK/HS automatiquement. Aucun changement du programme.
