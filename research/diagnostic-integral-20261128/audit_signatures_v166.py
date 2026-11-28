#!/usr/bin/env python3
"""V1.66: resolve context from actual 80196 dispatch branches; no ECU communication."""
import csv,json,re,hashlib,collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
PRE=Path("research/diagnostic-integral-20261128")
SRC=PRE/"results_v165/signatures"
OUT=PRE/"results_v166";OUT.mkdir(parents=True,exist_ok=True)
scope=json.loads((PRE/"results_v165/perimetre.json").read_text())
prior=list(csv.DictReader((SRC/"repertoire_signatures.csv").open(encoding="utf-8-sig")))
inv=list(csv.DictReader((SRC/"couverture_449.csv").open(encoding="utf-8-sig")))
assert len(inv)==449 and all(r["family"] in scope["included"] for r in inv)
by=collections.defaultdict(list)
for r in prior:by[r["path"]].append(r)
def hx(n):return f"{n:04X}"
def reg(s):return s.split(",")[0].strip()
def target(s):
    m=re.fullmatch(r"ROM_([0-9A-F]+)",s.strip())
    return int(m[1],16) if m else None
def ld(s):
    m=re.fullmatch(r"([^,]+),\s*#([0-9A-F]+)h",s)
    return (m[1].strip(),int(m[2],16)) if m else None
contexts=[];out=[];coverage=[]
for f in inv:
    path=ROOT/f["path"];raw=path.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==f["sha256"]
    text=(path.parent/f["asm"]).read_text(encoding="latin-1") if f["asm"] else ""
    ins={int(m[1],16):(m[2],m[3].strip()) for m in re.finditer(r"(?m)^ROM:([0-9A-F]+)\s+([a-z][a-z0-9.]*)\s+([^;\r\n]+)",text)}
    rs=by[f["path"]]; labels={}; proof={}
    def at(a,op):
        z=ins.get(a,("",""))
        return z[1] if z[0]==op else None
    def pair(a):
        x=at(a,"ld");y=at(a+4,"ld")
        if not x or not y:return None
        l=ld(x);h=ld(y)
        if not l or not h:return None
        return l,h
    def compare(a,imm):
        x=at(a,"cmpb")
        return x is not None and re.search(r",\s*#"+imm+r"h$",x,re.I) is not None
    def br(a,op):
        return target(at(a,op) or "")
    def match(a):
        # Verify preceding 14 and 1E conditions, default branch, state branch,
        # same pointer registers, common join, and selector state substitution.
        p14,p1,pdef,pstate=[pair(x) for x in (a-15,a,a+10,a+20)]
        if not all((p14,p1,pdef,pstate)):return
        if not all([compare(a-20,"14"),compare(a-5,"1E")]):return
        if reg(at(a-20,"cmpb"))!=reg(at(a-5,"cmpb")):return
        if br(a-17,"jne")!=a-5 or br(a-2,"jne")!=a+10:return
        join=a+34
        if any(br(x,"sjmp")!=join for x in (a-7,a+8,a+18)):return
        if not at(a+28,"ldb") or not at(a+31,"ldb") or not at(join,"lcall"):return
        if any(tuple(y[0] for y in x)!=tuple(y[0] for y in p1) for x in (p14,pdef,pstate)):return
        for k,p in ((a-15,p14),(a,p1),(a+10,pdef),(a+20,pstate)):
            lo,hi=p[0][1],p[1][1]
            if not (0x2000<=lo<hi<=len(raw)+0x2000 and (hi-lo)%4==0):return
        return [(a-15,"14",p14),(a,"1E",p1),(a+10,"DEFAULT_SELECTOR",pdef),(a+20,"INTERNAL_STATE",pstate)],join
    if f["family"] in ("Rover MEMS 1.6/NA","Rover MEMS 1.9"):
        anchors={int(m[1],16) for r in rs if r["mode"]=="1E" for m in [re.search(r"LD ([0-9A-F]+)",r["detail"])] if m}
        for a in sorted(anchors):
            z=match(a)
            if not z:continue
            entries,join=z
            for site,mode,p in entries:
                labels[site]=mode
                proof[site]=f"join={hx(join)}; mode14={hx(a-20)}; mode1E={hx(a-5)}; pointer bounds={hx(p[0][1])}-{hx(p[1][1])}"
                contexts.append(dict(path=f["path"],family=f["family"],mode=mode,load=hx(site),start=hx(p[0][1]),end=hx(p[1][1]),proof=proof[site]))
    n=0
    for r in rs:
        r=dict(r);r["previous_status"]=r["status"];r["previous_mode"]=r["mode"];r["context_proof"]=""
        if r["status"]=="TABLE_CONTEXT_UNRESOLVED":
            m=re.search(r"LD ([0-9A-F]+)",r["detail"]);site=int(m[1],16) if m else -1
            if site in labels:
                r["mode"]=labels[site];r["status"]="INTERNAL_STATE_TABLE_LINKED" if labels[site]=="INTERNAL_STATE" else "STATIC_TABLE_CONTEXT_LINKED"
                r["context_proof"]=proof[site];n+=1
        out.append(r)
    coverage.append(dict(path=f["path"],family=f["family"],firmware=f["firmware"],contexts=len(labels),reclassified=n,remaining=sum(r["status"]=="TABLE_CONTEXT_UNRESOLVED" for r in rs)-n))
assert len(out)==len(prior)
assert any(x["firmware"]=="AANMP002" and x["contexts"]==4 for x in coverage)
assert any(x["firmware"]=="FTST0152" and x["contexts"]==4 for x in coverage)
def write(name,rr):
    with (OUT/name).open("w",encoding="utf-8-sig",newline="") as f:
        w=csv.DictWriter(f,fieldnames=list(rr[0]));w.writeheader();w.writerows(rr)
write("repertoire_signatures.csv",out);write("contextes_tables.csv",contexts);write("couverture_449.csv",coverage)
g=collections.defaultdict(list)
for r in out:
    if r["command"]:g[(r["family"],r["mode"],r["command"],r["status"])].append(r)
write("index_commandes.csv",[dict(family=k[0],mode=k[1],command=k[2],status=k[3],firmwares=len({r["path"] for r in a}),occurrences=len(a)) for k,a in sorted(g.items())])
b={"firmwares":len(inv),"entries":len(out),"reclassified":sum(x["reclassified"] for x in coverage),"context_firmwares":sum(x["contexts"]==4 for x in coverage),"statuses":dict(collections.Counter(x["status"] for x in out)),"families":{},"complete":False}
for f in scope["included"]:
    a=[r for r in coverage if r["family"]==f];b["families"][f]={"firmwares":len(a),"four_contexts":sum(x["contexts"]==4 for x in a),"reclassified":sum(x["reclassified"] for x in a),"remaining_table_contexts":sum(x["remaining"] for x in a)}
(OUT/"bilan.json").write_text(json.dumps(b,indent=2),encoding="utf-8");print(json.dumps(b))
