#!/usr/bin/env python3
"""Independent ASM-first inventory, including orphan listings.
Review candidates only. No ECU communication or Manager changes."""
import re,csv,json,hashlib
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive/Rover MEMS 1.6/NA")
O=Path("research/instruments-integral-20261127/results")
known=set("AANMP000 AANMP001 AANMP002 ABNMF030 ABNMF032 ABNMP002 ABNMP005 ABNMP006 ACNMF024 ACNMF026 ACNMP001 ACNMP003 ACNMP004 ACNMP005 EKDMF009".split())
orphans={"ABNMP000","ABNMP003"}
def num(s):
    s=s.strip().lower()
    if s.startswith("#"):s=s[1:]
    try:return int(s[:-1],16) if s.endswith("h") else int(s)
    except ValueError:return None
def reg(s):
    s=s.strip().upper()
    alias={"HSO_COMMAND":6,"HSO_TIME":4}
    if s in alias:return alias[s]
    m=re.fullmatch(r"\(?INTMEM_([A-F0-9]+)(?:\+([0-9]+))?\)?(?:\[\])?",s)
    return int(m[1],16)+int(m[2] or 0) if m else None
def parse(txt):
    ins={}; db={}; offsets={}
    for l in txt.splitlines():
        m=re.match(r"^ROM:([0-9A-Fa-f]{4,8})\s+(.*)",l)
        if not m:continue
        a=int(m[1],16); s=m[2].split(";")[0].strip()
        if not s or re.match(r"[\w.$]+:",s):continue
        parts=s.split(None,1); op=parts[0].lower()
        if op=="db" and len(parts)>1:
            vals=[num(v) for v in parts[1].split(",")]
            j=offsets.get(a,a)
            for v in vals:
                if v is not None and 0<=v<=255:db[j]=v
                j+=1
            offsets[a]=j
        elif op not in ("dw","dd","ds","equ","org","align","end","endp","proc") and re.fullmatch(r"[a-z][a-z0-9]*",op):
            ins[a]=(op,[v.strip() for v in parts[1].split(",")] if len(parts)>1 else [],s)
    return ins,db
def analyse(p):
    n=p.stem; txt=p.read_text(errors="replace"); ins,db=parse(txt)
    rom=p.with_suffix(".hex"); b=rom.read_bytes() if rom.exists() else None
    def byte(a):
        if b is not None and 0x2000<=a<0x2000+len(b):return b[a-0x2000]
        return db.get(a)
    def raw(a,k):return [byte(a+i) for i in range(k)]
    # Remove data misdisassembled as code in known diagnostic table spans.
    # Identify ranges using actual D0 entries, not symbol names.
    tables=[]; occupied=set()
    for a in range(0x2000,0xA000):
        if raw(a,2)!=[0xd0,0xd0]:continue
        es=[]
        for j in range(a,a+1200,4):
            q=raw(j,4)
            if None in q:break
            h,l,x,y=q; dest=x+256*y
            if not 0x2000<=dest<0xA000:break
            es.append((j,h,l,dest))
            if (h,l)==(0x7f,0):break
        if len(es)>=15 and es[-1][1:3]==(0x7f,0) and {0xd0,0xd1,0xf4} <= {h for _,h,l,_ in es if h==l}:
            tables.append(es)
    table=max(tables,key=len,default=[])
    if table:occupied.update(range(table[0][0],table[-1][0]+4))
    keys=sorted(a for a in ins if a not in occupied)
    ix={a:i for i,a in enumerate(keys)}
    def context(i,before=6,after=12):
        return [[f"{a:04X}",ins[a][2]] for a in keys[max(0,i-before):i+after]]
    def op(i):return ins[keys[i]][0] if 0<=i<len(keys) else ""
    def arg(i):return ins[keys[i]][1] if 0<=i<len(keys) else []
    def dest(i):
        q=arg(i); o=op(i)
        if not q:return None
        return reg(q[1] if o in ("st","stb") and len(q)>1 else q[0])
    # Every HSO command write, including indexed/dynamic values; recover
    # preceding table-load bytes, bit transformations and timer writes.
    hso=[]; carry=[]
    for i,a in enumerate(keys):
        o,q,s=ins[a]
        if o in ("ldb","ld","stb","st","orb","andb","xorb","clrb","clr") and dest(i)==6:
            v=num(q[1]) if o in ("ldb","ld") and len(q)==2 else None
            e={"address":f"{a:04X}","instruction":s,"constant":v,"context":context(i,14,18),"indexed_sources":[]}
            if v is None and len(q)==2:
                src=reg(q[1]) if o in ("ldb","ld") else reg(q[0])
                for j in range(max(0,i-24),i):
                    qq=arg(j)
                    if op(j)!="ldb" or len(qq)!=2 or reg(qq[0])!=src:continue
                    m=re.fullmatch(r"([0-9A-Fa-f]+)h\[[^\]]+\]",qq[1])
                    if not m:continue
                    base=int(m[1],16)
                    e["indexed_sources"].append({"load":f"{keys[j]:04X}","base":f"{base:04X}","first_four_bytes":raw(base,4),"next_twelve_bytes":raw(base+4,12),"bound_requires_review":True})
            hso.append(e)
        if o=="addb" and len(q)>=2 and op(i+1) in ("jc","jnc"):
            # Do not require a symbol, fixed output channel, fixed bit or
            # two-operand addition. Retain every accumulator candidate.
            carry.append({"address":f"{a:04X}","source":q[-1],"source_reg":reg(q[-1]),"context":context(i,7,15)})
    # Independent known positive signatures: the complete zero/FF selector
    # stores a duty byte; find the accumulator that reads exactly that byte.
    selectors=[]
    for i in range(len(keys)-4):
        if [op(i+j) for j in range(5)]!=["jbc","clrb","jbc","decb","stb"]:continue
        x,y,z,w,v=[arg(i+j) for j in range(5)]
        if len(x)!=3 or len(z)!=3 or len(v)!=2 or y!=w or y!=v[:1]:continue
        target=f"ROM_{keys[i+4]:04X}"
        if x[-1].upper()!=target or z[-1].upper()!=target:continue
        d=reg(v[1])
        if d is None:continue
        matches=[c for c in carry if c["source_reg"]==d]
        if not matches:continue
        selectors.append({"address":f"{keys[i]:04X}","flag_tests":[x,z],"duty":f"{d:04X}","context":context(i,4,8),"accumulator_matches":matches})
    # Record physical port writes, including other bits and non-XOR forms.
    ports=[]
    for i,a in enumerate(keys):
        o,q,s=ins[a]
        if o in ("stb","st") and len(q)==2 and re.fullmatch(r"(?:0?40[0-9a-f]h|1ff[ef]h)\[\]|IOPORT[12]",q[1],re.I):
            ports.append({"address":f"{a:04X}","context":context(i,15,3)})
    # Compare two independent sources for tables where ASM data exists.
    mismatches=[a for a,v in db.items() if b is not None and 0x2000<=a<0x2000+len(b) and b[a-0x2000]!=v]
    return {"firmware":n,"known":n in known,"orphan":n in orphans,"rom":b is not None,"asm_sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"rom_sha256":hashlib.sha256(b).hexdigest() if b is not None else None,"instructions":len(keys),"db_bytes":len(db),"db_rom_mismatch_count":len(mismatches),"table_entries":len(table),"commands":[[f"{a:04X}",h,l,f"{t:04X}"] for a,h,l,t in table],"hso":hso,"carry":carry,"selectors":selectors,"ports":ports,"status":"REVIEW_REQUIRED"}
res=[analyse(p) for p in sorted(R.glob("*.asm"))]
# Retain all evidence before failing a positive control.
import gzip
with gzip.open(O/"preuves_16na_asm_lot13.jsonl.gz","wt",encoding="utf-8") as f:
    for x in res:f.write(json.dumps(x,ensure_ascii=False)+"\n")
def positive(x):
    return bool(x["selectors"])
def dyn(x):return [h for h in x["hso"] if h["indexed_sources"]]
with (O/"registre_16na_asm_lot13.csv").open("w",newline="",encoding="utf-8-sig") as f:
    w=csv.writer(f);w.writerow(["firmware","known","orphan","rom","instructions","db_bytes","db_rom_mismatches","commands","hso_writes","dynamic_indexed_hso","carry_candidates","selector_accumulator","port_writes","status"])
    for x in res:w.writerow([x["firmware"],x["known"],x["orphan"],x["rom"],x["instructions"],x["db_bytes"],x["db_rom_mismatch_count"],x["table_entries"],len(x["hso"]),len(dyn(x)),len(x["carry"]),len(x["selectors"]),len(x["ports"]),x["status"]])
missing=[x["firmware"] for x in res if (x["known"] or x["orphan"]) and not positive(x)]
summary={"asm_count":len(res),"rom_count":sum(x["rom"] for x in res),"orphans":[x["firmware"] for x in res if not x["rom"]],"positive_controls_recovered":[x["firmware"] for x in res if (x["known"] or x["orphan"]) and positive(x)],"missing_controls":missing,"new_selector_candidates":[x["firmware"] for x in res if not x["known"] and not x["orphan"] and positive(x)],"dynamic_hso_profiles":{x["firmware"]:[{"address":h["address"],"sources":h["indexed_sources"]} for h in dyn(x)] for x in res if dyn(x)},"limits":"Selector and HSO inventory are candidates, not qualification. Bounds and producers must be checked manually. Other periodic paths and hardware routing unresolved."}
(O/"bilan_16na_asm_lot13.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
print(json.dumps({k:v for k,v in summary.items() if k!="dynamic_hso_profiles"},ensure_ascii=False))
assert len(res)==125 and sum(x["rom"] for x in res)==123
assert {x["firmware"] for x in res if not x["rom"]}==orphans
assert not missing,missing
