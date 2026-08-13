#!/usr/bin/env python3
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
LANGS = ("es", "it", "pt", "de")

# Traductions explicites des chaînes les plus visibles et de toutes les zones
# qui avaient encore des valeurs anglaises dans ES/IT/PT/DE.
T = {
"&Fichier": {"es":"&Archivo","it":"&File","pt":"&Ficheiro","de":"&Datei"},
"&Options": {"es":"&Opciones","it":"&Opzioni","pt":"&Opções","de":"&Optionen"},
"&Aide": {"es":"A&yuda","it":"&Aiuto","pt":"A&juda","de":"&Hilfe"},
"&Modifier les paramètres...": {"es":"&Editar configuración...","it":"&Modifica impostazioni...","pt":"&Editar definições...","de":"&Einstellungen bearbeiten..."},
"&Sommaire...": {"es":"&Resumen...","it":"&Riepilogo...","pt":"&Resumo...","de":"&Übersicht..."},
"À &propos": {"es":"&Acerca de","it":"&Informazioni","pt":"&Sobre","de":"&Über"},
"&Quitter": {"es":"&Salir","it":"&Esci","pt":"&Sair","de":"&Beenden"},
"&Enregistrer image ROM...": {"es":"&Guardar imagen ROM...","it":"&Salva immagine ROM...","pt":"&Guardar imagem ROM...","de":"ROM-Abbild &speichern..."},
"Communications :": {"es":"Comunicaciones:","it":"Comunicazioni:","pt":"Comunicações:","de":"Kommunikation:"},
"ID ECU :": {"es":"ID ECU:","it":"ID ECU:","pt":"ID ECU:","de":"ECU-ID:"},
"Erreurs ECU :": {"es":"Errores ECU:","it":"Errori ECU:","pt":"Erros ECU:","de":"ECU-Fehler:"},
"Vue instantanée": {"es":"Captura","it":"Istantanea","pt":"Captura","de":"Momentaufnahme"},
"Voir les captures": {"es":"Ver capturas","it":"Visualizza acquisizioni","pt":"Ver capturas","de":"Aufnahmen anzeigen"},
"Aperçu": {"es":"Resumen","it":"Panoramica","pt":"Visão geral","de":"Übersicht"},
"Réglages": {"es":"Configuración","it":"Impostazioni","pt":"Definições","de":"Einstellungen"},
"Toutes les mesures": {"es":"Todas las mediciones","it":"Tutte le misure","pt":"Todas as medições","de":"Alle Messwerte"},
"Erreurs": {"es":"Errores","it":"Errori","pt":"Erros","de":"Fehler"},
"Actionneurs": {"es":"Actuadores","it":"Attuatori","pt":"Atuadores","de":"Aktoren"},
"Toutes les données": {"es":"Todos los datos","it":"Tutti i dati","pt":"Todos os dados","de":"Alle Daten"},
"Mode interactif": {"es":"Modo interactivo","it":"Modalità interattiva","pt":"Modo interativo","de":"Interaktiver Modus"},
"Analyse": {"es":"Análisis","it":"Analisi","pt":"Análise","de":"Analyse"},
"Diagnostic automatique": {"es":"Diagnóstico automático","it":"Diagnostica automatica","pt":"Diagnóstico automático","de":"Automatische Diagnose"},
"Contact ralenti": {"es":"Interruptor de ralentí","it":"Interruttore minimo","pt":"Interruptor de ralenti","de":"Leerlaufschalter"},
"Boucle fermée": {"es":"Bucle cerrado","it":"Anello chiuso","pt":"Malha fechada","de":"Geschlossener Regelkreis"},
"Boucle ouverte": {"es":"Bucle abierto","it":"Anello aperto","pt":"Malha aberta","de":"Offener Regelkreis"},
"Régime moteur": {"es":"Régimen del motor","it":"Regime motore","pt":"Rotação do motor","de":"Motordrehzahl"},
"Pression collecteur": {"es":"Presión del colector","it":"Pressione collettore","pt":"Pressão do coletor","de":"Saugrohrdruck"},
"Position papillon": {"es":"Posición del acelerador","it":"Posizione farfalla","pt":"Posição da borboleta","de":"Drosselklappenstellung"},
"Régulation d'air du ralenti": {"es":"Control de aire de ralentí","it":"Controllo aria minimo","pt":"Controlo de ar de ralenti","de":"Leerlaufluftregelung"},
"Tension batterie": {"es":"Tensión de batería","it":"Tensione batteria","pt":"Tensão da bateria","de":"Batteriespannung"},
"Température moteur": {"es":"Temperatura del refrigerante","it":"Temperatura liquido refrigerante","pt":"Temperatura do líquido de refrigeração","de":"Kühlmitteltemperatur"},
"Température d'air": {"es":"Temperatura del aire de admisión","it":"Temperatura aria aspirata","pt":"Temperatura do ar de admissão","de":"Ansauglufttemperatur"},
"Tension sonde lambda": {"es":"Tensión de la sonda lambda","it":"Tensione sonda lambda","pt":"Tensão da sonda lambda","de":"Lambdasondenspannung"},
"Correction carburant court terme": {"es":"Corrección de combustible a corto plazo","it":"Correzione carburante a breve termine","pt":"Correção de combustível de curto prazo","de":"Kurzzeit-Kraftstoffkorrektur"},
"Correction carburant long terme": {"es":"Corrección de combustible a largo plazo","it":"Correzione carburante a lungo termine","pt":"Correção de combustível de longo prazo","de":"Langzeit-Kraftstoffkorrektur"},
"Long terme :": {"es":"Largo plazo:","it":"Lungo termine:","pt":"Longo prazo:","de":"Langzeit:"},
"Nom du fichier :": {"es":"Nombre del archivo:","it":"Nome file:","pt":"Nome do ficheiro:","de":"Dateiname:"},
"Enregistrer": {"es":"Grabar","it":"Registra","pt":"Gravar","de":"Aufzeichnen"},
"Arrêter": {"es":"Detener","it":"Arresta","pt":"Parar","de":"Stoppen"},
"  Régime jusqu'à 2000 tr/min pour réglage fin du ralenti": {"es":"  Régimen hasta 2000 rpm para ajuste fino del ralentí","it":"  Regime fino a 2000 rpm per la regolazione fine del minimo","pt":"  Rotação até 2000 rpm para ajuste fino do ralenti","de":"  Drehzahl bis 2000 U/min zur Feinabstimmung des Leerlaufs"},
"Correction carburant": {"es":"Corrección de combustible","it":"Correzione carburante","pt":"Correção de combustível","de":"Kraftstoffkorrektur"},
"Position ralenti chaud": {"es":"Posición de ralentí en caliente","it":"Posizione minimo a caldo","pt":"Posição de ralenti a quente","de":"Warmleerlaufposition"},
"Régime de ralenti": {"es":"Régimen de ralentí","it":"Regime minimo","pt":"Rotação de ralenti","de":"Leerlaufdrehzahl"},
"Avance à l'allumage": {"es":"Avance de encendido","it":"Anticipo accensione","pt":"Avanço da ignição","de":"Zündvorverstellung"},
"Plus pauvre": {"es":"Más pobre","it":"Più magro","pt":"Mais pobre","de":"Magerer"},
"Plus riche": {"es":"Más rico","it":"Più ricco","pt":"Mais rico","de":"Fetter"},
"Moins": {"es":"Menos","it":"Meno","pt":"Menos","de":"Weniger"},
"Plus": {"es":"Más","it":"Più","pt":"Mais","de":"Mehr"},
"Plus lent": {"es":"Más lento","it":"Più lento","pt":"Mais lento","de":"Langsamer"},
"Plus rapide": {"es":"Más rápido","it":"Più veloce","pt":"Mais rápido","de":"Schneller"},
"Avancer (?)": {"es":"Avanzar (?)","it":"Anticipare (?)","pt":"Avançar (?)","de":"Vorstellen (?)"},
"Retarder (?)": {"es":"Retrasar (?)","it":"Ritardare (?)","pt":"Atrasar (?)","de":"Zurückstellen (?)"},
"Réinitialiser les réglages": {"es":"Restablecer configuración","it":"Ripristina impostazioni","pt":"Repor definições","de":"Einstellungen zurücksetzen"},
"Réinitialiser ECU": {"es":"Restablecer ECU","it":"Ripristina ECU","pt":"Repor ECU","de":"ECU zurücksetzen"},
"Paramètre": {"es":"Parámetro","it":"Parametro","pt":"Parâmetro","de":"Parameter"},
"Valeur reçue ECU": {"es":"Valor recibido de ECU","it":"Valore ricevuto ECU","pt":"Valor recebido da ECU","de":"ECU-Empfangswert"},
"Valeur interprétée": {"es":"Valor interpretado","it":"Valore interpretato","pt":"Valor interpretado","de":"Interpretierter Wert"},
"Reçu": {"es":"Recibido","it":"Ricevuto","pt":"Recebido","de":"Empfangen"},
"Calculé": {"es":"Calculado","it":"Calcolato","pt":"Calculado","de":"Berechnet"},
"Description": {"es":"Descripción","it":"Descrizione","pt":"Descrição","de":"Beschreibung"},
"Inconnu": {"es":"Desconocido","it":"Sconosciuto","pt":"Desconhecido","de":"Unbekannt"},
"inconnu": {"es":"desconocido","it":"sconosciuto","pt":"desconhecido","de":"unbekannt"},
"Contact allumage": {"es":"Contacto de encendido","it":"Contatto accensione","pt":"Interruptor da ignição","de":"Zündschalter"},
"Angle papillon": {"es":"Ángulo del acelerador","it":"Angolo farfalla","pt":"Ângulo da borboleta","de":"Drosselklappenwinkel"},
"Rapport air/carburant": {"es":"Relación aire/combustible","it":"Rapporto aria/carburante","pt":"Relação ar/combustível","de":"Luft/Kraftstoff-Verhältnis"},
"Fréquence lambda": {"es":"Frecuencia lambda","it":"Frequenza lambda","pt":"Frequência lambda","de":"Lambdafrequenz"},
"Cycle lambda": {"es":"Ciclo lambda","it":"Ciclo lambda","pt":"Ciclo lambda","de":"Lambda-Tastverhältnis"},
"État lambda": {"es":"Estado lambda","it":"Stato lambda","pt":"Estado lambda","de":"Lambda-Status"},
"Consigne de ralenti": {"es":"Consigna de ralentí","it":"Setpoint minimo","pt":"Referência de ralenti","de":"Leerlauf-Sollwert"},
"Capteur position vilebrequin": {"es":"Sensor de posición del cigüeñal","it":"Sensore posizione albero motore","pt":"Sensor de posição da cambota","de":"Kurbelwellensensor"},
"Capteur potentiomètre papillon": {"es":"Sensor potenciómetro del acelerador","it":"Sensore potenziometro farfalla","pt":"Sensor potenciómetro da borboleta","de":"Drosselklappenpotentiometer"},
"Contact climatisation": {"es":"Interruptor de A/C","it":"Interruttore A/C","pt":"Interruptor A/C","de":"Klimaschalter"},
"Contact point mort": {"es":"Interruptor punto muerto","it":"Interruttore folle","pt":"Interruptor ponto morto","de":"Leerlauf/Neutral-Schalter"},
"Temps bobine": {"es":"Tiempo de carga de bobina","it":"Tempo carica bobina","pt":"Tempo de carga da bobina","de":"Spulenladezeit"},
"Défauts mémorisés :": {"es":"Errores almacenados:","it":"Errori memorizzati:","pt":"Erros memorizados:","de":"Gespeicherte Fehler:"},
"Effacer les codes défaut": {"es":"Borrar códigos de error","it":"Cancella codici guasto","pt":"Limpar códigos de avaria","de":"Fehlercodes löschen"},
"Signal régime et capteur PMH": {"es":"Señal de régimen y sensor de cigüeñal","it":"Segnale regime e sensore albero motore","pt":"Sinal de rotação e sensor da cambota","de":"Drehzahlsignal und Kurbelwellensensor"},
"En direct": {"es":"En directo","it":"In tempo reale","pt":"Em direto","de":"Live"},
"Enreg.": {"es":"Registrado","it":"Registrato","pt":"Registado","de":"Aufgezeichnet"},
"Anomalies :": {"es":"Anomalías:","it":"Anomalie:","pt":"Anomalias:","de":"Anomalien:"},
"Erreur signal lambda": {"es":"Error de señal lambda","it":"Errore segnale lambda","pt":"Erro do sinal lambda","de":"Lambdasignalfehler"},
"Signal lambda anormalement élevé": {"es":"Señal lambda anormalmente alta","it":"Segnale lambda anormalmente alto","pt":"Sinal lambda anormalmente alto","de":"Lambdasignal ungewöhnlich hoch"},
"Signal lambda anormalement bas": {"es":"Señal lambda anormalmente baja","it":"Segnale lambda anormalmente basso","pt":"Sinal lambda anormalmente baixo","de":"Lambdasignal ungewöhnlich niedrig"},
"Position mini du moteur pas-à-pas atteinte": {"es":"Posición mínima del motor paso a paso alcanzada","it":"Posizione minima motore passo-passo raggiunta","pt":"Posição mínima do motor passo-a-passo atingida","de":"Mindestposition des Schrittmotors erreicht"},
"Compteur Jack au maximum (>255 réinitialisations)": {"es":"Contador Jack al máximo (>255 reinicios)","it":"Contatore Jack al massimo (>255 reset)","pt":"Contador Jack no máximo (>255 reposições)","de":"Jack-Zähler am Maximum (>255 Rücksetzungen)"},
"Effacer les anomalies enregistrées": {"es":"Borrar anomalías registradas","it":"Cancella anomalie registrate","pt":"Limpar anomalias registadas","de":"Gespeicherte Anomalien löschen"},
"Tester les actionneurs :": {"es":"Probar actuadores:","it":"Test attuatori:","pt":"Testar atuadores:","de":"Aktoren testen:"},
"Réchauffeur collecteur": {"es":"Calentador del colector","it":"Riscaldatore collettore","pt":"Aquecedor do coletor","de":"Saugrohrheizung"},
"Pompe à carburant": {"es":"Bomba de combustible","it":"Pompa carburante","pt":"Bomba de combustível","de":"Kraftstoffpumpe"},
"Chauffage sonde O2/lambda": {"es":"Calentador sonda O2/lambda","it":"Riscaldatore sonda O2/lambda","pt":"Aquecedor da sonda O2/lambda","de":"O2/Lambda-Sondenheizung"},
"Électrovanne purge canister": {"es":"Válvula de purga del canister","it":"Elettrovalvola spurgo canister","pt":"Válvula de purga do canister","de":"Tankentlüftungsventil"},
"Embrayage climatisation": {"es":"Embrague A/C","it":"Frizione A/C","pt":"Embraiagem A/C","de":"Klimakupplung"},
"Électrovanne suralimentation": {"es":"Válvula de sobrealimentación","it":"Elettrovalvola sovralimentazione","pt":"Válvula de sobrealimentação","de":"Ladedruckventil"},
"Ventilateur 1": {"es":"Ventilador 1","it":"Ventola 1","pt":"Ventoinha 1","de":"Lüfter 1"},
"Ventilateur 2": {"es":"Ventilador 2","it":"Ventola 2","pt":"Ventoinha 2","de":"Lüfter 2"},
"Ventilateur 3": {"es":"Ventilador 3","it":"Ventola 3","pt":"Ventoinha 3","de":"Lüfter 3"},
"Injecteur": {"es":"Inyector","it":"Iniettore","pt":"Injetor","de":"Einspritzventil"},
"Bobine d'allumage": {"es":"Bobina de encendido","it":"Bobina di accensione","pt":"Bobina de ignição","de":"Zündspule"},
"Tester": {"es":"Probar","it":"Test","pt":"Testar","de":"Testen"},
"Activé": {"es":"Activado","it":"Attivato","pt":"Ativado","de":"Aktiviert"},
"Désactivé": {"es":"Desactivado","it":"Disattivato","pt":"Desativado","de":"Deaktiviert"},
"Ouvrir": {"es":"Abrir","it":"Apri","pt":"Abrir","de":"Öffnen"},
"Fermer": {"es":"Cerrar","it":"Chiudi","pt":"Fechar","de":"Schließen"},
"Déclencher": {"es":"Activar","it":"Attiva","pt":"Acionar","de":"Auslösen"},
"Étincelle": {"es":"Chispa","it":"Scintilla","pt":"Faísca","de":"Funke"},
"Réinitialiser tous les actionneurs": {"es":"Restablecer todos los actuadores","it":"Ripristina tutti gli attuatori","pt":"Repor todos os atuadores","de":"Alle Aktoren zurücksetzen"},
"tous les actionneurs désactivés / réinitialisés": {"es":"todos los actuadores desactivados / restablecidos","it":"tutti gli attuatori disattivati / ripristinati","pt":"todos os atuadores desativados / repostos","de":"alle Aktoren deaktiviert / zurückgesetzt"},
"Position moteur pas-à-pas ralenti :": {"es":"Posición del motor paso a paso de ralentí:","it":"Posizione motore passo-passo minimo:","pt":"Posição do motor passo-a-passo de ralenti:","de":"Position Leerlauf-Schrittmotor:"},
"Fermé": {"es":"Cerrado","it":"Chiuso","pt":"Fechado","de":"Geschlossen"},
"Ouvert": {"es":"Abierto","it":"Aperto","pt":"Aberto","de":"Offen"},
"Déplacer le moteur de régulation d'air ralenti": {"es":"Mover motor de control de aire de ralentí","it":"Sposta motore controllo aria minimo","pt":"Mover motor de controlo de ar de ralenti","de":"Leerlaufluft-Stellmotor bewegen"},
"Aucun fichier chargé": {"es":"Ningún archivo cargado","it":"Nessun file caricato","pt":"Nenhum ficheiro carregado","de":"Keine Datei geladen"},
"Tout cocher": {"es":"Seleccionar todo","it":"Seleziona tutto","pt":"Selecionar tudo","de":"Alles auswählen"},
"Tout décocher": {"es":"Deseleccionar todo","it":"Deseleziona tutto","pt":"Limpar seleção","de":"Auswahl aufheben"},
"Superposer toutes\nles courbes sélectionnées": {"es":"Superponer todas\nlas curvas seleccionadas","it":"Sovrapponi tutte\nle curve selezionate","pt":"Sobrepor todas\nas curvas selecionadas","de":"Alle ausgewählten\nKurven überlagern"},
"Voies disponibles :": {"es":"Canales disponibles:","it":"Canali disponibili:","pt":"Canais disponíveis:","de":"Verfügbare Kanäle:"},
"Charger un fichier CSV / TXT...": {"es":"Cargar archivo CSV / TXT...","it":"Carica file CSV / TXT...","pt":"Carregar ficheiro CSV / TXT...","de":"CSV-/TXT-Datei laden..."},
"Ceci ne fonctionne qu'en dehors de mems-rosco": {"es":"Esto solo funciona fuera de mems-rosco.","it":"Funziona solo al di fuori di mems-rosco.","pt":"Isto só funciona fora do mems-rosco.","de":"Dies funktioniert nur außerhalb von mems-rosco."},
"Vous pouvez vérifier cela en tapant \"dir\" ou \".dir\".": {"es":"Puede comprobarlo escribiendo \"dir\" o \".dir\".","it":"Puoi verificarlo digitando \"dir\" o \".dir\".","pt":"Pode verificar escrevendo \"dir\" ou \".dir\".","de":"Sie können dies mit \"dir\" oder \".dir\" prüfen."},
"com est le port où votre interface est connectée !": {"es":"¡com es el puerto al que está conectada la interfaz!","it":"com è la porta a cui è collegata l'interfaccia!","pt":"com é a porta onde a interface está ligada!","de":"com ist der Port, an dem die Schnittstelle angeschlossen ist!"},
"Mode : inconnu": {"es":"Modo: desconocido","it":"Modalità: sconosciuta","pt":"Modo: desconhecido","de":"Modus: unbekannt"},
"D1  • Identifiant étendu": {"es":"D1  • Identificador ampliado","it":"D1  • Identificatore esteso","pt":"D1  • Identificador estendido","de":"D1  • Erweiterte Kennung"},
"D2  • Statut sécurité": {"es":"D2  • Estado de seguridad","it":"D2  • Stato sicurezza","pt":"D2  • Estado de segurança","de":"D2  • Sicherheitsstatus"},
"F0  • Statut session": {"es":"F0  • Estado de sesión","it":"F0  • Stato sessione","pt":"F0  • Estado da sessão","de":"F0  • Sitzungsstatus"},
"F4  → session normale": {"es":"F4  → sesión normal","it":"F4  → sessione normale","pt":"F4  → sessão normal","de":"F4  → normale Sitzung"},
"Les réponses ROSCO apparaîtront ici en hexadécimal.": {"es":"Las respuestas ROSCO aparecerán aquí en hexadecimal.","it":"Le risposte ROSCO appariranno qui in esadecimale.","pt":"As respostas ROSCO aparecerão aqui em hexadecimal.","de":"ROSCO-Antworten werden hier hexadezimal angezeigt."},
"En attente de données ECU": {"es":"Esperando datos de ECU","it":"In attesa dei dati ECU","pt":"A aguardar dados da ECU","de":"Warte auf ECU-Daten"},
"Capturer comme référence": {"es":"Capturar como referencia","it":"Acquisisci come riferimento","pt":"Capturar como referência","de":"Als Referenz erfassen"},
"Effacer référence": {"es":"Borrar referencia","it":"Cancella riferimento","pt":"Limpar referência","de":"Referenz löschen"},
"Diagnostic : --": {"es":"Diagnóstico: --","it":"Diagnostica: --","pt":"Diagnóstico: --","de":"Diagnose: --"},
"Exporter le rapport": {"es":"Exportar informe","it":"Esporta rapporto","pt":"Exportar relatório","de":"Bericht exportieren"},
"Contrôle": {"es":"Comprobación","it":"Controllo","pt":"Verificação","de":"Prüfung"},
"Valeur": {"es":"Valor","it":"Valore","pt":"Valor","de":"Wert"},
"État": {"es":"Estado","it":"Stato","pt":"Estado","de":"Status"},
"Interprétation / action": {"es":"Interpretación / acción","it":"Interpretazione / azione","pt":"Interpretação / ação","de":"Interpretation / Maßnahme"},
"Rapport automatique": {"es":"Informe automático","it":"Rapporto automatico","pt":"Relatório automático","de":"Automatischer Bericht"},
"Anglais": {"es":"Inglés","it":"Inglese","pt":"Inglês","de":"Englisch"},
"Français": {"es":"Francés","it":"Francese","pt":"Francês","de":"Französisch"},
"Langue :": {"es":"Idioma:","it":"Lingua:","pt":"Idioma:","de":"Sprache:"},
"Clair": {"es":"Claro","it":"Chiaro","pt":"Claro","de":"Hell"},
"Sombre": {"es":"Oscuro","it":"Scuro","pt":"Escuro","de":"Dunkel"},
"Thème de l'interface :": {"es":"Tema de la interfaz:","it":"Tema interfaccia:","pt":"Tema da interface:","de":"Oberflächendesign:"},
"Unités de température :": {"es":"Unidades de temperatura:","it":"Unità di temperatura:","pt":"Unidades de temperatura:","de":"Temperatureinheiten:"},
"Nom du périphérique série :": {"es":"Nombre del dispositivo serie:","it":"Nome dispositivo seriale:","pt":"Nome do dispositivo série:","de":"Name des seriellen Geräts:"},
"Créer et maintenir un raccourci sur le Bureau": {"es":"Crear y mantener un acceso directo en el escritorio","it":"Crea e mantieni un collegamento sul Desktop","pt":"Criar e manter um atalho no Ambiente de Trabalho","de":"Desktop-Verknüpfung erstellen und verwalten"},
}

# Chaînes HTML importantes qui ne doivent jamais retomber en anglais.
HTML_T = {
"<html><head/><body><p>Sélectionnez la position voulue par pas de 25% puis activez &quot;Déplacer le moteur de régulation d'air ralenti&quot;</p></body></html>": {
"es":"<html><head/><body><p>Seleccione la posición deseada en pasos del 25% y active &quot;Mover motor de control de aire de ralentí&quot;.</p></body></html>",
"it":"<html><head/><body><p>Seleziona la posizione desiderata a passi del 25% e attiva &quot;Sposta motore controllo aria minimo&quot;.</p></body></html>",
"pt":"<html><head/><body><p>Selecione a posição pretendida em passos de 25% e ative &quot;Mover motor de controlo de ar de ralenti&quot;.</p></body></html>",
"de":"<html><head/><body><p>Gewünschte Position in 25%-Schritten wählen und &quot;Leerlaufluft-Stellmotor bewegen&quot; aktivieren.</p></body></html>"},
"<html><head/><body><p>Ouvrez le mode interactif en tapant &quot;readmems com3 interactive&quot; ou en mode PowerShell &quot;.readmems com3 interactive&quot;.</p></body></html>": {
"es":"<html><head/><body><p>Abra el modo interactivo escribiendo &quot;readmems com3 interactive&quot; o, en PowerShell, &quot;.readmems com3 interactive&quot;.</p></body></html>",
"it":"<html><head/><body><p>Apri la modalità interattiva digitando &quot;readmems com3 interactive&quot; oppure, in PowerShell, &quot;.readmems com3 interactive&quot;.</p></body></html>",
"pt":"<html><head/><body><p>Abra o modo interativo escrevendo &quot;readmems com3 interactive&quot; ou, no PowerShell, &quot;.readmems com3 interactive&quot;.</p></body></html>",
"de":"<html><head/><body><p>Interaktiven Modus mit &quot;readmems com3 interactive&quot; oder in PowerShell mit &quot;.readmems com3 interactive&quot; öffnen.</p></body></html>"},
"<html><head/><body><p>&quot;0xFA&quot; détruit tous vos réglages !</p></body></html>": {
"es":"<html><head/><body><p>&quot;0xFA&quot; destruye todos los ajustes.</p></body></html>",
"it":"<html><head/><body><p>&quot;0xFA&quot; cancella tutte le impostazioni.</p></body></html>",
"pt":"<html><head/><body><p>&quot;0xFA&quot; elimina todas as definições.</p></body></html>",
"de":"<html><head/><body><p>&quot;0xFA&quot; löscht alle Einstellungen.</p></body></html>"},
}

# Remplacements de secours sur les traductions qui sont encore strictement
# identiques à l'anglais. Ils couvrent surtout les bulles d'aide et messages longs.
PHRASES = {
"es": [("engine speed","régimen del motor"),("idle switch","interruptor de ralentí"),("closed loop","bucle cerrado"),("open loop","bucle abierto"),("fuel trim","corrección de combustible"),("coolant temperature","temperatura del refrigerante"),("intake air temperature","temperatura del aire de admisión"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura del combustible"),("battery voltage","tensión de batería"),("lambda sensor","sonda lambda"),("crankshaft position sensor","sensor de posición del cigüeñal"),("stepper motor","motor paso a paso"),("ignition advance","avance de encendido"),("throttle position","posición del acelerador"),("throttle angle","ángulo del acelerador"),("manifold pressure","presión del colector"),("fault codes","códigos de error"),("No file loaded","Ningún archivo cargado"),("Select all","Seleccionar todo"),("Available channels","Canales disponibles"),("Automatic report","Informe automático"),("Waiting for ECU data","Esperando datos de ECU"),("Export report","Exportar informe"),("Unable to","No se puede"),("Select a","Seleccione una"),("Click","Haga clic"),("Value","Valor"),("Status","Estado"),("Received","Recibido"),("Calculated","Calculado"),("Unknown","Desconocido"),("Enabled","Activado"),("Disabled","Desactivado"),("sensor","sensor"),("temperature","temperatura"),("pressure","presión"),("voltage","tensión"),("error","error"),("warning","advertencia"),("report","informe"),("file","archivo")],
"it": [("engine speed","regime motore"),("idle switch","interruttore minimo"),("closed loop","anello chiuso"),("open loop","anello aperto"),("fuel trim","correzione carburante"),("coolant temperature","temperatura liquido refrigerante"),("intake air temperature","temperatura aria aspirata"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura carburante"),("battery voltage","tensione batteria"),("lambda sensor","sonda lambda"),("crankshaft position sensor","sensore posizione albero motore"),("stepper motor","motore passo-passo"),("ignition advance","anticipo accensione"),("throttle position","posizione farfalla"),("throttle angle","angolo farfalla"),("manifold pressure","pressione collettore"),("fault codes","codici guasto"),("No file loaded","Nessun file caricato"),("Select all","Seleziona tutto"),("Available channels","Canali disponibili"),("Automatic report","Rapporto automatico"),("Waiting for ECU data","In attesa dei dati ECU"),("Export report","Esporta rapporto"),("Unable to","Impossibile"),("Select a","Seleziona una"),("Click","Fai clic"),("Value","Valore"),("Status","Stato"),("Received","Ricevuto"),("Calculated","Calcolato"),("Unknown","Sconosciuto"),("Enabled","Attivato"),("Disabled","Disattivato"),("sensor","sensore"),("temperature","temperatura"),("pressure","pressione"),("voltage","tensione"),("error","errore"),("warning","avviso"),("report","rapporto"),("file","file")],
"pt": [("engine speed","rotação do motor"),("idle switch","interruptor de ralenti"),("closed loop","malha fechada"),("open loop","malha aberta"),("fuel trim","correção de combustível"),("coolant temperature","temperatura do líquido de refrigeração"),("intake air temperature","temperatura do ar de admissão"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura do combustível"),("battery voltage","tensão da bateria"),("lambda sensor","sonda lambda"),("crankshaft position sensor","sensor de posição da cambota"),("stepper motor","motor passo-a-passo"),("ignition advance","avanço da ignição"),("throttle position","posição da borboleta"),("throttle angle","ângulo da borboleta"),("manifold pressure","pressão do coletor"),("fault codes","códigos de avaria"),("No file loaded","Nenhum ficheiro carregado"),("Select all","Selecionar tudo"),("Available channels","Canais disponíveis"),("Automatic report","Relatório automático"),("Waiting for ECU data","A aguardar dados da ECU"),("Export report","Exportar relatório"),("Unable to","Não foi possível"),("Select a","Selecione uma"),("Click","Clique"),("Value","Valor"),("Status","Estado"),("Received","Recebido"),("Calculated","Calculado"),("Unknown","Desconhecido"),("Enabled","Ativado"),("Disabled","Desativado"),("sensor","sensor"),("temperature","temperatura"),("pressure","pressão"),("voltage","tensão"),("error","erro"),("warning","aviso"),("report","relatório"),("file","ficheiro")],
"de": [("engine speed","Motordrehzahl"),("idle switch","Leerlaufschalter"),("closed loop","geschlossener Regelkreis"),("open loop","offener Regelkreis"),("fuel trim","Kraftstoffkorrektur"),("coolant temperature","Kühlmitteltemperatur"),("intake air temperature","Ansauglufttemperatur"),("ambient temperature","Umgebungstemperatur"),("fuel temperature","Kraftstofftemperatur"),("battery voltage","Batteriespannung"),("lambda sensor","Lambdasonde"),("crankshaft position sensor","Kurbelwellensensor"),("stepper motor","Schrittmotor"),("ignition advance","Zündvorverstellung"),("throttle position","Drosselklappenstellung"),("throttle angle","Drosselklappenwinkel"),("manifold pressure","Saugrohrdruck"),("fault codes","Fehlercodes"),("No file loaded","Keine Datei geladen"),("Select all","Alles auswählen"),("Available channels","Verfügbare Kanäle"),("Automatic report","Automatischer Bericht"),("Waiting for ECU data","Warte auf ECU-Daten"),("Export report","Bericht exportieren"),("Unable to","Nicht möglich:"),("Select a","Wählen Sie eine"),("Click","Klicken Sie"),("Value","Wert"),("Status","Status"),("Received","Empfangen"),("Calculated","Berechnet"),("Unknown","Unbekannt"),("Enabled","Aktiviert"),("Disabled","Deaktiviert"),("sensor","Sensor"),("temperature","Temperatur"),("pressure","Druck"),("voltage","Spannung"),("error","Fehler"),("warning","Warnung"),("report","Bericht"),("file","Datei")],
}

EN_FINGERPRINT = re.compile(r"\b(the|this|that|with|without|from|into|only|before|after|select|check|value|status|sensor|engine|fuel|file|report|warning|error|unknown|received|calculated|enabled|disabled|open|closed|reset|capture|available|stored|maximum|minimum)\b", re.I)

def text_of(el):
    return "" if el is None else "".join(el.itertext())

def set_translation(el, value):
    # Les catalogues n'utilisent pas de formes plurielles ici.
    el.clear()
    el.text = value

def fallback_translate(text, lang):
    out = text
    for a, b in sorted(PHRASES[lang], key=lambda x: len(x[0]), reverse=True):
        out = re.sub(re.escape(a), b, out, flags=re.I)
    return out

def load_english():
    tree = ET.parse(ROOT / "ECUMemsManager_en.ts")
    d = {}
    for ctx in tree.getroot().findall("context"):
        name = text_of(ctx.find("name"))
        for msg in ctx.findall("message"):
            src = text_of(msg.find("source"))
            tr = text_of(msg.find("translation"))
            d[(name, src)] = tr
    return d

english = load_english()
for lang in LANGS:
    path = ROOT / f"ECUMemsManager_{lang}.ts"
    tree = ET.parse(path)
    changed = 0
    for ctx in tree.getroot().findall("context"):
        name = text_of(ctx.find("name"))
        for msg in ctx.findall("message"):
            src_el = msg.find("source")
            tr_el = msg.find("translation")
            if src_el is None or tr_el is None:
                continue
            src = text_of(src_el)
            old = text_of(tr_el)
            new = None
            if src in T:
                new = T[src][lang]
            elif src in HTML_T:
                new = HTML_T[src][lang]
            else:
                en = english.get((name, src), "")
                # Ne corriger automatiquement que les entrées manifestement laissées en anglais.
                if en and old.strip() == en.strip() and EN_FINGERPRINT.search(en):
                    candidate = fallback_translate(en, lang)
                    if candidate != en:
                        new = candidate
            if new is not None and new != old:
                set_translation(tr_el, new)
                changed += 1
    ET.indent(tree, space=" ")
    tree.write(path, encoding="utf-8", xml_declaration=True)
    print(f"{lang}: {changed} translation(s) corrected")
