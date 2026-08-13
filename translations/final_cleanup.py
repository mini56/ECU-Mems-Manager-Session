#!/usr/bin/env python3
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent
LANGS = ("es", "it", "pt", "de")

# Final pass. complete_catalogs.py and translate_remaining.py remain the main
# translators; this file removes the residual English fragments seen in UI,
# long help bubbles and technical labels.

EXACT = {
    "File": {"es":"Archivo","it":"File","pt":"Ficheiro","de":"Datei"},
    "Options": {"es":"Opciones","it":"Opzioni","pt":"Opções","de":"Optionen"},
    "Help": {"es":"Ayuda","it":"Aiuto","pt":"Ajuda","de":"Hilfe"},
    "Nom du fichier :": {"es":"Nombre del archivo:","it":"Nome file:","pt":"Nome do ficheiro:","de":"Dateiname:"},
    "Contact ralenti": {"es":"Interruptor de ralentí","it":"Interruttore minimo","pt":"Interruptor de ralenti","de":"Leerlaufschalter"},
    "Boucle fermée": {"es":"Bucle cerrado","it":"Anello chiuso","pt":"Malha fechada","de":"Geschlossener Regelkreis"},
    "Correction carburant court terme": {"es":"Corrección de combustible a corto plazo","it":"Correzione carburante a breve termine","pt":"Correção de combustível a curto prazo","de":"Kurzzeit-Kraftstoffkorrektur"},
    "Correction carburant long terme": {"es":"Corrección de combustible a largo plazo","it":"Correzione carburante a lungo termine","pt":"Correção de combustível a longo prazo","de":"Langzeit-Kraftstoffkorrektur"},
    "Erreur de ralenti à chaud corrigée": {"es":"Error de ralentí en caliente corregido","it":"Errore minimo a caldo corretto","pt":"Erro de ralenti a quente corrigido","de":"Korrigierter Warmleerlauffehler"},
    "Trame 7D14-15 brute": {"es":"Trama 7D14-15 sin procesar","it":"Frame 7D14-15 grezzo","pt":"Trama 7D14-15 bruta","de":"7D14-15-Rohframe"},
    "Signal régime et capteur PMH": {"es":"Señal de régimen y sensor de cigüeñal","it":"Segnale regime e sensore albero motore","pt":"Sinal de rotação e sensor da cambota","de":"Drehzahlsignal und Kurbelwellensensor"},
    "Enregistré": {"es":"Registrado","it":"Registrato","pt":"Registado","de":"Gespeichert"},
    "Anomalies :": {"es":"Anomalías:","it":"Anomalie:","pt":"Anomalias:","de":"Anomalien:"},
    "Erreur signal lambda": {"es":"Error de señal lambda","it":"Errore segnale lambda","pt":"Erro no sinal lambda","de":"Lambda-Signalfehler"},
    "Signal lambda anormalement élevé": {"es":"Señal lambda anormalmente alta","it":"Segnale lambda anormalmente alto","pt":"Sinal lambda anormalmente alto","de":"Lambda-Signal ungewöhnlich hoch"},
    "Signal lambda anormalement bas": {"es":"Señal lambda anormalmente baja","it":"Segnale lambda anormalmente basso","pt":"Sinal lambda anormalmente baixo","de":"Lambda-Signal ungewöhnlich niedrig"},
    "Position mini du moteur pas-à-pas atteinte": {"es":"Posición mínima del motor paso a paso alcanzada","it":"Posizione minima del motore passo-passo raggiunta","pt":"Posição mínima do motor passo-a-passo atingida","de":"Mindestposition des Schrittmotors erreicht"},
    "Compteur Jack au maximum (>255 réinitialisations)": {"es":"Contador Jack al máximo (>255 reinicios)","it":"Contatore Jack al massimo (>255 reset)","pt":"Contador Jack no máximo (>255 reinicializações)","de":"Jack-Zähler am Maximum (>255 Rücksetzungen)"},
    "Réchauffeur collecteur": {"es":"Calentador del colector","it":"Riscaldatore collettore","pt":"Aquecedor do coletor","de":"Ansaugkrümmerheizung"},
    "Pompe à carburant": {"es":"Bomba de combustible","it":"Pompa carburante","pt":"Bomba de combustível","de":"Kraftstoffpumpe"},
    "Chauffage sonde O2/lambda": {"es":"Calentador de sonda O2/lambda","it":"Riscaldatore sonda O2/lambda","pt":"Aquecedor da sonda O2/lambda","de":"O2-/Lambda-Sondenheizung"},
    "Électrovanne purge canister": {"es":"Válvula de purga del canister","it":"Valvola spurgo canister","pt":"Válvula de purga do canister","de":"Tankentlüftungsventil"},
    "Embrayage climatisation": {"es":"Embrague del A/C","it":"Frizione A/C","pt":"Embraiagem do A/C","de":"Klimakompressorkupplung"},
    "Électrovanne suralimentation": {"es":"Válvula de control de sobrealimentación","it":"Valvola controllo sovralimentazione","pt":"Válvula de controlo da sobrealimentação","de":"Ladedruckregelventil"},
    "Ventilateur 1": {"es":"Ventilador 1","it":"Ventola 1","pt":"Ventoinha 1","de":"Lüfter 1"},
    "Ventilateur 2": {"es":"Ventilador 2","it":"Ventola 2","pt":"Ventoinha 2","de":"Lüfter 2"},
    "Ventilateur 3": {"es":"Ventilador 3","it":"Ventola 3","pt":"Ventoinha 3","de":"Lüfter 3"},
    "Bobine d'allumage": {"es":"Bobina de encendido","it":"Bobina di accensione","pt":"Bobina de ignição","de":"Zündspule"},
    "Tester": {"es":"Probar","it":"Prova","pt":"Testar","de":"Testen"},
    "Déclencher": {"es":"Activar","it":"Attiva","pt":"Ativar","de":"Auslösen"},
    "Étincelle": {"es":"Chispa","it":"Scintilla","pt":"Faísca","de":"Funke"},
    "Tous les actionneurs désactivés / réinitialisés": {"es":"Todos los actuadores desactivados / restablecidos","it":"Tutti gli attuatori disattivati / ripristinati","pt":"Todos os atuadores desativados / reinicializados","de":"Alle Aktuatoren deaktiviert / zurückgesetzt"},
    "Mode inconnu": {"es":"Modo desconocido","it":"Modalità sconosciuta","pt":"Modo desconhecido","de":"Unbekannter Modus"},
    "Session normale": {"es":"Sesión normal","it":"Sessione normale","pt":"Sessão normal","de":"Normale Sitzung"},
}

# Generic numbered undocumented labels.
UNDOC_PREFIX = {
    "es": "No documentado",
    "it": "Non documentato",
    "pt": "Não documentado",
    "de": "Nicht dokumentiert",
}

# English fragments still present after the first two passes. Longest first.
FRAGMENTS = {
"it": {
    "Only do this if you know Colin's command list well.": "Fallo solo se conosci bene l'elenco dei comandi di Colin.",
    "Proceed at your own risk!": "Procedi a tuo rischio!",
    "Open Command Prompt or PowerShell and change to the folder containing readmems": "Apri il Prompt dei comandi o PowerShell e passa alla cartella contenente readmems",
    "The statuses and guidance in this report are consistency checks and do not replace manufacturer specifications.": "Gli stati e i consigli di questo rapporto sono controlli di coerenza e non sostituiscono le specifiche del costruttore.",
    "No active fault bit in 0x7D/0x80 frames.": "Nessun bit di guasto attivo nei frame 0x7D/0x80.",
    "No known active TPS fault in the monitored bits.": "Nessun guasto TPS noto attivo nei bit monitorati.",
    "No critical IAC/error combination detected.": "Nessuna combinazione critica IAC/errore rilevata.",
    "Closed loop active: lambda corrections are being applied.": "Anello chiuso attivo: le correzioni lambda sono applicate.",
    "Closed loop is currently inactive; interpret together with temperature and engine conditions.": "L'anello chiuso è attualmente inattivo; interpretare insieme alla temperatura e alle condizioni del motore.",
    "Large correction: check for air leaks, fuel pressure/supply, injection or lambda measurement before changing settings.": "Correzione importante: controllare prese d'aria, pressione/alimentazione carburante, iniezione o misura lambda prima di modificare le impostazioni.",
    "IAC at its limit with idle error: check stepper motor, stop, air leaks and mechanical adjustment.": "IAC a fine corsa con errore minimo: controllare motore passo-passo, battuta, prese d'aria e regolazione meccanica.",
    "Measurement displayed. The 1.9–3.1 ms check is applied automatically only when battery voltage is close to 14 V.": "Misura visualizzata. Il controllo 1,9–3,1 ms viene applicato automaticamente solo con tensione batteria vicina a 14 V.",
    "Reference captured on %1. This comparison helps identify drift; it is not a manufacturer specification.": "Riferimento acquisito il %1. Il confronto aiuta a individuare una deriva; non costituisce una specifica del costruttore.",
    "Unable to open file": "Impossibile aprire il file",
    "Unable to read this capture.": "Impossibile leggere questa acquisizione.",
    "Unable to delete this capture.": "Impossibile eliminare questa acquisizione.",
    "Permanently delete the selected capture?": "Eliminare definitivamente l'acquisizione selezionata?",
    "No file loaded": "Nessun file caricato",
    "No usable data in this file.": "Nessun dato utilizzabile in questo file.",
    "CSV file is empty or incomplete.": "Il file CSV è vuoto o incompleto.",
    "Unrecognized CSV file format.": "Formato file CSV non riconosciuto.",
    "Return to stacked display": "Torna alla visualizzazione impilata",
    "Overlay all selected curves": "Sovrapponi tutte le curve selezionate",
    "Available channels": "Canali disponibili",
    "Select all": "Seleziona tutto",
    "View captures": "Visualizza acquisizioni",
    "Capture as reference": "Acquisisci come riferimento",
    "Automatic report": "Rapporto automatico",
    "Export report": "Esporta rapporto",
    "Waiting for ECU data": "In attesa dei dati ECU",
    "Interpretation / action": "Interpretazione / azione",
    "Hot idle error (corrected)": "Errore minimo a caldo corretto",
    "Hot idle error": "Errore minimo a caldo",
    "Engine-speed signal and crankshaft sensor": "Segnale regime e sensore albero motore",
    "Engine-speed signal": "Segnale regime motore",
    "Crankshaft position sensor": "Sensore posizione albero motore",
    "Lambda signal error": "Errore segnale lambda",
    "Lambda signal abnormally high": "Segnale lambda anormalmente alto",
    "Lambda signal abnormally low": "Segnale lambda anormalmente basso",
    "Stepper motor minimum position reached": "Posizione minima del motore passo-passo raggiunta",
    "Jack counter at maximum (>255 resets)": "Contatore Jack al massimo (>255 reset)",
    "Manifold heater": "Riscaldatore collettore",
    "Fuel pump": "Pompa carburante",
    "O2/lambda sensor heater": "Riscaldatore sonda O2/lambda",
    "Canister purge valve": "Valvola spurgo canister",
    "Boost control valve": "Valvola controllo sovralimentazione",
    "Ignition coil": "Bobina di accensione",
    "Ignition switch": "Contatto accensione",
    "Throttle angle": "Angolo farfalla",
    "Air/fuel ratio": "Rapporto aria/carburante",
    "Lambda frequency": "Frequenza lambda",
    "Lambda duty cycle": "Ciclo lambda",
    "Lambda status": "Stato lambda",
    "Closed loop": "Anello chiuso",
    "Long-term fuel trim": "Correzione carburante a lungo termine",
    "Short-term fuel trim": "Correzione carburante a breve termine",
    "Idle set point": "Setpoint minimo",
    "Idle base position": "Posizione base minimo",
    "Idle speed offset": "Offset regime minimo",
    "Idle speed deviation": "Deviazione regime minimo",
    "Idle air control motor position": "Posizione motore controllo aria minimo",
    "Park/neutral switch": "Interruttore folle",
    "A/C switch": "Interruttore A/C",
    "Coil charge time": "Tempo di carica bobina",
    "Raw 7D14-15 frame": "Frame 7D14-15 grezzo",
    "Unknown / DTC?": "Sconosciuto / DTC?",
    "Unknown": "Sconosciuto",
    "Undocumented": "Non documentato",
    "Recorded": "Registrato",
    "Check": "Controllo",
    "Value": "Valore",
    "Status": "Stato",
    "Received": "Ricevuto",
    "Calculated": "Calcolato",
    "Description": "Descrizione",
    "Parameter": "Parametro",
    "Interpreted": "Interpretato",
    "Enabled / Disabled": "Attivato / Disattivato",
    "Enabled": "Attivato",
    "Disabled": "Disattivato",
    "Open / Closed": "Aperto / Chiuso",
    "Open": "Aperto",
    "Close": "Chiudi",
    "signal yes/no": "segnale sì/no",
    "battery": "batteria",
    "idle error": "errore minimo",
    "warning": "avvertimento",
    "anomaly": "anomalia",
    "Reference": "Riferimento",
    "Date": "Data",
},
"es": {
    "Undocumented":"No documentado", "Unknown":"Desconocido", "Recorded":"Registrado",
    "Hot idle error (corrected)":"Error de ralentí en caliente corregido",
    "Engine-speed signal and crankshaft sensor":"Señal de régimen y sensor de cigüeñal",
    "Ignition coil":"Bobina de encendido", "Manifold heater":"Calentador del colector",
    "Canister purge valve":"Válvula de purga del canister", "Boost control valve":"Válvula de control de sobrealimentación",
    "Available channels":"Canales disponibles", "Automatic report":"Informe automático",
    "Waiting for ECU data":"Esperando datos de ECU", "Interpretation / action":"Interpretación / acción",
},
"pt": {
    "Undocumented":"Não documentado", "Unknown":"Desconhecido", "Recorded":"Registado",
    "Hot idle error (corrected)":"Erro de ralenti a quente corrigido",
    "Engine-speed signal and crankshaft sensor":"Sinal de rotação e sensor da cambota",
    "Ignition coil":"Bobina de ignição", "Manifold heater":"Aquecedor do coletor",
    "Canister purge valve":"Válvula de purga do canister", "Boost control valve":"Válvula de controlo da sobrealimentação",
    "Available channels":"Canais disponíveis", "Automatic report":"Relatório automático",
    "Waiting for ECU data":"A aguardar dados da ECU", "Interpretation / action":"Interpretação / ação",
},
"de": {
    "Undocumented":"Nicht dokumentiert", "Unknown":"Unbekannt", "Recorded":"Gespeichert",
    "Hot idle error (corrected)":"Korrigierter Warmleerlauffehler",
    "Engine-speed signal and crankshaft sensor":"Drehzahlsignal und Kurbelwellensensor",
    "Ignition coil":"Zündspule", "Manifold heater":"Ansaugkrümmerheizung",
    "Canister purge valve":"Tankentlüftungsventil", "Boost control valve":"Ladedruckregelventil",
    "Available channels":"Verfügbare Kanäle", "Automatic report":"Automatischer Bericht",
    "Waiting for ECU data":"Warte auf ECU-Daten", "Interpretation / action":"Interpretation / Maßnahme",
},
}

def clean_fragment(text: str, lang: str) -> str:
    if not text:
        return text
    out = text
    for old, new in sorted(FRAGMENTS.get(lang, {}).items(), key=lambda kv: len(kv[0]), reverse=True):
        out = out.replace(old, new)
    return out


def process(path: Path, lang: str):
    tree = ET.parse(path)
    root = tree.getroot()
    changed = 0
    for msg in root.findall('.//message'):
        src_el = msg.find('source')
        tr_el = msg.find('translation')
        if src_el is None or tr_el is None:
            continue
        src = src_el.text or ''
        cur = tr_el.text or ''
        new = cur

        if src in EXACT:
            new = EXACT[src][lang]
        else:
            m = re.fullmatch(r"Non documenté\s*([0-9A-Fa-f]+)", src.strip())
            if m:
                new = f"{UNDOC_PREFIX[lang]} {m.group(1)}"
            else:
                new = clean_fragment(cur, lang)

        if new != cur:
            tr_el.text = new
            tr_el.attrib.pop('type', None)
            changed += 1

    ET.indent(tree, space=' ')
    tree.write(path, encoding='utf-8', xml_declaration=True)
    print(f"{path.name}: final cleanup {changed} entrées")


def main():
    for lang in LANGS:
        process(ROOT / f"ECUMemsManager_{lang}.ts", lang)

if __name__ == '__main__':
    main()
