#!/usr/bin/env python3
"""Lot 18: exact identity records and alternative physical signal leads, no ECU access."""
import ast,collections,csv,hashlib,json,re
from pathlib import Path
O=Path("research/instruments-integral-20261127/results")
R=Path("firmwares-extraits/Firmware Archive")
for filename,names in [("audit_16na_asm_complete.py",{"parse","num","reg"}),("audit_physical_first_lot16.py",{"rr","hx","imm","table","dest","physical"})]:
    for node in ast.parse((O.parent/filename).read_text()).body:
        if isinstance(node,ast.FunctionDef) and node.name in names:
            exec(compile(ast.Module(body=[node],type_ignores=[]),filename,"exec"))
stores={"st","stb"}
mut={"ld","ldb","ldbze","ldbs","add","addb","addc","addcb","sub","subb","subc","subcb","and","andb","or","orb","xor","xorb","clr","clrb","inc","incb","dec","decb","not","notb","neg","negb","shl","shlb","shr","shrb","mulu","mul","divu","div"}
lot=json.loads((O/"registre_sorties_physiques_lot16.json").read_text())
results=[]
for row in lot:
    n,f=row["firmware"],row["family"]
    p=R/("Rover MEMS 1.6/NA" if f=="Rover MEMS 1.6" else f)/f"{n}.asm"
    txt=p.read_text(errors="replace"); ins,db=parse(txt)
    b=p.with_suffix(".hex").read_bytes() if p.with_suffix(".hex").exists() else None
    es=table(b,db); mode=0; normal=None; entries=[]
    for a,h,l,t in es:
        if h==l==0xd0:mode+=1
        if h==l==0xd1:normal=mode
        entries.append((a,h,l,t,mode))
    cmds={h:t for a,h,l,t,m in entries if h==l and m==normal}
    keys=sorted(ins); ix={a:i for i,a in enumerate(keys)}
    def win(a,before=0,after=12):
        i=ix.get(a)
        return keys[max(0,i-before):i+after+1] if i is not None else []
    def evidence(ks):return [[hx(k),ins[k][2]] for k in ks]
    def ptrs(entry):
        out=[]
        for k in win(entry,0,4):
            o,q,s=ins[k]
            if o=="ld" and len(q)==2 and (v:=imm(q[1])) is not None and 0x2000<=v<0xa000:out.append((k,v))
        return out
    def raw(a,n):
        if b is not None and 0x2000<=a and a+n<=0x2000+len(b):return b[a-0x2000:a-0x2000+n]
        z=[db.get(a+i) for i in range(n)]
        return bytes(z) if None not in z else None
    identities=[]
    for k,v in ptrs(cmds.get(0xd0)):
        for j,w in ptrs(cmds.get(0xd1)):
            if v!=w+8:continue
            rec=raw(w,12)
            if rec is None or rec[:8]!=n.encode("ascii"):continue
            asmbytes=[db.get(w+i) for i in range(12)]
            identities.append({"d0_entry":hx(cmds[0xd0]),"d1_entry":hx(cmds[0xd1]),"d0_load":hx(k),"d1_load":hx(j),"record_address":hx(w),"name":rec[:8].decode(),"d0_nominal":rec[8:].hex().upper(),"record":rec.hex().upper(),"asm_db_complete":None not in asmbytes,"asm_db_matches":None not in asmbytes and bytes(asmbytes)==rec,"rom_present":b is not None,"d0_code":evidence(win(cmds[0xd0],0,5)),"d1_code":evidence(win(cmds[0xd1],0,5))})
    # For the 242 outside the 100 lot16 profiles, identify alternatives
    # that do NOT require a carry or a XOR: paired HSO channel levels,
    # software output set/clear at a common timer event, and dynamic HSO
    # command producers. These are leads, not inferred instrument functions.
    outside=not row["known_gauge"] and not row["known_tacho"] and not row["unqualified_linked_generators"]
    alt=[]
    if outside:
        if es:keys=[k for k in keys if not es[0][0]<=k<es[-1][0]+4];ix={a:i for i,a in enumerate(keys)}
        def d(k):return dest(*ins[k][:2])
        def sreg(k):return rr(d(k))
        def literal(k):
            o,q,s=ins[k]
            return imm(q[1]) if o in ("ld","ldb") and len(q)==2 else None
        hso=[k for k in keys if sreg(k)==6]
        # Local schedules: HSO command and actual HSO time store.
        for k in hso:
            v=literal(k); local=win(k,5,12)
            times=[j for j in local if sreg(j)==4 and j>k]
            if not times:continue
            if v is None:
                alt.append({"kind":"DYNAMIC_HSO_WITH_TIME","anchor":hx(k),"evidence":evidence(local)})
            else:
                if v&0x0f>3:continue
                for j in hso:
                    if j<=k or j-k>240 or literal(j) is None or literal(j)^v!=0x20:continue
                    end=win(j,0,12)
                    t2=[u for u in end if sreg(u)==4 and u>j]
                    if t2:alt.append({"kind":"HSO_OPPOSITE_LEVELS_TIMED","anchor":hx(k),"channel":v&15,"second":hx(j),"evidence":evidence(sorted(set(local+end)))})
        # Start from a direct port store, find same-bit set and clear
        # in the shadow register, coupled to a timer compare/update.
        for k in keys:
            o,q,s=ins[k]
            if o!="stb" or len(q)!=2 or not physical(q[1]) or physical(q[1]) in ("0004","0006"):continue
            shadow=rr(q[0])
            if shadow is None:continue
            local=win(k,24,24)
            changes=[]
            for j in local:
                oo,qq,_=ins[j]
                if len(qq)!=2 or rr(qq[0])!=shadow or imm(qq[1]) is None:continue
                z=imm(qq[1])&255
                if oo=="orb":changes.append((j,"set",z))
                if oo=="andb":changes.append((j,"clear",(~z)&255))
            for a,op,m in changes:
                if op!="set" or not m or m&(m-1):continue
                for j,op2,m2 in changes:
                    if op2!="clear" or m2!=m:continue
                    # Actual timer-related instruction rather than any
                    # arbitrary write; deadline register not assumed.
                    timers=[z for z in local if sreg(z)==4 or re.search(r"\b(?:TIMER[12]|HSO_TIME)\b",ins[z][2],re.I)]
                    if timers:alt.append({"kind":"PORT_SET_CLEAR_WITH_TIMER","anchor":hx(k),"bit":m.bit_length()-1,"set":hx(a),"clear":hx(j),"evidence":evidence(local)})
    results.append({"family":f,"firmware":n,"outside_lot16":outside,"identity":identities,"rom":b is not None,"asm_sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"alternative_signal_leads":alt})
assert len(results)==342 and sum(x["outside_lot16"] for x in results)==242
groups=collections.defaultdict(list)
for r in results:
    for rec in r["identity"]:
        groups[(r["family"],rec["d0_nominal"])].append(r["firmware"])
dup={f+" "+id:sorted(set(names)) for (f,id),names in groups.items() if len(set(names))>1}
known={x["firmware"]:x for x in lot}
summary={"profiles":len(results),"identity_records_found":sum(bool(r["identity"]) for r in results),"identity_missing":[r["firmware"] for r in results if not r["identity"]],"nominal_d0_collision_groups":dup,"outside_lot16":242,"outside_with_alternative_signal_leads":sum(bool(r["alternative_signal_leads"]) for r in results),"alternative_lead_types":dict(collections.Counter(z["kind"] for r in results for z in r["alternative_signal_leads"])),"candidate_identity_collisions":{r["firmware"]:dup.get(r["family"]+" "+r["identity"][0]["d0_nominal"],[]) for r in results if r["identity"] and not known[r["firmware"]]["known_gauge"] and not known[r["firmware"]]["known_tacho"] and known[r["firmware"]]["unqualified_linked_generators"]},"orphans":{r["firmware"]:r["identity"] for r in results if not r["rom"]},"authorized":0,"note":"D0/D1 records are static; runtime fourth byte may differ. Alternatives are scheduling leads, not qualified instruments; no proof of absence."}
(O/"bilan_identites_sorties_lot18.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
(O/"preuves_identites_sorties_lot18.json").write_text(json.dumps(results,indent=2,ensure_ascii=False))
print(json.dumps({k:v for k,v in summary.items() if k not in ("orphans","nominal_d0_collision_groups","candidate_identity_collisions")},ensure_ascii=False))
