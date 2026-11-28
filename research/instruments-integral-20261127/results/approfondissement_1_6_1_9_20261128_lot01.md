# Approfondissement après recherche inverse V2 — lot 01
Corpus et registre V2 : commit 9c6c443eabcf5c23dd1235b7e9c892de50802dd7. Recherche statique, aucune commande envoyée à un ECU, aucune modification Manager/DLL.

## KLH3G015 (MEMS 1.9, identité 900015) : nouveau mécanisme de test du compte-tours
Sources ASM fee5c0a8e7477a3e59486ec62af7222ebc6f0825, XF3 7d3e91edc859d0fe90f0dbcbe7dbc70ea7ef6f09, ROM SHA256 a5a54fecc7b3af1eac63a7963d0883891cd9b8ac4f9071f99286ac9074a98b76.
La table normale confirme 6B -> 54DF et 63 -> 54E9. 54DF charge 030E, positionne 20h, 54E9 charge 030E et efface 20h. Convergence 54F1 écrit 030E ; 54F6-54FE efface 20h de 0308 ; 5503-5506 force la sortie à zéro initialement (22 -> 1FFE). XF3 : iqport3_fdir 030E, iqport3_fmsk 0308, iq_tacho_fmsk bit 5, iqport3_tdir 0022.
Contrairement au TFH7G029 (6B charge une consigne 3000 tr/min et 63 charge 0 dans tacho_display_speed), le KLH3G015 utilise un générateur périodique. 447B-44AA : si 0308/20 est effacé, test 030E/20 ; s'il est positionné, XOR 22 avec 20h à 4493 puis écriture 1FFE à 4496 ; chargement du délai 14h[40] à 449B, addition au compteur (33C+4) = 0340 à 44A0-44A5. XF3 identifie tacho_sw_time offset 0014, unité 0,001 ms, et tacho_next_rti 0340. ROM à 9514 (tune 9500+14) = 88 13, soit 5000 unités = 5 ms entre inversions, donc fréquence théorique 100 Hz. Correspond à 3000 tr/min uniquement pour un instrument configuré à 2 impulsions par tour ; cette équivalence n'est pas certifiée pour le véhicule. Ordonnancement : 303F-3097 examine 0340/0342 et programme le prochain événement ; table ASM 3ECE contient pointeur 447B. Fonction normale : 2304-2311 bascule la sortie en fonction d'événements moteur lorsque le masque 0308/20 est activé ; 4124-4131 peut la ramener à zéro. Annulation : F4 -> 4DDE -> 4DFD -> 5576 ; 5607-560F repositionne 0308/20. Aucun chargement de régime fictif moteur.
Statut : NOUVEAU CANDIDAT FORT, chaîne logicielle commande -> sortie -> générateur et retour documentée ; vérifier la compatibilité physique et la signification de 100 Hz, la programmation événementielle complète et les protections Manager avant activation. Ne pas annoncer 3000 tr/min sans preuve de calibration du combiné. Ne pas utiliser 63 seul comme restauration, F4 réactive la stratégie.

## TAP8P007 (MEMS 1.6 NA, A80007) : nouveau candidat V2, distinction jauge/papillon
ASM a8974d01d3263882e8476ceba63ed5551430d9fa, XF3 1357574c640c4d4de68827c737a40c66582846cb.
00/10 -> 3EBF/3EBA, AC bits 6/7 (XF3 flagbyte9). Les seules écritures spécifiques sont 3EBA/3EBF/3EC2, effacement 3ADA ; aucun test JBC/JBS de AC bits 6/7 dans le relevé. AD bits 0/1, en revanche, sont lus à 5148/514B ; XF3 flagbyte10 les identifie explicitement comme forçage du contact papillon, pas jauge. Ne pas confondre les deux commandes adjacentes. Pas de m_tmpgauge_dr ni m_tempg_pwm_cnt ; A6 fault_mask. Aucune chaîne de jauge supplémentaire établie.

## TBP7F015 (MEMS 1.9, 900015) : architecture atypique
ASM b085f990e237531cf4e88c9af4918c27b77c7a1d, XF3 0941fa0df403d3226b18a4abd978f2cbfb78c4a0.
00/10 -> 3F28/3F23 ; 8B bits 6/7, effacement 42F4 ; aucun JBC/JBS de ces bits dans le relevé. Les tests de 8C bits 0/1 à 5D18/5D1B correspondent au papillon selon XF3, pas à la jauge. Aucune variable dédiée au PWM jauge. Ne pas intégrer à ce stade.

## KBHBG009 (MEMS 1.9, 900009) : exemple de faux candidat dû aux masques partagés
ASM fb54ca321b3c19b064f677fdc579b93e2906639f, XF3 018f487f102e4031b5914a3ec515d4c556a018e4.
XF3 iq_tacho_fmsk 0308 bit 5. La routine 4C3F-4C62 combine des masques de port entiers (0308, 030E, 0302) puis écrit 1FFE ; une référence à 0308 n'est pas une commande du compte-tours. Par exemple 5290/529A modifie 02h et 52A7-52AF efface 02h dans 0308, pas 20h. Pas de 63/6B dans table normale, aucun symbole de consigne ou minuterie tacho dans ce XF3. Aucun chemin de test spécifique démontré. Analyse non exhaustive des accès indirects.

## Conséquence pour le corpus
Le générateur périodique découvert sur KLH3G015 doit être recherché dans toutes les ROM 1.9, indépendamment du numéro de commande, en vérifiant masque de sortie, temporisateur, calibration individuelle, retour à la stratégie et identité. Le chiffre de 40 précédemment classés « logique uniquement » ne doit pas être pris comme une exclusion : KLH3G015 prouve un mécanisme distinct. Les références de masque partagé doivent être filtrées bit par bit, sans éliminer les accès indirects non résolus. Les profils connus ne sont pas une preuve d'exhaustivité.
