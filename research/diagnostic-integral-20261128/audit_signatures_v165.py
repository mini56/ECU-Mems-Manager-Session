#!/usr/bin/env python3
"""First pass: catalogue exact ROM signatures; petrol only; no ECU access."""
import csv,json,re,hashlib,collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
PRE=Path("research/diagnostic-integral-20261128")
OUT=PRE/"results_v165/signatures";OUT.mkdir(parents=True,exist_ok=True)
scope=json.loads((PRE/"results_v165/perimetre.json").read_text())
old={x["path"]:x for x in json.loads((PRE/"results_v164/matrice_483.json").read_text()) if x["family"] in scope["included"]}
special={x["n"].upper():x for x in json.loads((PRE/"results_v162/lecteurs_13_turbo.json").read_text())}
files={str(p.relative_to(ROOT)):p for p in ROOT.rglob("*") if p.is_file() and p.suffix.lower() in (".rom",".hex",".bin") and str(p.relative_to(ROOT).parent) in scope["included"]}
assert set(files)==set(old) and len(files)==449
def one(v):return v[0] if isinstance(v,list) else v
def hx(n):return f"{n:04X}"
def fmt(b):return b.hex(" ").upper()
def u(b,i):return int.from_bytes(b[i:i+2],"little")
rows=[]; coverage=[]
for rel,p in sorted(files.items()):
    v=old[rel];raw=p.read_bytes();assert hashlib.sha256(raw).hexdigest()==v["sha256"]
    sib={x.suffix.lower():x for x in p.parent.iterdir() if x.stem.lower()==p.stem.lower()}
    asm=sib.get(".asm");s=asm.read_text(encoding="latin-1") if asm else ""
    ins={int(m[1],16):(m[2],m[3].strip()) for m in re.finditer(r"(?m)^ROM:([0-9A-F]+)\s+([a-z][a-z0-9.]*)\s+([^;\r\n]+)",s)}
    start=len(rows);anchor=False;issues=[]
    def add(mode,cmd,site,handler,sig,status,kind,detail=""):
        rows.append(dict(path=rel,family=v["family"],firmware=p.stem,mode=mode,command=cmd,site=hx(site),handler=hx(handler) if handler is not None else "",signature=fmt(sig),status=status,kind=kind,detail=detail,physical="NOT_TESTED"))
    if v["status"]=="PREVIOUS_WORD_READER_LINKED":
        h=int(one(v["previous_handler"]),16);t=int(one(v["previous_table"]),16);dc=int(one(v["previous_dc"]),16);base=0x2000
        if p.stem.upper() in special:
            q=special[p.stem.upper()];anchor=True
            for j in range(t,t+36,2):
                ptr=u(raw,j-base)
                if not base<=ptr<base+len(raw):
                    issues.append("Pointer out of range "+hx(j));continue
                if j==t:
                    add("1E","00-7F",j,ptr,raw[j-base:j-base+2],"STATIC_LINKED","WORD_READER")
                else:
                    found=False
                    for m in re.finditer(rb"\x99(.)\x47",raw[ptr-base:ptr-base+96],re.S):
                        at=ptr+m.start();code=m[1][0];found=True
                        add("1E",f"{code:02X}",at,ptr,m[0],"SELECTOR_CANDIDATE","POINTER_HANDLER_COMPARE","Pointer "+hx(j)+"; "+str(ins.get(at,"no ASM")))
                    if not found:add("1E","",j,ptr,raw[j-base:j-base+2],"COMMAND_UNRESOLVED","POINTER")
            add("1E","DC",dc,dc,raw[dc-base:dc-base+12],"STATIC_LINKED","BLOCK_SELECTION")
            issues.append("Pointer lists: remaining modes and selector paths pending")
        else:
            pairs=[]
            for m in re.finditer(rb"\xa1(..)(.)\xa1(..)(.)",raw,re.S):
                lo=u(m[1],0);hi=u(m[3],0);at=m.start()+base
                if lo<=t<hi and 0<hi-lo<=1024 and (hi-lo)%4==0 and (t-lo)%4==0 and m[2]!=m[4] and raw[t-base:t-base+4]==bytes([127,0])+h.to_bytes(2,"little") and ins.get(at,("",))[0]=="ld" and ins.get(at+4,("",))[0]=="ld":
                    pairs.append((m.start(),lo,hi,m[2],m[4]))
            if len(pairs)==1:
                off,lo,hi,r1,r2=pairs[0];anchor=True
                for m in re.finditer(rb"\xa1(..)"+re.escape(r1)+rb"\xa1(..)"+re.escape(r2),raw[max(0,off-80):off+96],re.S):
                    pos=max(0,off-80)+m.start();l=u(m[1],0);e=u(m[2],0)
                    if not(base<=l<e<=base+len(raw) and (e-l)%4==0 and e-l<=1024 and ins.get(pos+base,("",))[0]=="ld" and ins.get(pos+base+4,("",))[0]=="ld"):continue
                    mode="1E" if pos==off else "OTHER_CONTEXT"
                    for j in range(l,e,4):
                        b=raw[j-base:j-base+4];high,low=b[:2];ptr=u(b,2)
                        if not(low<=high and base<=ptr<base+len(raw)):
                            issues.append("Unresolved entry "+hx(j));continue
                        cmd=f"{low:02X}" if low==high else f"{low:02X}-{high:02X}"
                        add(mode,cmd,j,ptr,b,"STATIC_TABLE_LINKED" if mode=="1E" else "TABLE_CONTEXT_UNRESOLVED","SELECTOR_RANGE","Bounds "+hx(l)+"-"+hx(e)+"; LD "+hx(pos+base))
            else:
                issues.append("Unique bounded table not recovered: "+str(len(pairs)))
                add("1E","00-7F",t,h,raw[t-base:t-base+4],"STATIC_LINKED","PREVIOUS_WORD_READER")
                add("1E","DC",dc,dc,raw[dc-base:dc-base+12],"STATIC_LINKED","PREVIOUS_BLOCK_SELECTION")
    else:
        q=next(x for x in v["candidates"] if x.get("verified"));base=int(q["base"],16);d=int(q["dispatch"],16);buf=int(q["buffer"],16);h=int(q["handler"],16)
        decl=[m for m in re.finditer(r"(?m)^ROM:([0-9A-F]{8})\s+(sub_[0-9A-F]+):[^\r\n]*",s) if int(m[1],16)<=d]
        body=""
        if decl:
            m=decl[-1];e=re.search(r"(?m)^.*; End of function "+re.escape(m[2])+r"\s*$",s[m.end():])
            if e:body=s[m.start():m.end()+e.end()]
        if body and re.search(r"(?m)^ROM:"+f"{d:08X}"+r"\s+cmpi\.b\s+#\$23,",body):
            anchor=True
            for m in re.finditer(r"(?m)^ROM:([0-9A-F]{8})\s+cmpi\.b\s+#(\$[0-9A-F]+|\d+),\$([0-9A-F]+)\((d[0-7])\.w\)",body):
                at=int(m[1],16);code=int(m[2][1:],16) if m[2].startswith("$") else int(m[2]);b=raw[at-base:at-base+8]
                if int(m[3],16)!=buf or len(b)!=8 or b[:4]!=bytes([12,48,0,code]) or int.from_bytes(b[6:8],"big")!=buf:continue
                before=body[max(0,m.start()-450):m.start()];idx=re.findall(r"move\.w\s+#(\$[0-9A-F]+|\d+),"+m[4]+r"\b",before)
                index=idx[-1] if idx else "unknown"
                add("SERVICE",f"{code:02X}",at,h if at==d else None,b,"STATIC_LINKED" if at==d else "SERVICE_COMPARE_CANDIDATE","BUFFER_COMPARE","Index "+index+"; "+m[0].strip())
        else:
            issues.append("Main ASM boundary not recovered")
            add("SERVICE","23",d,h,bytes.fromhex(q["dispatch_bytes"]),"STATIC_LINKED","PREVIOUS_READER")
        issues.append("Subfunction tables and secondary dispatch pending")
    coverage.append(dict(path=rel,family=v["family"],firmware=p.stem,sha256=v["sha256"],asm=asm.name if asm else "",xf=(sib.get(".xf3") or sib.get(".xf1")).name if sib.get(".xf3") or sib.get(".xf1") else "",anchor_recovered=anchor,entries=len(rows)-start,issues="; ".join(dict.fromkeys(issues)),status="FIRST_PASS_REVIEW_REQUIRED"))
def write(n,a):
    with (OUT/n).open("w",encoding="utf-8-sig",newline="") as f:
        w=csv.DictWriter(f,fieldnames=list(a[0]));w.writeheader();w.writerows(a)
write("repertoire_signatures.csv",rows);write("couverture_449.csv",coverage)
g=collections.defaultdict(list)
for r in rows:
    if r["command"]:g[(r["family"],r["mode"],r["command"],r["status"])].append(r)
write("index_commandes.csv",[dict(family=k[0],mode=k[1],command=k[2],status=k[3],firmwares=len({r["path"] for r in a}),occurrences=len(a),signatures=len({r["signature"] for r in a})) for k,a in sorted(g.items())])
summary={"scope":"PETROL_ONLY","firmwares":len(coverage),"diesel_excluded":34,"entries":len(rows),"anchors":sum(r["anchor_recovered"] for r in coverage),"statuses":dict(collections.Counter(r["status"] for r in rows)),"families":{},"complete":False}
for f in scope["included"]:
    a=[r for r in coverage if r["family"]==f];b=[r for r in rows if r["family"]==f];summary["families"][f]={"firmwares":len(a),"anchors":sum(r["anchor_recovered"] for r in a),"entries":len(b)}
(OUT/"bilan.json").write_text(json.dumps(summary,indent=2),encoding="utf-8");print(json.dumps(summary))
