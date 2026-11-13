#!/usr/bin/env python3
"""Essai réel, isolé, Qwen3-4B GGUF + procédures RAVE FR locales.

Aucun changement du programme SESSION, aucun Setup ni publication de base.
Les réponses sont générées par le modèle dans llama.cpp, PAS préécrites.
"""
from __future__ import annotations
import json
import os
import sqlite3
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

DB = Path("output/rave-fr/mems_documentary_fr.sqlite")
MODEL = Path("models/Qwen3-4B-Q4_K_M.gguf")
SERVER = Path("llama-bin/llama-server")
HOST = "http://127.0.0.1:18439"
OUTPUT = Path("benchmark/qwen3_4b_test_dialogue.json")
MAX_PER_TURN = 270

SYSTEM = """Tu es IA MEMS, un assistant MECANICIEN CONVERSATIONNEL, pas l'onglet Base de données.
Tu aides le mécanicien en français naturel. Tu comprends le composant demandé,
puis tu poses UNE question utile lorsqu'il ne précise pas l'opération.
Quand il choisit l'opération, explique les VRAIES étapes de l'opération
à partir des seules données RAVE fournies, dans leur ordre.
L'utilisateur doit pouvoir demander la suite, puis changer de dépose à repose
sans répéter « joint de culasse ». Tu conserves le contexte de la conversation.
Ne donne jamais une liste de numéros de dossiers ou un catalogue SQLite comme réponse.
Les transcriptions techniques RAVE sont à vérifier sur l'original constructeur
avant l'intervention : ne prétends pas qu'elles sont validées.
Ne crée aucun couple, couple supplémentaire, étape, page ni illustration.
Si les étapes disponibles sont trop nombreuses, commence les premières étapes
et propose la suite. En cas de doute sur la motorisation, demande confirmation.
Réponse claire, concise et naturelle, sans inventer. /no_think"""

def procedures() -> dict[str, dict]:
    con = sqlite3.connect(f"file:{DB}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    all_procs = con.execute(
        "SELECT title,source_page,verification_status,raw_json "
        "FROM documentary_entries "
        "WHERE source_table='mems_procedure' AND title LIKE '%CULASSE%'"
    ).fetchall()
    data: dict[str,dict] = {}
    for r in all_procs:
        raw = json.loads(r["raw_json"])
        label = "depose" if "dépose" in r["title"].lower() else (
            "repose" if "repose" in r["title"].lower() else "autre")
        key = raw.get("procedure_key")
        if not key or label not in ("depose","repose"):
            continue
        data[label] = {
            "title": r["title"], "page": r["source_page"],
            "procedure_key": key, "status": r["verification_status"],
            "steps": [], "requirements": [],
        }
    if not all(x in data for x in ("depose","repose")):
        raise RuntimeError("REAL_RAVE_FR_CULASSE_PROCEDURES_MISSING")
    for row in con.execute(
        "SELECT source_table,source_page,verification_status,raw_json "
        "FROM documentary_entries WHERE source_table IN "
        "('mems_procedure_step','mems_procedure_requirement')"
    ):
        raw = json.loads(row["raw_json"])
        for entry in data.values():
            if raw.get("procedure_key") != entry["procedure_key"]:
                continue
            if row["source_table"] == "mems_procedure_step":
                entry["steps"].append({
                    "n": raw.get("step_no",0),
                    "text": raw.get("instruction_source",""),
                    "page": row["source_page"],
                    "status": row["verification_status"]
                })
            else:
                entry["requirements"].append({
                    "text": raw.get("requirement_source",""),
                    "page": row["source_page"],
                    "status": row["verification_status"]
                })
    con.close()
    for label,count in (("depose",36),("repose",35)):
        entry=data[label]
        entry["steps"].sort(key=lambda s: int(s["n"] or 0))
        if len(entry["steps"]) != count:
            raise RuntimeError(f"REAL_RAVE_FR_{label.upper()}_{count}_STEPS_REQUIRED_ACTUAL_{len(entry['steps'])}")
        if any(not str(s["text"]).strip() for s in entry["steps"]):
            raise RuntimeError(f"REAL_RAVE_FR_{label.upper()}_EMPTY_STEP")
        entry["pages"]=sorted(set([entry["page"]]+[s["page"] for s in entry["steps"] if s["page"]]))
    print("REAL_RAVE_FR_FOUND",json.dumps({
        label:{"steps":len(v["steps"]),"pages":v["pages"],
               "warnings":len(v["requirements"]),"validation":v["status"]}
        for label,v in data.items()
    },ensure_ascii=False),flush=True)
    return data

def evidence(name: str, info: dict, step_from: int = 1, step_to: int | None = None) -> str:
    steps=[s for s in info["steps"] if s["n"]>=step_from and (step_to is None or s["n"]<=step_to)]
    lines=[
        f"DOCUMENT ORIGINAL : RCL0193FRE / opération « {info['title']} »",
        f"Type demandé : {name}. Total original : {len(info['steps'])} étapes.",
        f"Pages document original : {', '.join(map(str,info['pages']))}.",
        "Statut : transcription documentaire à vérifier dans les pages constructeur.",
        "EXTRAITS ORIGINAUX DES ÉTAPES (ne pas substituer des connaissances générales) :"
    ]
    for s in steps:
        lines.append(f"ÉTAPE SOURCE {s['n']} (page {s['page']}) : {s['text']}")
    if step_from == 1 and info["requirements"]:
        lines.append("EXIGENCES D'OPÉRATION SOURCE :")
        for item in info["requirements"]:
            if item["text"]:
                lines.append(f"(page {item['page']}) {item['text']}")
    return "\n".join(lines)

def health() -> None:
    for i in range(180):
        try:
            with urllib.request.urlopen(HOST+"/health",timeout=3) as r:
                if r.status==200:
                    print("QWEN3_4B_REAL_MODEL_LOADED",flush=True)
                    return
        except (urllib.error.URLError,TimeoutError):
            pass
        time.sleep(2)
    raise RuntimeError("REAL_QWEN3_4B_SERVER_NOT_READY")

def chat(messages: list[dict],user_text:str,grounding:str) -> dict:
    payload={
        "model":"Qwen3-4B",
        "messages":messages+[{
            "role":"user",
            "content":user_text+"\n\n"+grounding+"\n\n/no_think"
        }],
        "temperature":0.25,
        "top_p":0.8,
        "max_tokens":MAX_PER_TURN,
        "stream":False,
        "chat_template_kwargs":{"enable_thinking":False},
    }
    request=urllib.request.Request(
        HOST+"/v1/chat/completions",data=json.dumps(payload,ensure_ascii=False).encode(),
        headers={"Content-Type":"application/json"},method="POST")
    began=time.monotonic()
    with urllib.request.urlopen(request,timeout=260) as resp:
        result=json.loads(resp.read())
    msg=result["choices"][0]["message"]
    answer=(msg.get("content") or "").strip()
    if not answer:
        raise RuntimeError("QWEN3_4B_MODEL_EMPTY_REPLY")
    print("QWEN3_4B_RESPONSE_SECONDS",round(time.monotonic()-began,2),flush=True)
    return {"answer":answer,"seconds":round(time.monotonic()-began,2),
            "tokens":result.get("usage"),"model_response":msg}

def main() -> int:
    data=procedures()
    if not MODEL.is_file() or MODEL.stat().st_size < 2_000_000_000:
        raise RuntimeError("OFFICIAL_QWEN3_4B_GGUF_FILE_NOT_DOWNLOADED")
    if not SERVER.is_file():
        raise RuntimeError("LLAMA_CPP_SERVER_NOT_PRESENT")
    env=os.environ.copy()
    env["LD_LIBRARY_PATH"]=str(SERVER.parent.resolve())+":"+env.get("LD_LIBRARY_PATH","")
    Path("benchmark").mkdir(exist_ok=True)
    with open("benchmark/llama-server.log","w",encoding="utf-8") as output:
        proc=subprocess.Popen([
            str(SERVER.resolve()),"-m",str(MODEL.resolve()),
            "--host","127.0.0.1","--port","18439",
            "-c","8192","-t","4","-ngl","0","--parallel","1"
        ],env=env,stdout=output,stderr=subprocess.STDOUT)
        try:
            health()
            history=[{"role":"system","content":SYSTEM}]
            tests=[]
            queries=[
                ("composant",
                 "Je cherche des informations sur la culasse.",
                 "SOURCES DOCUMENTAIRES DISPONIBLES : joint de culasse — dépose : 36 étapes (pages 59 à 62) ; joint de culasse — repose : 35 étapes (pages 62 à 63). Le manuel Rover existe en français. Ne déroule pas la procédure tant que l'utilisateur n'a pas dit quelle intervention il veut."),
                ("depose",
                 "Je veux déposer le joint de culasse.",
                 evidence("dépose",data["depose"],1,36)+
                 "\nDemande d'utilisateur : commence la procédure avec quelques premières étapes exactes et propose de continuer."),
                ("suite",
                 "Et ensuite ?",
                 evidence("dépose",data["depose"],6,17)+
                 "\nL'utilisateur demande la suite de la dépose, pas la repose. Poursuis aux étapes suivantes."),
                ("repose",
                 "Et pour la repose ?",
                 evidence("repose",data["repose"],1,35)+
                 "\nL'utilisateur parle toujours du joint de culasse. Commence la repose, sans citer les étapes de dépose."),
                ("precision",
                 "Sur quel moteur cette procédure s'applique-t-elle exactement ?",
                 "La source disponible est le manuel Rover RCL0193FRE, édition liée aux véhicules couverts par ce manuel. Aucun numéro de série exact ni caractéristique du véhicule connecté ne t'a été communiqué. Ne suppose pas que la procédure est universelle."),
            ]
            for label,question,context in queries:
                result=chat(history,question,context)
                answer=result["answer"]
                history.append({"role":"user","content":question+"\n\n"+context+"\n\n/no_think"})
                history.append({"role":"assistant","content":answer})
                tests.append({
                    "test":label,"question":question,"response":answer,
                    "seconds":result["seconds"],"usage":result["tokens"]})
                print(f"=== QWEN3_4B_REAL_{label.upper()}_REPLY ===",flush=True)
                print(answer,flush=True)
                print("=== END_REAL_REPLY ===",flush=True)
            report={
                "model":"Qwen/Qwen3-4B-GGUF Q4_K_M",
                "runtime":"llama.cpp b12240 CPU, no network inference",
                "setup_unchanged":True,"source_database_published":False,
                "original_documents_sent_to_cloud":False,
                "steps_checked":{"depose":36,"repose":35},
                "tests":tests,
                "note":"Essai de modèle et contexte RAVE, PAS une validation en application Windows."
            }
            OUTPUT.parent.mkdir(exist_ok=True)
            OUTPUT.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding="utf-8")
            print("QWEN3_4B_REAL_DIALOGUE_BENCHMARK_COMPLETE",flush=True)
        finally:
            proc.terminate()
            try:proc.wait(timeout=10)
            except subprocess.TimeoutExpired:proc.kill()
    return 0

if __name__=="__main__":
    try:sys.exit(main())
    except Exception as e:
        print("QWEN3_4B_BENCHMARK_ERROR",type(e).__name__,str(e),flush=True)
        log=Path("benchmark/llama-server.log")
        if log.is_file():print("SERVER_LOG_TAIL",log.read_text(errors="replace")[-3500:],flush=True)
        sys.exit(1)
