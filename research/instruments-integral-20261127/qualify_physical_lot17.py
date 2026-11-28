#!/usr/bin/env python3
"""Qualify every lot16 candidate, with source evidence and no ECU access."""
import ast,collections,csv,json,re,hashlib
from pathlib import Path
O=Path("research/instruments-integral-20261127/results")
R=Path("firmwares-extraits/Firmware Archive")
for file,names in [("audit_16na_asm_complete.py",{"parse","reg","num"}),("audit_physical_first_lot16.py",{"table","rr","hx","imm"})]:
    for node in ast.parse((O.parent/file).read_text()).body:
        if isinstance(node,ast.FunctionDef) and node.name in names:
            exec(compile(ast.Module(body=[node],type_ignores=[]),file,"exec"))
rows=list(csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")))
inventory={(r["family"],r["firmware"]):r for r in rows}
lot=json.loads((O/"registre_sorties_physiques_lot16.json").read_text())
candidates=[r for r in lot if not r["known_gauge"] and not r["known_tacho"] and r["unqualified_linked_generators"]]
assert len(candidates)==44
def analyse(r):
    f,n=r["family"],r["firmware"]
    path=R/("Rover MEMS 1.6/NA" if f=="Rover MEMS 1.6" else f)/f"{n}.asm"
    txt=path.read_text(errors="replace"); ins,db=parse(txt)
    b=path.with_suffix(".hex").read_bytes() if path.with_suffix(".hex").exists() else None
    xfpath=next((path.with_suffix(e) for e in (".xf3",".xf1") if path.with_suffix(e).exists()),None)
    xf=xfpath.read_text(errors="replace") if xfpath else ""
    keys=sorted(ins); ix={a:i for i,a in enumerate(keys)}
    es=table(b,db)
    def window(a,lo=0,hi=50):return [(k,ins[k][0],ins[k][1]) for k in keys if a-lo<=k<a+hi]
    def proof(w):return [[hx(k),ins[k][2]] for k,_,_ in w]
    def regof(q):return rr(q)
    def bitwrite(w,reg,mask,op):
        # Resolve either direct register modification or load/change/store
        hits=[]
        for i,(a,o,q) in enumerate(w):
            if o!=op or len(q)!=2 or imm(q[1]) is None:continue
            v=imm(q[1])&255
            if (v if op=="orb" else (~v)&255)!=mask:continue
            dst=regof(q[0])
            if dst==reg:hits.append(a);continue
            if dst is None:continue
            before=[k for k,oo,qq in w[max(0,i-3):i] if oo=="ldb" and len(qq)==2 and regof(qq[0])==dst and regof(qq[1])==reg]
            after=[k for k,oo,qq in w[i+1:i+4] if oo=="stb" and len(qq)==2 and regof(qq[0])==dst and regof(qq[1])==reg]
            if before and after:hits.extend([before[-1],a,after[0]])
        return sorted(set(hits))
    def target(q):
        m=re.fullmatch(r"ROM_([0-9a-f]+)",q[-1],re.I) if q else None
        return int(m[1],16) if m else None
    # Follow both paths of short diagnostic routines, ending at return or
    # long jump to response; do not mistake adjacent commands for execution.
    def handler(a,limit=80,stop_long=True):
        seen=set(); todo=[a]
        while todo and len(seen)<limit:
            k=todo.pop()
            if k in seen or k not in ins:continue
            seen.add(k);o,q,_=ins[k]; i=ix[k]; t=target(q)
            if o in ("ret","reti","br") or (stop_long and o=="ljmp"):continue
            if o in ("sjmp","ljmp"):todo.append(t);continue
            if o.startswith("j") or o=="djnz":todo.append(t)
            if i+1<len(keys) and keys[i+1]-k<=8:todo.append(keys[i+1])
        return [(k,*ins[k][:2]) for k in sorted(seen)]
    mode=0; normal=None; entries=[]
    for a,h,l,t in es:
        if h==l==0xd0:mode+=1
        if h==l==0xd1:normal=mode
        entries.append((a,h,l,t,mode))
    cmds={h:t for a,h,l,t,m in entries if h==l and m==normal}
    row=inventory.get((f,n),{})
    ident=row.get("identity")
    same=[x["firmware"] for x in rows if x["family"]==f and ident and x["identity"]==ident]
    out={"firmware":n,"family":f,"rom_present":b is not None,"xf_present":bool(xf),"identity_nominal":ident,"same_nominal_identity":same,"asm_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"rom_sha256":hashlib.sha256(b).hexdigest() if b is not None else None,"xf_sha256":hashlib.sha256(xfpath.read_bytes()).hexdigest() if xfpath else None,"normal_commands":{hx(k):hx(v) for k,v in cmds.items()},"authorized":False}
    if row and b is not None:
        out["rom_inventory_hash_matches"]=out["rom_sha256"]==row["sha256"]
    if f=="Rover MEMS 1.6":
        spec={"ABNMP000":(0x2698,0x48f2,0x37b3,0x1ac),"ABNMP003":(0x2793,0x4fa3,0x3979,0x1ab)}
        a,s,restore,duty=spec[n]
        out.update(kind="GAUGE_ASM_ONLY",generator=proof(window(a,8,18)),selector=proof(window(s,0,20)),restore=proof(window(restore,0,8)),handlers={hx(k):proof(handler(cmds[k])) for k in (0,0x10,0xf4) if k in cmds},duty=hx(duty),status="ASM_ONLY_REVIEW_REQUIRED")
        return out
    # Inspect each actual signal-producing routine discovered in lot16.
    a=int(next(g["anchor"] for g in r["unqualified_linked_generators"] if g["kind"]=="PERIODIC_PORT"),16)
    sig=window(a,32,32); out["generator"]=proof(sig)
    toggle=[k for k,o,q in sig if o=="xorb" and len(q)==2 and regof(q[0])==0x22 and imm(q[1])==32 and k<a]
    port=ins.get(a,("",[],"")); portok=port[0]=="stb" and len(port[1])==2 and regof(port[1][0])==0x22 and port[1][1].upper()=="1FFEH[]"
    deadline=[k for k,o,q in sig if o=="st" and len(q)==2 and regof(q[1])==0x340 and k>a]
    # 1.9 aliases occasionally use (INTMEM_33D+3) or (INTMEM_33C+4).
    loads=[]
    for k,o,q in sig:
        if o=="ld" and len(q)==2 and k>a:
            m=re.fullmatch(r"([0-9a-f]+)h\[INTMEM_40\]",q[1],re.I)
            if m:loads.append((k,int(m[1],16)))
    out["signal_checks"]={"toggle":list(map(hx,toggle)),"port":portok,"deadline":list(map(hx,deadline)),"calibration_loads":[[hx(k),hx(v)] for k,v in loads]}
    on=handler(cmds[0x6b]) if 0x6b in cmds else []
    off=handler(cmds[0x63]) if 0x63 in cmds else []
    out["on"]=proof(on);out["off"]=proof(off)
    out["force_checks"]={"on_sets_030E":list(map(hx,bitwrite(on,0x30e,32,"orb"))),"off_clears_030E":list(map(hx,bitwrite(off,0x30e,32,"andb"))),"on_clears_0308":list(map(hx,bitwrite(on,0x308,32,"andb"))),"off_clears_0308":list(map(hx,bitwrite(off,0x308,32,"andb")))}
    # F4 path: follow the reachable short handler, inspect callees for the
    # exact restoring bit. A positive result is static, not a safety guarantee.
    rest=handler(cmds[0xf4],limit=120) if 0xf4 in cmds else []
    calls=[target(q) for k,o,q in rest if o in ("lcall","scall") and target(q) is not None]
    restored=[]
    for t in calls:
        w=handler(t,limit=180)
        hit=bitwrite(w,0x308,32,"orb")
        if hit:restored.append({"callee":hx(t),"writes":list(map(hx,hit)),"evidence":proof(w)})
    out["f4_handler"]=proof(rest);out["f4_restore"]=restored
    sym=[l.strip() for l in xf.splitlines() if re.match(r"\s*(?:tacho_sw_time|tacho_next_rti|tacho_force_on_spd|tacho_force_off_spd|tacho_display_speed)\b",l,re.I)]
    out["xf_symbols"]=sym
    out["xf_identity_lines"]=[l.strip() for l in xf.splitlines() if re.match(r"\s*(?:IDENT |ECU IDENTIFICATION)",l,re.I)]
    tune=int(row["tune_start"],16) if row.get("tune_start") else None
    fixed=any("tacho_sw_time" in l for l in sym) or bool(loads and not any("tacho_display_speed" in l for l in sym))
    out["kind"]="FIXED_PERIOD" if fixed else "SPEED_SIMULATION"
    cal=[]
    for k,offset in loads:
        if tune is None or b is None:continue
        pos=tune+offset-0x2000
        if 0<=pos<len(b)-1:
            val=int.from_bytes(b[pos:pos+2],"little")
            cal.append({"instruction":hx(k),"address":hx(tune+offset),"bytes":b[pos:pos+2].hex(),"delay_us":val,"frequency_hz":round(500000/val,6) if val else None})
    out["fixed_period_calibration"]=cal
    # Extract the XF calibration offset independently, including XF1 decimal.
    sx=next((l for l in sym if l.startswith("tacho_sw_time")), "")
    m=re.match(r"tacho_sw_time\s*,\s*([0-9a-f]+),",sx,re.I)
    m2=re.match(r"tacho_sw_time\s+(\d+)\s*;",sx,re.I)
    offsym=int(m[1],16) if m else int(m2[1]) if m2 else None
    out["xf_period_offset"]=offsym
    out["xf_period_matches_load"]=offsym is not None and any(v==offsym for _,v in loads)
    passed=bool(toggle and portok and deadline and all(out["force_checks"].values()) and restored)
    complete=bool(passed and fixed and b is not None and out.get("rom_inventory_hash_matches") and out["xf_period_matches_load"] and cal and all(c["delay_us"]>0 for c in cal))
    out["status"]="STATIC_FIXED_PERIOD_CHAIN" if complete else "ASM_ONLY_REVIEW_REQUIRED" if b is None else "REVIEW_REQUIRED"
    return out
results=[analyse(r) for r in candidates]
assert len(results)==44
summary={"profiles":len(results),"by_kind":dict(collections.Counter(r["kind"] for r in results)),"by_status":dict(collections.Counter(r["status"] for r in results)),"fixed_period_complete":[r["firmware"] for r in results if r["status"]=="STATIC_FIXED_PERIOD_CHAIN"],"manual_review":[r["firmware"] for r in results if r["status"]!="STATIC_FIXED_PERIOD_CHAIN"],"identity_collisions":{r["firmware"]:r["same_nominal_identity"] for r in results if len(r["same_nominal_identity"])>1},"authorized":0,"note":"Static chain only. Full four-byte D0 identity, instrument wiring, frequency-to-RPM relation, bench interlocks and physical validation not certified."}
(O/"qualification_sorties_lot17.json").write_text(json.dumps(results,indent=2,ensure_ascii=False))
(O/"bilan_qualification_sorties_lot17.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
print(json.dumps(summary,ensure_ascii=False))
