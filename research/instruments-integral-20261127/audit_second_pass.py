#!/usr/bin/env python3
"""Independent, read-only second-pass evidence. Never enables an ECU command."""
import csv, json, re, hashlib, collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
OUT=Path("research/instruments-integral-20261127/results")
rows=list(csv.DictReader((OUT/"inventaire_individuel.csv").open(encoding="utf-8-sig")))
def hx(x): return f"{x:04X}"
def bit(x): return x.bit_length()-1 if x and x&(x-1)==0 else None
def match(b,p): return [i for i in range(len(b)-len(p)+1) if all(x is None or b[i+j]==x for j,x in enumerate(p))]
def read(p): return p.read_bytes().decode("utf-8",errors="replace") if p.exists() else ""
def instr(asm):
    out={}
    for l in asm.splitlines():
        m=re.match(r"^\s*(?:[A-Za-z0-9_]+:)?([0-9A-Fa-f]{4,8})\s+([A-Za-z][A-Za-z0-9.]*\s+[^;]+|ret\b|reti\b|nop\b|pushf\b|popf\b)",l)
        if m:
            s=m.group(2).strip()
            if not re.match(r"^(?:db|dw|dd|ds|dc[bdw]|equ|org|align|end|endp|proc)\b",s,re.I): out.setdefault(int(m.group(1),16),s)
    return out
def window(ins,addr,length=35):
    return [f"{hx(i)} {s}" for i,s in ins.items() if addr<=i<addr+length]
def adr(row,k): return int(row[k],16) if row.get(k) else None
def xfvar(xf,n):
    m=re.search(r"(?im)^\s*"+n+r"\s+(?:,?\s*)?([0-9a-f]{2});([0-9a-f]{2});",xf)
    if m:return int("".join(m.groups()),16)
    m=re.search(r"(?im)^\s*"+n+r"\s*,\s*([0-9a-f]{4})\s*,",xf)
    return int(m.group(1),16) if m else None
def refs(ins,v):
    if v is None:return []
    pat=r"\bINTMEM_0*"+f"{v:X}"+r"\b"
    return [(i,s) for i,s in ins.items() if re.search(pat,s,re.I)]
def gauge(row,b,ins,xf):
    cmd=adr(row,"cmd10")
    low=adr(row,"cmd00")
    known=row["gauge_status"]=="INTEGRE_227"
    ans={"historical_integration":known,"handler":None,"consumers":[],"other_branch_candidates":[],"restoration_candidates":[],"status":"NO_KNOWN_COMMAND_PAIR"}
    if cmd is None or low is None:return ans
    h=b[cmd-0x2000:cmd-0x2000+14]
    ans["handler"]={"address":hx(cmd),"low":hx(low),"bytes":h.hex(" "),"asm":window(ins,cmd,18)}
    if len(h)!=14 or low!=cmd+5 or h[0]!=0x91 or h[3:6]!=bytes([0x20,3,0x71]) or h[6]!=(255^h[1]) or h[7]!=h[2] or h[8]!=0x91 or bit(h[1]) is None or bit(h[9]) is None:
        ans["status"]="PAIR_DIFFERENT_NEEDS_TRACE";return ans
    d,dr,f,fr=h[1],h[2],h[9],h[10]
    db,fb=bit(d),bit(f)
    ans["handler"].update(direction_register=hx(dr),direction_bit=db,force_register=hx(fr),force_bit=fb)
    for i in match(b,[0x71,255^f,fr]):
        ans["restoration_candidates"].append({"address":hx(i+0x2000),"asm":window(ins,i+0x2000,8)})
    for opcode in (0x30+fb,0x38+fb):
        for i in match(b,[opcode,fr]):
            a=i+0x2000
            if a in ins and re.match(r"jb[cs]\b",ins[a],re.I):
                ans["other_branch_candidates"].append({"address":hx(a),"asm":window(ins,a,18)})
    for i in match(b,[0x30+fb,fr,7,0x11,0x1c,0x30+db,dr,2,0x15,0x1c,0xc7,1,None,None,0x1c]):
        a=i+0x2000; v=b[i+12]+256*b[i+13]
        references=refs(ins,v)
        output=[]
        for ra,s in references:
            if not re.match(r"addb\b",s,re.I):continue
            w=window(ins,max(0,ra-8),30)
            output.append({"address":hx(ra),"asm":w})
        ans["consumers"].append({"address":hx(a),"variable":hx(v),"xf_gauge_variable":hx(xfvar(xf,"m_tmpgauge_dr")) if xfvar(xf,"m_tmpgauge_dr") is not None else None,"asm":window(ins,a,18),"all_variable_refs":[[hx(a),s] for a,s in references],"output_accumulator_candidates":output})
    if not ans["consumers"]:
        ans["status"]="HANDLER_WITHOUT_KNOWN_CONSUMER" if not ans["other_branch_candidates"] else "OTHER_CONSUMER_NEEDS_TRACE"
    else:
        ans["status"]="CONSUMER_AND_OUTPUT_CANDIDATE" if any(c["output_accumulator_candidates"] for c in ans["consumers"]) else "CONSUMER_WITHOUT_OUTPUT_PROOF"
    return ans
def tach(row,b,ins,xf):
    on,off=adr(row,"cmd6B"),adr(row,"cmd63")
    known=row["tach_status"]=="INTEGRE_227"
    ans={"historical_integration":known,"status":"NO_KNOWN_COMMAND_PAIR","on":[],"off":[],"variable_refs":[],"calibration":{}}
    if on is None or off is None:return ans
    ans["on"]=window(ins,on,45);ans["off"]=window(ins,off,45)
    var=xfvar(xf,"tacho_display_speed")
    if var is None:
        # The two branches converge on a store. Record, but do not assume
        # that an unlabelled variable is the physical display speed.
        for a,s in ins.items():
            if max(on,off)<=a<max(on,off)+35:
                m=re.match(r"st\s+\w+,\s*INTMEM_([0-9A-F]+)\[\]",s,re.I)
                if m: var=int(m.group(1),16);break
    ans["variable"]=hx(var) if var is not None else None
    ans["variable_refs"]=[[hx(a),s,window(ins,a,22)] for a,s in refs(ins,var)]
    for key in ("tacho_force_off_spd","tacho_force_on_spd"):
        offset=xfvar(xf,key); tune=adr(row,"tune_start")
        if offset is not None and tune is not None and 0<=tune-0x2000+offset<len(b)-1:
            ans["calibration"][key]=int.from_bytes(b[tune-0x2000+offset:tune-0x2000+offset+2],"little")
    o=" ".join(ans["on"]);f=" ".join(ans["off"])
    loads=bool(re.search(r"\bld\s+[^,]+,\s*(?:0?1A|26)h?\[",o,re.I) and re.search(r"\bld\s+[^,]+,\s*(?:0?18|24)h?\[",f,re.I))
    ans["status"]="SPEED_LOADS_AND_VARIABLE_CANDIDATE" if loads and var is not None else "COMMAND_PAIR_DIFFERENT_OR_LOGICAL_ONLY"
    return ans
results=[]
for row in rows:
    p=ROOT/row["path"]; raw=p.read_bytes()
    assert len(raw)==int(row["size"]) and hashlib.sha256(raw).hexdigest()==row["sha256"],p
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read(siblings.get(".asm",p.with_suffix(".asm")))
    xf=read(siblings.get(".xf3",siblings.get(".xf1",p.with_suffix(".xf1"))))
    ins=instr(asm)
    b=raw[:max(0,adr(row,"tune_start")-0x2000)] if adr(row,"tune_start") and adr(row,"tune_start")>0x2000 else raw
    older=row["family"] in ("Rover MEMS 1.3","Rover MEMS 1.6","Rover MEMS 1.6 TURBO","Rover MEMS 1.9")
    if older:
        g=gauge(row,b,ins,xf);t=tach(row,raw,ins,xf)
    else:
        g={"status":"OTHER_ARCHITECTURE_REQUIRES_PROTOCOL_TRACE"};t=dict(g)
    # Exact F4 handler and initial call/jump targets, not a proof that
    # restoration is reached on every execution path.
    restore=adr(row,"cmdF4")
    restore_evidence=[]
    if restore is not None:
        restore_evidence=window(ins,restore,50)
        for l in list(restore_evidence):
            m=re.search(r"\b(?:lcall|scall|call|ljmp|sjmp)\s+ROM_([0-9A-F]+)\b",l,re.I)
            if m:restore_evidence+=window(ins,int(m.group(1),16),50)
    symbols=[l.strip() for l in xf.splitlines() if re.search(r"tach|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tpg_force|tpg_onoff|instrument[_ ]cluster",l,re.I)]
    hits=[l.strip() for l in asm.splitlines() if re.search(r"tach|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tpg_force|tpg_onoff|instrument[_ ]cluster",l,re.I)]
    results.append({"family":row["family"],"firmware":row["firmware"],"path":row["path"],"sha256":row["sha256"],"identity":row["identity"],"asm_instructions":len(ins),"symbols":symbols,"assembly_symbol_hits":hits,"F4_entry":row["cmdF4"],"F4_evidence":restore_evidence,"gauge":g,"tachometer":t})
assert len(results)==483
summary={}
for fam in sorted({r["family"] for r in results}):
    rr=[r for r in results if r["family"]==fam]
    summary[fam]={"firmwares":len(rr),"gauge":dict(collections.Counter(r["gauge"]["status"] for r in rr)),"tachometer":dict(collections.Counter(r["tachometer"]["status"] for r in rr)),"gauge_historical":sum(r["gauge"].get("historical_integration",False) for r in rr),"tach_historical":sum(r["tachometer"].get("historical_integration",False) for r in rr)}
# Positive controls from previously established assembly traces.
for name,expected in (("AANMP002","01AB"),("EKDMF009","01AC")):
    r=next(r for r in results if r["firmware"]==name)
    assert any(c["variable"]==expected for c in r["gauge"]["consumers"]),name
assert next(r for r in results if r["firmware"]=="TFH7G029")["tachometer"]["status"]=="SPEED_LOADS_AND_VARIABLE_CANDIDATE"
(OUT/"preuves_seconde_passe_20261128.json").write_text(json.dumps(results,indent=2,ensure_ascii=False),encoding="utf-8")
(OUT/"bilan_seconde_passe_20261128.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False),encoding="utf-8")
with (OUT/"registre_seconde_passe_20261128.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(["family","firmware","path","identity","gauge_status","tach_status","gauge_historical","tach_historical","asm_instructions","symbols_count"])
    for r in results:w.writerow([r["family"],r["firmware"],r["path"],r["identity"],r["gauge"]["status"],r["tachometer"]["status"],r["gauge"].get("historical_integration",False),r["tachometer"].get("historical_integration",False),r["asm_instructions"],len(r["symbols"])])
print(json.dumps(summary,indent=2))
