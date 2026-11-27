#!/usr/bin/env python3
"""Independent firmware research. Never modifies Manager or commands an ECU."""
import csv, hashlib, json, re, collections, pathlib
ROOT=pathlib.Path("firmwares-extraits/Firmware Archive")
OUT=pathlib.Path("research/instruments-integral-20261127/results")
OUT.mkdir(parents=True,exist_ok=True)
GAUGE=set(["AANMP000","AANMP001","AANMP002","ABNMF030","ABNMF032","ABNMP002","ABNMP005","ABNMP006","ACNMF024","ACNMF026","ACNMP001","ACNMP003","ACNMP004","ACNMP005","EKDMF009"])
TACH={"TAHHG019":"900019","TBH7G019":"900019","TFH7G028":"900028","TFH7G029":"900029","TAH7G018":"900018","REGI1371":"901371","TAHHV006":"AD0006","TFH7G025":"900025","TFHSG011":"900011","TFHSV000":"510000","TFHSV006":"510006","REGI1369":"901369","TFH7G026":"900026","TFH7G027":"900027","TFHSG012":"900012","TFHSG013":"900013","TFHSG014":"900014","TAHHG013":"900013","TAHHV003":"AD0003","TAHHV004":"AD0004","TAHHV005":"AD0005","TBH7G016":"900016","TBH7G017":"900017","TFHSV001":"510001","TFHSV002":"510002","TFHSV003":"510003","TFHSV004":"510004","TFHSV005":"510005","TAH7V002":"A90002","TAH7V003":"A90003","TAH7V004":"A90004","TAH8V001":"A80001","TBH7V003":"A70003","TBH7V004":"A70004","TBH7V005":"A70005","TBH7V006":"A70006","TBH7V007":"A70007","TFH7V004":"440004","TFH7V005":"440005","TFH7V006":"440006","TFH7V007":"440007"}
def read_text(p):
    return p.read_bytes().decode("utf-8",errors="replace") if p.exists() else ""
def hexword(b,i):
    return int.from_bytes(b[i:i+2],"little") if 0<=i<=len(b)-2 else None
def fmt(a): return f"{a:04X}" if a is not None else ""
def table(b):
    # Older 80196 dialogue tables: exact command pairs, valid destination,
    # D0/F4/D1 anchors, and terminal 7F/00. Reject isolated byte matches.
    candidates=[]
    for m in re.finditer(b"\xd0\xd0",b):
        j=m.start(); entries=[]
        for k in range(j,min(j+800,len(b)-3),4):
            x,y=b[k:k+2]; a=hexword(b,k+2)
            if not (0x2000<=a<min(0x10000,0x2000+len(b))): break
            entries.append((x,y,a))
            if (x,y)==(0x7f,0):break
        exact={x for x,y,a in entries if x==y}
        if len(entries)>=15 and {0xd0,0xd1,0xf4}<=exact and entries[-1][:2]==(0x7f,0):
            candidates.append((j,entries))
    if not candidates:return None,{}
    j,entries=max(candidates,key=lambda p:len(p[1]))
    # Normal diagnostic table is the segment containing D1; earlier segments
    # may contain different F4 handlers for other modes.
    d1=next(i for i,e in enumerate(entries) if e[:2]==(0xd1,0xd1))
    start=max(i for i,e in enumerate(entries[:d1]) if e[:2]==(0xd0,0xd0))
    normal=entries[start:]
    return j+0x2000,{x:a for x,y,a in normal if x==y}
def xf_fields(t):
    ident=re.search(r"(?im)^\s*(?:ECU IDENTIFICATION\s*=\s*[^,]+,\s*|IDENT\s+)([0-9A-F]{2})[,\s]+([0-9A-F]{2})[,\s]+([0-9A-F]{2})",t)
    tune=re.search(r"(?im)^\s*TUNE AREA START ADDRESS\s*=\s*([0-9A-F]+)",t)
    if not tune:tune=re.search(r"(?im)Tune Area Start[^\n]*\n;\s*([0-9A-F]+)",t)
    symbols={}
    for line in t.splitlines():
        if re.search(r"(?i)tacho|tachometer|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tpg_force|tpg_onoff|speedometer|instrument[_ ]cluster",line):
            key=re.split(r"[,;\s]+",line.strip())[0].lower()
            symbols[key]=line.strip()
    return ("".join(ident.groups()).upper() if ident else "",int(tune.group(1),16) if tune else None,symbols)
def calibration(b,tune,symbols):
    vals={}
    for key in ("tacho_force_off_spd","tacho_force_on_spd"):
        line=symbols.get(key,"")
        m=re.search(r",\s*([0-9A-F]{4})\s*,\s*W\s*,",line,re.I)
        if m and tune is not None:
            vals[key]=hexword(b,tune-0x2000+int(m.group(1),16))
    return vals
def instructions(asm):
    out={}
    for l in asm.splitlines():
        m=re.match(r"^\s*(?:[A-Za-z0-9_]+:)?([0-9A-Fa-f]{4,8})\s+(.+)$",l)
        if not m:continue
        s=m.group(2).split(";")[0].strip()
        if not s or s.endswith(":"):continue
        if re.match(r"^(?:db|dw|dd|ds|dcb|dcw|dcd|equ|org|align|end|endp|proc)\b",s,re.I):continue
        if not re.match(r"^[A-Za-z][A-Za-z0-9.]*\s",s+" "):continue
        out.setdefault(int(m.group(1),16),s)
    return out
def handler(b,a,ins,n=14):
    if a is None:return {}
    k=a-0x2000
    return {"address":fmt(a),"raw_48":b[k:k+48].hex(" ") if 0<=k<len(b) else "",
            "asm":[f"{x:04X} {s}" for x,s in sorted(ins.items()) if a<=x<a+60][:n]}
def gauge_candidate(cmds,ins,syms):
    if not all(c in cmds for c in (0,0x10,0xf4)):return False
    if not any("tpg_force" in k or "tmpgauge" in k or "tmp_gauge" in k for k in syms):return False
    a,b=cmds[0],cmds[0x10]
    if abs(a-b)>30:return False
    lines=[s for x,s in sorted(ins.items()) if min(a,b)<=x<max(a,b)+30]
    return any(re.search(r"\borb\b",s,re.I) for s in lines) and any(re.search(r"\bandb\b",s,re.I) for s in lines)
def tacho_candidate(cmds,ins,vals):
    if not all(c in cmds for c in (0x63,0x6b,0xf4)):return False
    if vals.get("tacho_force_off_spd")!=0 or vals.get("tacho_force_on_spd")!=3000:return False
    a,b=cmds[0x63],cmds[0x6b]
    if abs(a-b)>60:return False
    lines=[s for x,s in sorted(ins.items()) if min(a,b)<=x<max(a,b)+40]
    return any(re.search(r"\bld\b",s,re.I) for s in lines) and any(re.search(r"\bst\b",s,re.I) for s in lines)
rows=[]; details=[]; data={}
for p in sorted(ROOT.rglob("*")):
    if not p.is_file() or p.suffix.lower() not in (".rom",".hex",".bin"):continue
    rel=p.relative_to(ROOT); family=rel.parts[0]; name=p.stem.upper()
    b=p.read_bytes()
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read_text(siblings.get(".asm",p.with_suffix(".asm")))
    xf=read_text(siblings.get(".xf3",siblings.get(".xf1",p.with_suffix(".xf3"))))
    ident,tune,syms=xf_fields(xf)
    proc=re.search(r"(?im)Processor\s*:\s*(.+)",asm)
    ins=instructions(asm)
    # Never assume the 80196 command protocol on a different family.
    older=family in ("Rover MEMS 1.3","Rover MEMS 1.6","Rover MEMS 1.6 TURBO","Rover MEMS 1.9")
    addr,cmds=table(b) if older else (None,{})
    vals=calibration(b,tune,syms) if older else {}
    g0=name in GAUGE and family=="Rover MEMS 1.6"
    t0=name in TACH and family=="Rover MEMS 1.9"
    g=gauge_candidate(cmds,ins,syms)
    t=tacho_candidate(cmds,ins,vals)
    # Candidate only. Does not prove physical output or restoration.
    row={"family":family,"subfamily":"/".join(rel.parts[:-1]),"firmware":name,"path":str(rel),
         "size":len(b),"sha256":hashlib.sha256(b).hexdigest(),"asm":bool(asm),"xf":bool(xf),
         "processor":proc.group(1).strip() if proc else "","identity":ident,"tune_start":fmt(tune),
         "table":fmt(addr),"cmd00":fmt(cmds.get(0)),"cmd10":fmt(cmds.get(0x10)),"cmd63":fmt(cmds.get(0x63)),
         "cmd6B":fmt(cmds.get(0x6b)),"cmdF4":fmt(cmds.get(0xf4)),"cmdD1":fmt(cmds.get(0xd1)),
         "tacho_off":vals.get("tacho_force_off_spd",""),"tacho_on":vals.get("tacho_force_on_spd",""),
         "gauge_status":"INTEGRE_227" if g0 else "CANDIDAT_A_PROUVER" if g else "NON_QUALIFIE",
         "tach_status":"INTEGRE_227" if t0 else "CANDIDAT_A_PROUVER" if t else "NON_QUALIFIE",
         "symbol_count":len(syms),"same_code_as":"","same_binary_as":""}
    details.append({"path":str(rel),"symbols":syms,"commands":{f"{c:02X}":fmt(a) for c,a in sorted(cmds.items())},
       "handlers":{f"{c:02X}":handler(b,cmds.get(c),ins) for c in (0,0x10,0x63,0x6b,0xf4,0xd1)},
       "instrument_asm_hits":[l[:300] for l in asm.splitlines() if re.search(r"(?i)tacho|rev.?counter|tmp.?gauge|temperature.?gauge|tpg_force|instrument.?cluster",l)][:150]})
    rows.append(row)
    data[str(rel)]=(b,tune,g0,t0,vals)
# Exact equality, no fuzzy inference. Even identical code requires identity,
# calibration, wiring and runtime compatibility before Manager integration.
by_hash=collections.defaultdict(list)
for row in rows:by_hash[row["sha256"]].append(row["path"])
known=[r for r in rows if r["gauge_status"]=="INTEGRE_227" or r["tach_status"]=="INTEGRE_227"]
for row in rows:
    b,tune,g0,t0,vals=data[row["path"]]
    row["same_binary_as"]=";".join(x for x in by_hash[row["sha256"]] if x!=row["path"])
    for ref in known:
        if ref["path"]==row["path"] or ref["family"]!=row["family"]:continue
        rb,rt,rg,rc,rv=data[ref["path"]]
        if tune is None or rt!=tune or not 0x2000<tune<=0x2000+min(len(b),len(rb)):continue
        if b[:tune-0x2000]==rb[:rt-0x2000]:
            row["same_code_as"]+=ref["firmware"]+";"
            if rg and not g0:row["gauge_status"]="CODE_IDENTIQUE_A_QUALIFIER"
            if rc and not t0 and rv==vals:row["tach_status"]="CODE_IDENTIQUE_A_QUALIFIER"
assert len(rows)==483, f"Corpus changed: {len(rows)}"
with (OUT/"inventaire_individuel.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
(OUT/"preuves_individuelles.json").write_text(json.dumps(details,indent=2,ensure_ascii=False),encoding="utf-8")
summary={}
for family in sorted({r["family"] for r in rows}):
    rr=[r for r in rows if r["family"]==family]
    summary[family]={"files":len(rr),"asm":sum(r["asm"] for r in rr),"xf":sum(r["xf"] for r in rr),
        "dialogue_tables":sum(bool(r["table"]) for r in rr),"gauge":dict(collections.Counter(r["gauge_status"] for r in rr)),
        "tach":dict(collections.Counter(r["tach_status"] for r in rr))}
(OUT/"bilan.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False),encoding="utf-8")
candidates=[r for r in rows if any(r[k] in ("CANDIDAT_A_PROUVER","CODE_IDENTIQUE_A_QUALIFIER") for k in ("gauge_status","tach_status"))]
(OUT/"candidats_integration.json").write_text(json.dumps(candidates,indent=2,ensure_ascii=False),encoding="utf-8")
print("CORPUS_BILAN "+json.dumps(summary,ensure_ascii=False))
for r in candidates:print("CANDIDAT "+json.dumps(r,ensure_ascii=False))
print("CORPUS_READ",len(rows),"CANDIDATS",len(candidates))
