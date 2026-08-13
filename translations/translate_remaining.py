#!/usr/bin/env python3
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
LANGS = ("es", "it", "pt", "de")

# Deuxieme passe : corrige les traductions encore anglaises ou mixtes.
# La source francaise reste la reference unique.
EXACT = {
    "Aucun fichier chargé": {"es":"Ningún archivo cargado","it":"Nessun file caricato","pt":"Nenhum ficheiro carregado","de":"Keine Datei geladen"},
    "Aucune donnée exploitable dans ce fichier.": {"es":"No hay datos utilizables en este archivo.","it":"Nessun dato utilizzabile in questo file.","pt":"Não existem dados utilizáveis neste ficheiro.","de":"In dieser Datei sind keine verwertbaren Daten vorhanden."},
    "Cochez une ou plusieurs voies dans la liste à gauche pour les afficher": {"es":"Seleccione uno o varios canales de la lista de la izquierda para mostrarlos","it":"Seleziona uno o più canali nell'elenco a sinistra per visualizzarli","pt":"Selecione um ou mais canais na lista à esquerda para os apresentar","de":"Wählen Sie links einen oder mehrere Kanäle aus, um sie anzuzeigen"},
    "Fichier CSV vide ou incomplet.": {"es":"El archivo CSV está vacío o incompleto.","it":"Il file CSV è vuoto o incompleto.","pt":"O ficheiro CSV está vazio ou incompleto.","de":"Die CSV-Datei ist leer oder unvollständig."},
    "Format de fichier CSV non reconnu.": {"es":"Formato de archivo CSV no reconocido.","it":"Formato file CSV non riconosciuto.","pt":"Formato de ficheiro CSV não reconhecido.","de":"CSV-Dateiformat nicht erkannt."},
    "Impossible d'ouvrir le fichier :\n%1": {"es":"No se puede abrir el archivo:\n%1","it":"Impossibile aprire il file:\n%1","pt":"Não foi possível abrir o ficheiro:\n%1","de":"Datei kann nicht geöffnet werden:\n%1"},
    "Revenir à l'affichage empilé": {"es":"Volver a la vista apilada","it":"Torna alla visualizzazione impilata","pt":"Voltar à vista empilhada","de":"Zur gestapelten Ansicht zurückkehren"},
    "Superposer toutes\nles courbes sélectionnées": {"es":"Superponer todas\nlas curvas seleccionadas","it":"Sovrapponi tutte\nle curve selezionate","pt":"Sobrepor todas\nas curvas selecionadas","de":"Alle ausgewählten\nKurven überlagern"},
    "Superposer toutes les courbes sélectionnées": {"es":"Superponer todas las curvas seleccionadas","it":"Sovrapponi tutte le curve selezionate","pt":"Sobrepor todas as curvas selecionadas","de":"Alle ausgewählten Kurven überlagern"},
    "Tout décocher": {"es":"Deseleccionar todo","it":"Deseleziona tutto","pt":"Desmarcar tudo","de":"Alles abwählen"},
    "Tout cocher": {"es":"Seleccionar todo","it":"Seleziona tutto","pt":"Selecionar tudo","de":"Alles auswählen"},
    "Voies disponibles :": {"es":"Canales disponibles:","it":"Canali disponibili:","pt":"Canais disponíveis:","de":"Verfügbare Kanäle:"},
    "Charger un fichier CSV / TXT...": {"es":"Cargar archivo CSV / TXT...","it":"Carica file CSV / TXT...","pt":"Carregar ficheiro CSV / TXT...","de":"CSV-/TXT-Datei laden..."},
    "Charger un fichier journal": {"es":"Cargar archivo de registro","it":"Carica file di registro","pt":"Carregar ficheiro de registo","de":"Protokolldatei laden"},
    "Fichiers journal (*.csv *.txt);;Fichiers CSV (*.csv);;Fichiers texte (*.txt);;Tous les fichiers (*.*)": {"es":"Archivos de registro (*.csv *.txt);;Archivos CSV (*.csv);;Archivos de texto (*.txt);;Todos los archivos (*.*)","it":"File di registro (*.csv *.txt);;File CSV (*.csv);;File di testo (*.txt);;Tutti i file (*.*)","pt":"Ficheiros de registo (*.csv *.txt);;Ficheiros CSV (*.csv);;Ficheiros de texto (*.txt);;Todos os ficheiros (*.*)","de":"Protokolldateien (*.csv *.txt);;CSV-Dateien (*.csv);;Textdateien (*.txt);;Alle Dateien (*.*)"},
    "Aucune capture enregistrée.": {"es":"No hay capturas guardadas.","it":"Nessuna acquisizione salvata.","pt":"Não existem capturas guardadas.","de":"Keine Aufnahmen gespeichert."},
    "Impossible de lire cette capture.": {"es":"No se puede leer esta captura.","it":"Impossibile leggere questa acquisizione.","pt":"Não foi possível ler esta captura.","de":"Diese Aufnahme kann nicht gelesen werden."},
    "Impossible de supprimer cette capture.": {"es":"No se puede eliminar esta captura.","it":"Impossibile eliminare questa acquisizione.","pt":"Não foi possível eliminar esta captura.","de":"Diese Aufnahme kann nicht gelöscht werden."},
    "Ouvrir / Agrandir": {"es":"Abrir / Ampliar","it":"Apri / Ingrandisci","pt":"Abrir / Ampliar","de":"Öffnen / Vergrößern"},
    "Supprimer définitivement la capture sélectionnée ?": {"es":"¿Eliminar definitivamente la captura seleccionada?","it":"Eliminare definitivamente l'acquisizione selezionata?","pt":"Eliminar definitivamente a captura selecionada?","de":"Ausgewählte Aufnahme endgültig löschen?"},
    "Supprimer la capture": {"es":"Eliminar captura","it":"Elimina acquisizione","pt":"Eliminar captura","de":"Aufnahme löschen"},
    "Sélectionnez une capture.": {"es":"Seleccione una captura.","it":"Seleziona un'acquisizione.","pt":"Selecione uma captura.","de":"Wählen Sie eine Aufnahme aus."},
    "Voir les captures": {"es":"Ver capturas","it":"Visualizza acquisizioni","pt":"Ver capturas","de":"Aufnahmen anzeigen"},
    "Les états et conseils de ce rapport sont des contrôles de cohérence et ne remplacent pas les spécifications constructeur.": {"es":"Los estados y consejos de este informe son controles de coherencia y no sustituyen las especificaciones del fabricante.","it":"Gli stati e i consigli di questo rapporto sono controlli di coerenza e non sostituiscono le specifiche del costruttore.","pt":"Os estados e recomendações deste relatório são verificações de coerência e não substituem as especificações do fabricante.","de":"Die Zustände und Hinweise in diesem Bericht sind Plausibilitätsprüfungen und ersetzen keine Herstellervorgaben."},
    "Aucun bit défaut actif dans les trames 0x7D/0x80.": {"es":"No hay ningún bit de fallo activo en las tramas 0x7D/0x80.","it":"Nessun bit di guasto attivo nei frame 0x7D/0x80.","pt":"Não existe qualquer bit de avaria ativo nas tramas 0x7D/0x80.","de":"In den 0x7D/0x80-Frames ist kein Fehlerbit aktiv."},
    "Aucun défaut TPS connu actif dans les bits surveillés.": {"es":"No hay ningún fallo TPS conocido activo en los bits supervisados.","it":"Nessun guasto TPS noto attivo nei bit monitorati.","pt":"Não existe qualquer avaria TPS conhecida ativa nos bits monitorizados.","de":"In den überwachten Bits ist kein bekannter TPS-Fehler aktiv."},
    "Aucune combinaison critique IAC/erreur détectée.": {"es":"No se ha detectado ninguna combinación crítica IAC/error.","it":"Nessuna combinazione critica IAC/errore rilevata.","pt":"Não foi detetada qualquer combinação crítica IAC/erro.","de":"Keine kritische IAC/Fehler-Kombination erkannt."},
    "Aucune donnée ECU disponible.": {"es":"No hay datos de ECU disponibles.","it":"Nessun dato ECU disponibile.","pt":"Não existem dados da ECU disponíveis.","de":"Keine ECU-Daten verfügbar."},
    "Aucune donnée à exporter.": {"es":"No hay datos para exportar.","it":"Nessun dato da esportare.","pt":"Não existem dados para exportar.","de":"Keine Daten zum Exportieren."},
    "Boucle fermée active : les corrections lambda sont en cours d'utilisation.": {"es":"Bucle cerrado activo: se están aplicando las correcciones lambda.","it":"Anello chiuso attivo: le correzioni lambda sono applicate.","pt":"Malha fechada ativa: as correções lambda estão a ser aplicadas.","de":"Geschlossener Regelkreis aktiv: Lambda-Korrekturen werden angewendet."},
    "Boucle fermée inactive à cet instant ; interpréter avec température et conditions moteur.": {"es":"El bucle cerrado está inactivo en este momento; interpretar junto con la temperatura y las condiciones del motor.","it":"L'anello chiuso è attualmente inattivo; interpretare insieme alla temperatura e alle condizioni del motore.","pt":"A malha fechada está atualmente inativa; interpretar em conjunto com a temperatura e as condições do motor.","de":"Der geschlossene Regelkreis ist derzeit inaktiv; zusammen mit Temperatur und Motorbedingungen interpretieren."},
    "Capturer comme référence": {"es":"Capturar como referencia","it":"Acquisisci come riferimento","pt":"Capturar como referência","de":"Als Referenz erfassen"},
    "Commande de ralenti IAC": {"es":"Control de ralentí IAC","it":"Comando minimo IAC","pt":"Comando de ralenti IAC","de":"IAC-Leerlaufsteuerung"},
    "Comparaison référence": {"es":"Comparación con referencia","it":"Confronto con riferimento","pt":"Comparação com referência","de":"Referenzvergleich"},
    "Contrôle": {"es":"Comprobación","it":"Controllo","pt":"Verificação","de":"Prüfung"},
    "Corrections carburant": {"es":"Correcciones de combustible","it":"Correzioni carburante","pt":"Correções de combustível","de":"Kraftstoffkorrekturen"},
    "Défauts ECU": {"es":"Fallos ECU","it":"Guasti ECU","pt":"Avarias ECU","de":"ECU-Fehler"},
    "Effacer référence": {"es":"Borrar referencia","it":"Cancella riferimento","pt":"Limpar referência","de":"Referenz löschen"},
    "En attente de données ECU": {"es":"Esperando datos de ECU","it":"In attesa dei dati ECU","pt":"A aguardar dados da ECU","de":"Warte auf ECU-Daten"},
    "Erreur ralenti à chaud": {"es":"Error de ralentí en caliente","it":"Errore minimo a caldo","pt":"Erro de ralenti a quente","de":"Warmleerlauffehler"},
    "Exporter le rapport": {"es":"Exportar informe","it":"Esporta rapporto","pt":"Exportar relatório","de":"Bericht exportieren"},
    "Interprétation / action": {"es":"Interpretación / acción","it":"Interpretazione / azione","pt":"Interpretação / ação","de":"Interpretation / Maßnahme"},
    "Rapport automatique": {"es":"Informe automático","it":"Rapporto automatico","pt":"Relatório automático","de":"Automatischer Bericht"},
    "Rapport texte (*.txt)": {"es":"Informe de texto (*.txt)","it":"Rapporto di testo (*.txt)","pt":"Relatório de texto (*.txt)","de":"Textbericht (*.txt)"},
    "ANOMALIE": {"es":"ANOMALÍA","it":"ANOMALIA","pt":"ANOMALIA","de":"ANOMALIE"},
    "ANOMALIE À INVESTIGUER": {"es":"ANOMALÍA A INVESTIGAR","it":"ANOMALIA DA VERIFICARE","pt":"ANOMALIA A INVESTIGAR","de":"ZU PRÜFENDE ANOMALIE"},
    "NON ÉVALUÉ": {"es":"NO EVALUADO","it":"NON VALUTATO","pt":"NÃO AVALIADO","de":"NICHT BEWERTET"},
    "SURVEILLER": {"es":"VIGILAR","it":"MONITORARE","pt":"MONITORIZAR","de":"BEOBACHTEN"},
    "INFORMATION": {"es":"INFORMACIÓN","it":"INFORMAZIONE","pt":"INFORMAÇÃO","de":"INFORMATION"},
    "Temps bobine": {"es":"Tiempo de carga de bobina","it":"Tempo di carica bobina","pt":"Tempo de carga da bobina","de":"Spulenladezeit"},
    "Inconnu": {"es":"Desconocido","it":"Sconosciuto","pt":"Desconhecido","de":"Unbekannt"},
    "Contact allumage": {"es":"Contacto de encendido","it":"Contatto accensione","pt":"Interruptor da ignição","de":"Zündschalter"},
    "Angle papillon": {"es":"Ángulo del acelerador","it":"Angolo farfalla","pt":"Ângulo da borboleta","de":"Drosselklappenwinkel"},
    "Rapport air/carburant": {"es":"Relación aire/combustible","it":"Rapporto aria/carburante","pt":"Relação ar/combustível","de":"Luft/Kraftstoff-Verhältnis"},
    "Fréquence lambda": {"es":"Frecuencia lambda","it":"Frequenza lambda","pt":"Frequência lambda","de":"Lambdafrequenz"},
    "Cycle lambda": {"es":"Ciclo lambda","it":"Ciclo lambda","pt":"Ciclo lambda","de":"Lambda-Tastverhältnis"},
    "État lambda": {"es":"Estado lambda","it":"Stato lambda","pt":"Estado lambda","de":"Lambda-Status"},
    "Position moteur pas-à-pas (%)": {"es":"Posición del motor paso a paso (%)","it":"Posizione motore passo-passo (%)","pt":"Posição do motor passo-a-passo (%)","de":"Schrittmotorposition (%)"},
    "Erreur ralenti": {"es":"Error de ralentí","it":"Errore minimo","pt":"Erro de ralenti","de":"Leerlauffehler"},
    "Décalage avance allumage": {"es":"Desplazamiento del avance de encendido","it":"Offset anticipo accensione","pt":"Desvio do avanço da ignição","de":"Zündvorverstellungs-Offset"},
    "Position de base ralenti": {"es":"Posición base de ralentí","it":"Posizione base minimo","pt":"Posição base de ralenti","de":"Leerlauf-Grundposition"},
    "Ralenti chaud": {"es":"Ralentí en caliente","it":"Minimo a caldo","pt":"Ralenti a quente","de":"Warmleerlauf"},
    "Tester les actionneurs :": {"es":"Probar actuadores:","it":"Test attuatori:","pt":"Testar atuadores:","de":"Aktoren testen:"},
    "Réchauffeur collecteur": {"es":"Calentador del colector","it":"Riscaldatore collettore","pt":"Aquecedor do coletor","de":"Saugrohrheizung"},
    "Pompe à carburant": {"es":"Bomba de combustible","it":"Pompa carburante","pt":"Bomba de combustível","de":"Kraftstoffpumpe"},
    "Chauffage sonde O2/lambda": {"es":"Calentador sonda O2/lambda","it":"Riscaldatore sonda O2/lambda","pt":"Aquecedor da sonda O2/lambda","de":"O2/Lambda-Sondenheizung"},
    "Électrovanne purge canister": {"es":"Válvula de purga del canister","it":"Elettrovalvola spurgo canister","pt":"Válvula de purga do canister","de":"Tankentlüftungsventil"},
    "Embrayage climatisation": {"es":"Embrague A/C","it":"Frizione A/C","pt":"Embraiagem A/C","de":"Klimakupplung"},
    "Électrovanne suralimentation": {"es":"Válvula de sobrealimentación","it":"Elettrovalvola sovralimentazione","pt":"Válvula de sobrealimentação","de":"Ladedruckventil"},
    "Injecteur": {"es":"Inyector","it":"Iniettore","pt":"Injetor","de":"Einspritzventil"},
    "Bobine d'allumage": {"es":"Bobina de encendido","it":"Bobina di accensione","pt":"Bobina de ignição","de":"Zündspule"},
    "Déclencher": {"es":"Activar","it":"Attiva","pt":"Acionar","de":"Auslösen"},
    "Étincelle": {"es":"Chispa","it":"Scintilla","pt":"Faísca","de":"Funke"},
    "Réinitialiser tous les actionneurs": {"es":"Restablecer todos los actuadores","it":"Ripristina tutti gli attuatori","pt":"Repor todos os atuadores","de":"Alle Aktoren zurücksetzen"},
    "tous les actionneurs désactivés / réinitialisés": {"es":"todos los actuadores desactivados / restablecidos","it":"tutti gli attuatori disattivati / ripristinati","pt":"todos os atuadores desativados / repostos","de":"alle Aktoren deaktiviert / zurückgesetzt"},
    "Ceci ne fonctionne qu'en dehors de mems-rosco": {"es":"Esto solo funciona fuera de mems-rosco.","it":"Questo funziona solo fuori da mems-rosco.","pt":"Isto só funciona fora do mems-rosco.","de":"Dies funktioniert nur außerhalb von mems-rosco."},
    "Vous pouvez vérifier cela en tapant \"dir\" ou \".dir\".": {"es":"Puede comprobarlo escribiendo \"dir\" o \".dir\".","it":"Puoi verificarlo digitando \"dir\" o \".dir\".","pt":"Pode verificar escrevendo \"dir\" ou \".dir\".","de":"Sie können dies mit \"dir\" oder \".dir\" prüfen."},
    "com est le port où votre interface est connectée !": {"es":"¡com es el puerto al que está conectada la interfaz!","it":"com è la porta a cui è collegata l'interfaccia!","pt":"com é a porta onde a interface está ligada!","de":"com ist der Port, an dem die Schnittstelle angeschlossen ist!"},
    "D0  • Identification": {"es":"D0  • Identificación","it":"D0  • Identificazione","pt":"D0  • Identificação","de":"D0  • Identifikation"},
    "D1  • Identifiant étendu": {"es":"D1  • Identificador ampliado","it":"D1  • Identificatore esteso","pt":"D1  • Identificador estendido","de":"D1  • Erweiterte Kennung"},
    "D2  • Statut sécurité": {"es":"D2  • Estado de seguridad","it":"D2  • Stato sicurezza","pt":"D2  • Estado de segurança","de":"D2  • Sicherheitsstatus"},
    "F0  • État session": {"es":"F0  • Estado de sesión","it":"F0  • Stato sessione","pt":"F0  • Estado da sessão","de":"F0  • Sitzungsstatus"},
    "F4  → session normale": {"es":"F4  → sesión normal","it":"F4  → sessione normale","pt":"F4  → sessão normal","de":"F4  → normale Sitzung"},
    "Les réponses ROSCO apparaîtront ici en hexadécimal.": {"es":"Las respuestas ROSCO aparecerán aquí en hexadecimal.","it":"Le risposte ROSCO appariranno qui in esadecimale.","pt":"As respostas ROSCO aparecerão aqui em hexadecimal.","de":"ROSCO-Antworten werden hier hexadezimal angezeigt."},
    "Changement de mode : la commande est envoyée uniquement sur demande. D3 (recodage) reste volontairement bloquée.": {"es":"Cambio de modo: el comando se envía únicamente bajo petición. D3 (recodificación) permanece bloqueado intencionadamente.","it":"Cambio modalità: il comando viene inviato solo su richiesta. D3 (ricodifica) rimane intenzionalmente bloccato.","pt":"Mudança de modo: o comando é enviado apenas mediante pedido. D3 (recodificação) permanece intencionalmente bloqueado.","de":"Moduswechsel: Der Befehl wird nur auf Anforderung gesendet. D3 (Umcodierung) bleibt absichtlich gesperrt."},
}

# Traductions exactes de quelques défauts/états visibles encore en anglais.
EN_EXACT = {
    "Coil time (ms)": {"es":"Tiempo de carga de bobina (ms)","it":"Tempo di carica bobina (ms)","pt":"Tempo de carga da bobina (ms)","de":"Spulenladezeit (ms)"},
    "Ambient temperature (°C)": {"es":"Temperatura ambiente (°C)","it":"Temperatura ambiente (°C)","pt":"Temperatura ambiente (°C)","de":"Umgebungstemperatur (°C)"},
    "Fuel temperature (°C)": {"es":"Temperatura del combustible (°C)","it":"Temperatura carburante (°C)","pt":"Temperatura do combustível (°C)","de":"Kraftstofftemperatur (°C)"},
    "Throttle angle (°)": {"es":"Ángulo del acelerador (°)","it":"Angolo farfalla (°)","pt":"Ângulo da borboleta (°)","de":"Drosselklappenwinkel (°)"},
    "Air/fuel ratio": {"es":"Relación aire/combustible","it":"Rapporto aria/carburante","pt":"Relação ar/combustível","de":"Luft/Kraftstoff-Verhältnis"},
    "Ignition switch": {"es":"Contacto de encendido","it":"Contatto accensione","pt":"Interruptor da ignição","de":"Zündschalter"},
    "Idle switch": {"es":"Interruptor de ralentí","it":"Interruttore minimo","pt":"Interruptor de ralenti","de":"Leerlaufschalter"},
    "Closed loop": {"es":"Bucle cerrado","it":"Anello chiuso","pt":"Malha fechada","de":"Geschlossener Regelkreis"},
    "Long-term fuel trim (%)": {"es":"Corrección de combustible a largo plazo (%)","it":"Correzione carburante a lungo termine (%)","pt":"Correção de combustível a longo prazo (%)","de":"Langzeit-Kraftstoffkorrektur (%)"},
    "Short-term fuel trim (%)": {"es":"Corrección de combustible a corto plazo (%)","it":"Correzione carburante a breve termine (%)","pt":"Correção de combustível a curto prazo (%)","de":"Kurzzeit-Kraftstoffkorrektur (%)"},
    "Canister purge duty cycle (%)": {"es":"Ciclo de purga del canister (%)","it":"Ciclo spurgo canister (%)","pt":"Ciclo de purga do canister (%)","de":"Tankentlüftungs-Tastverhältnis (%)"},
    "Stepper motor position (%)": {"es":"Posición del motor paso a paso (%)","it":"Posizione motore passo-passo (%)","pt":"Posição do motor passo-a-passo (%)","de":"Schrittmotorposition (%)"},
    "Idle error": {"es":"Error de ralentí","it":"Errore minimo","pt":"Erro de ralenti","de":"Leerlauffehler"},
    "Ignition advance offset": {"es":"Desplazamiento del avance de encendido","it":"Offset anticipo accensione","pt":"Desvio do avanço da ignição","de":"Zündvorverstellungs-Offset"},
    "Idle base position": {"es":"Posición base de ralentí","it":"Posizione base minimo","pt":"Posição base de ralenti","de":"Leerlauf-Grundposition"},
    "Hot idle": {"es":"Ralentí en caliente","it":"Minimo a caldo","pt":"Ralenti a quente","de":"Warmleerlauf"},
    "Manifold heater": {"es":"Calentador del colector","it":"Riscaldatore collettore","pt":"Aquecedor do coletor","de":"Saugrohrheizung"},
    "Canister purge valve": {"es":"Válvula de purga del canister","it":"Valvola spurgo canister","pt":"Válvula de purga do canister","de":"Tankentlüftungsventil"},
    "A/C clutch": {"es":"Embrague A/C","it":"Frizione A/C","pt":"Embraiagem A/C","de":"Klimakupplung"},
    "Boost control valve": {"es":"Válvula de control de sobrealimentación","it":"Valvola controllo sovralimentazione","pt":"Válvula de controlo de sobrealimentação","de":"Ladedruckregelventil"},
    "High boost pressure": {"es":"Presión de sobrealimentación alta","it":"Pressione di sovralimentazione elevata","pt":"Pressão de sobrealimentação elevada","de":"Hoher Ladedruck"},
    "Temperature gauge": {"es":"Indicador de temperatura","it":"Indicatore temperatura","pt":"Indicador de temperatura","de":"Temperaturanzeige"},
    "Fuel pump circuit": {"es":"Circuito de la bomba de combustible","it":"Circuito pompa carburante","pt":"Circuito da bomba de combustível","de":"Kraftstoffpumpenkreis"},
    "A/C clutch control": {"es":"Control del embrague A/C","it":"Comando frizione A/C","pt":"Controlo da embraiagem A/C","de":"Klimakupplungssteuerung"},
    "Purge valve": {"es":"Válvula de purga","it":"Valvola di spurgo","pt":"Válvula de purga","de":"Entlüftungsventil"},
    "MAP sensor": {"es":"Sensor MAP","it":"Sensore MAP","pt":"Sensor MAP","de":"MAP-Sensor"},
    "Throttle position sensor circuit": {"es":"Circuito del sensor de posición del acelerador","it":"Circuito sensore posizione farfalla","pt":"Circuito do sensor de posição da borboleta","de":"Drosselklappensensor-Kreis"},
    "Lambda sensor heater supply voltage": {"es":"Tensión de alimentación del calentador de la sonda lambda","it":"Tensione alimentazione riscaldatore sonda lambda","pt":"Tensão de alimentação do aquecedor da sonda lambda","de":"Versorgungsspannung der Lambdasondenheizung"},
    "Crankshaft synchronization": {"es":"Sincronización del cigüeñal","it":"Sincronizzazione albero motore","pt":"Sincronização da cambota","de":"Kurbelwellensynchronisation"},
    "Fan 1 control": {"es":"Control del ventilador 1","it":"Comando ventola 1","pt":"Controlo da ventoinha 1","de":"Lüfter-1-Steuerung"},
    "Fan 2 control": {"es":"Control del ventilador 2","it":"Comando ventola 2","pt":"Controlo da ventoinha 2","de":"Lüfter-2-Steuerung"},
    "Immobilizer command": {"es":"Comando del inmovilizador","it":"Comando immobilizer","pt":"Comando do imobilizador","de":"Wegfahrsperren-Befehl"},
    "Engine-speed signal and crankshaft sensor": {"es":"Señal de régimen y sensor de cigüeñal","it":"Segnale regime e sensore albero motore","pt":"Sinal de rotação e sensor da cambota","de":"Drehzahlsignal und Kurbelwellensensor"},
    "Lambda signal error": {"es":"Error de señal lambda","it":"Errore segnale lambda","pt":"Erro do sinal lambda","de":"Lambdasignalfehler"},
    "Lambda signal abnormally high": {"es":"Señal lambda anormalmente alta","it":"Segnale lambda anormalmente alto","pt":"Sinal lambda anormalmente alto","de":"Lambdasignal ungewöhnlich hoch"},
    "Lambda signal abnormally low": {"es":"Señal lambda anormalmente baja","it":"Segnale lambda anormalmente basso","pt":"Sinal lambda anormalmente baixo","de":"Lambdasignal ungewöhnlich niedrig"},
    "Stepper motor minimum position reached": {"es":"Posición mínima del motor paso a paso alcanzada","it":"Posizione minima motore passo-passo raggiunta","pt":"Posição mínima do motor passo-a-passo atingida","de":"Mindestposition des Schrittmotors erreicht"},
    "Jack counter at maximum (>255 resets)": {"es":"Contador Jack al máximo (>255 reinicios)","it":"Contatore Jack al massimo (>255 reset)","pt":"Contador Jack no máximo (>255 reposições)","de":"Jack-Zähler am Maximum (>255 Rücksetzungen)"},
}

# Remplacements de secours pour les bulles d'aide et phrases longues.
# Ils ne sont appliques que si la traduction contient encore des mots anglais.
PHRASES = {
"es": [("engine speed","régimen del motor"),("coolant temperature","temperatura del refrigerante"),("intake air temperature","temperatura del aire de admisión"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura del combustible"),("manifold pressure","presión del colector"),("battery voltage","tensión de batería"),("throttle position","posición del acelerador"),("throttle angle","ángulo del acelerador"),("idle switch","interruptor de ralentí"),("closed loop","bucle cerrado"),("open loop","bucle abierto"),("lambda sensor","sonda lambda"),("fuel trim","corrección de combustible"),("ignition advance","avance de encendido"),("stepper motor","motor paso a paso"),("crankshaft sensor","sensor de cigüeñal"),("select the required position","seleccione la posición requerida"),("move idle air control motor","mover motor de control de aire de ralentí"),("only works outside","solo funciona fuera de"),("proceed at your own risk","proceda bajo su propia responsabilidad"),("only do this if you know","haga esto solo si conoce"),("No file loaded","Ningún archivo cargado"),("Select all","Seleccionar todo"),("Clear all","Deseleccionar todo"),("Available channels","Canales disponibles"),("Waiting for ECU data","Esperando datos de ECU"),("Capture as reference","Capturar como referencia"),("Clear reference","Borrar referencia"),("Export report","Exportar informe"),("Automatic report","Informe automático"),("Status","Estado"),("Check","Comprobación"),("Value","Valor"),("Received","Recibido"),("Calculated","Calculado"),("Unknown","Desconocido"),("Enabled","Activado"),("Disabled","Desactivado"),("Open","Abrir"),("Close","Cerrar"),("sensor","sensor"),("temperature","temperatura"),("pressure","presión"),("voltage","tensión"),("fault","fallo"),("error","error"),("warning","advertencia"),("file","archivo"),("report","informe")],
"it": [("engine speed","regime motore"),("coolant temperature","temperatura liquido refrigerante"),("intake air temperature","temperatura aria aspirata"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura carburante"),("manifold pressure","pressione collettore"),("battery voltage","tensione batteria"),("throttle position","posizione farfalla"),("throttle angle","angolo farfalla"),("idle switch","interruttore minimo"),("closed loop","anello chiuso"),("open loop","anello aperto"),("lambda sensor","sonda lambda"),("fuel trim","correzione carburante"),("ignition advance","anticipo accensione"),("stepper motor","motore passo-passo"),("crankshaft sensor","sensore albero motore"),("select the required position","seleziona la posizione richiesta"),("move idle air control motor","sposta motore controllo aria minimo"),("only works outside","funziona solo fuori da"),("proceed at your own risk","procedi a tuo rischio"),("only do this if you know","fallo solo se conosci"),("No file loaded","Nessun file caricato"),("Select all","Seleziona tutto"),("Clear all","Deseleziona tutto"),("Available channels","Canali disponibili"),("Waiting for ECU data","In attesa dei dati ECU"),("Capture as reference","Acquisisci come riferimento"),("Clear reference","Cancella riferimento"),("Export report","Esporta rapporto"),("Automatic report","Rapporto automatico"),("Status","Stato"),("Check","Controllo"),("Value","Valore"),("Received","Ricevuto"),("Calculated","Calcolato"),("Unknown","Sconosciuto"),("Enabled","Attivato"),("Disabled","Disattivato"),("Open","Apri"),("Close","Chiudi"),("sensor","sensore"),("temperature","temperatura"),("pressure","pressione"),("voltage","tensione"),("fault","guasto"),("error","errore"),("warning","avviso"),("file","file"),("report","rapporto")],
"pt": [("engine speed","rotação do motor"),("coolant temperature","temperatura do líquido de refrigeração"),("intake air temperature","temperatura do ar de admissão"),("ambient temperature","temperatura ambiente"),("fuel temperature","temperatura do combustível"),("manifold pressure","pressão do coletor"),("battery voltage","tensão da bateria"),("throttle position","posição da borboleta"),("throttle angle","ângulo da borboleta"),("idle switch","interruptor de ralenti"),("closed loop","malha fechada"),("open loop","malha aberta"),("lambda sensor","sonda lambda"),("fuel trim","correção de combustível"),("ignition advance","avanço da ignição"),("stepper motor","motor passo-a-passo"),("crankshaft sensor","sensor da cambota"),("select the required position","selecione a posição pretendida"),("move idle air control motor","mover motor de controlo de ar de ralenti"),("only works outside","só funciona fora de"),("proceed at your own risk","prossiga por sua conta e risco"),("only do this if you know","faça isto apenas se conhecer"),("No file loaded","Nenhum ficheiro carregado"),("Select all","Selecionar tudo"),("Clear all","Desmarcar tudo"),("Available channels","Canais disponíveis"),("Waiting for ECU data","A aguardar dados da ECU"),("Capture as reference","Capturar como referência"),("Clear reference","Limpar referência"),("Export report","Exportar relatório"),("Automatic report","Relatório automático"),("Status","Estado"),("Check","Verificação"),("Value","Valor"),("Received","Recebido"),("Calculated","Calculado"),("Unknown","Desconhecido"),("Enabled","Ativado"),("Disabled","Desativado"),("Open","Abrir"),("Close","Fechar"),("sensor","sensor"),("temperature","temperatura"),("pressure","pressão"),("voltage","tensão"),("fault","avaria"),("error","erro"),("warning","aviso"),("file","ficheiro"),("report","relatório")],
"de": [("engine speed","Motordrehzahl"),("coolant temperature","Kühlmitteltemperatur"),("intake air temperature","Ansauglufttemperatur"),("ambient temperature","Umgebungstemperatur"),("fuel temperature","Kraftstofftemperatur"),("manifold pressure","Saugrohrdruck"),("battery voltage","Batteriespannung"),("throttle position","Drosselklappenstellung"),("throttle angle","Drosselklappenwinkel"),("idle switch","Leerlaufschalter"),("closed loop","geschlossener Regelkreis"),("open loop","offener Regelkreis"),("lambda sensor","Lambdasonde"),("fuel trim","Kraftstoffkorrektur"),("ignition advance","Zündvorverstellung"),("stepper motor","Schrittmotor"),("crankshaft sensor","Kurbelwellensensor"),("select the required position","gewünschte Position auswählen"),("move idle air control motor","Leerlaufluft-Stellmotor bewegen"),("only works outside","funktioniert nur außerhalb von"),("proceed at your own risk","Fortfahren auf eigene Gefahr"),("only do this if you know","nur ausführen, wenn Sie kennen"),("No file loaded","Keine Datei geladen"),("Select all","Alles auswählen"),("Clear all","Alles abwählen"),("Available channels","Verfügbare Kanäle"),("Waiting for ECU data","Warte auf ECU-Daten"),("Capture as reference","Als Referenz erfassen"),("Clear reference","Referenz löschen"),("Export report","Bericht exportieren"),("Automatic report","Automatischer Bericht"),("Status","Status"),("Check","Prüfung"),("Value","Wert"),("Received","Empfangen"),("Calculated","Berechnet"),("Unknown","Unbekannt"),("Enabled","Aktiviert"),("Disabled","Deaktiviert"),("Open","Öffnen"),("Close","Schließen"),("sensor","Sensor"),("temperature","Temperatur"),("pressure","Druck"),("voltage","Spannung"),("fault","Fehler"),("error","Fehler"),("warning","Warnung"),("file","Datei"),("report","Bericht")],
}

ENGLISH = re.compile(r"\b(the|this|that|with|without|from|into|only|before|after|select|check|value|status|sensor|engine|fuel|file|report|warning|error|unknown|received|calculated|enabled|disabled|open|closed|reset|capture|available|stored|maximum|minimum|position|temperature|pressure|voltage|signal|control|time|idle|lambda|battery|throttle|coil|boost|pump|circuit)\b", re.I)


def text(el):
    return "" if el is None else "".join(el.itertext())


def set_text(el, value):
    el.clear()
    el.text = value


def replace_phrases(value, lang):
    out = value
    for a, b in sorted(PHRASES[lang], key=lambda x: len(x[0]), reverse=True):
        out = re.sub(re.escape(a), b, out, flags=re.I)
    return out


for lang in LANGS:
    path = ROOT / f"ECUMemsManager_{lang}.ts"
    tree = ET.parse(path)
    changed = 0
    for ctx in tree.getroot().findall("context"):
        for msg in ctx.findall("message"):
            src_el = msg.find("source")
            tr_el = msg.find("translation")
            if src_el is None or tr_el is None:
                continue
            src = text(src_el)
            old = text(tr_el)
            new = None

            if src in EXACT:
                new = EXACT[src][lang]
            elif old in EN_EXACT:
                new = EN_EXACT[old][lang]
            elif ENGLISH.search(old):
                candidate = replace_phrases(old, lang)
                if candidate != old:
                    new = candidate

            if new is not None and new != old:
                set_text(tr_el, new)
                changed += 1

    ET.indent(tree, space=" ")
    tree.write(path, encoding="utf-8", xml_declaration=True)
    print(f"{lang}: second pass corrected {changed} translation(s)")
