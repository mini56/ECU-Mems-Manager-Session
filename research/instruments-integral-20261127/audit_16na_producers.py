#!/usr/bin/env python3
"""Independent output/bit producer inventory for 123 MEMS 1.6 NA ROMs.
No write to ECU. No profile enabled by this script."""
import ast,bisect,collections,csv,gzip,hashlib,json,re
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive")
O=Path("research/instruments-integral-20261127/results")
instpat=re.compile(r"tacho|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tempg_pwm|tpg_force|tpg_onoff|instrument[_ ]cluster",re.I)
diagpat=re.compile(r"kp_.*(?:control|override)|diagnostic.*override|actuator",re.I)
def load_functions(path,names):
    tree=ast.parse(Path(path).read_text())
    for node in tree.body:
        if isinstance(node,ast.FunctionDef) and node.name in names:
            exec(compile(ast.Module(body=[node],type_ignores=[]),path,"exec"),globals())
load_functions("research/instruments-integral-20261127/audit_reverse.py",{"hx","read","parse","target","tables","symbols","refs","generic_masks","reachable"})
load_functions("research/instruments-integral-20261127/audit_16na_outputs.py",{"var","imm","op","args","xfmap","onebit"})
rows=[r for r in csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")) if r["family"]=="Rover MEMS 1.6"]
assert len(rows)==123
prior={x["firmware"]:x for x in map(json.loads,gzip.open(O/"preuves_16na_sorties_lot03.jsonl.gz","rt",encoding="utf-8"))}
assert len(prior)==123
def analyse(r):
    p=R/r["path"]; b=p.read_bytes(); assert hashlib.sha256(b).hexdigest()==r["sha256"],p
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read(siblings.get(".asm",p.with_suffix(".asm"))); xf=read(siblings.get(".xf3",siblings.get(".xf1",p.with_suffix(".xf1"))))
    ins,labels,_=parse(asm); cs=tables(b); sy=xfmap(xf); keys=sorted(ins); idx={a:i for i,a in enumerate(keys)}
    assert cs and ins,p
    def context(a,lo=10,hi=12):
        i=idx[a];return [[hx(k),ins[k]] for k in keys[max(0,i-lo):i+hi]]
    def dest(s):
        q=args(s)
        if not q:return None
        if op(s) in ("st","stb"):return q[1] if len(q)>1 else None
        if op(s) in ("ld","ldb","ldbze","ldbs","andb","orb","xorb","addb","subb","and","or","xor","add","sub","clrb","clr","incb","decb","notb","not","shlb","shrb"):return q[0]
        return None
    def readreg(s,v):
        return any(var(q)==v for q in args(s))
    def writes(v):
        return [a for a,s in ins.items() if dest(s) is not None and var(dest(s))==v]
    # Reverse traversal from actual bit-producing instructions, not gauge flags.
    # Restrict direct control-flow edges to valid decoded addresses. Calls have
    # continuation and callee edges; computed jumps are explicitly unresolved.
    def closure(entry):
        visited=set(); todo=[entry]; unresolved=[]
        while todo:
            a=todo.pop()
            if a in visited or a not in ins:continue
            visited.add(a); s=ins[a]; o=op(s); t=target(s,labels); i=idx[a]
            nxt=keys[i+1] if i+1<len(keys) and keys[i+1]-a<=8 else None
            if o in ("ret","reti","rts","rte"):continue
            if o in ("br","ljmp","sjmp","jmp"):
                if t in ins:todo.append(t)
                else:unresolved.append(a)
                continue
            if o.startswith("j") or o in ("djnz","lcall","scall","call"):
                if t in ins:todo.append(t)
                else:unresolved.append(a)
            if nxt is not None:todo.append(nxt)
        return visited,unresolved
    normal_mode=next(c["mode_block"] for c in cs if c["high"]==c["low"]==0xd1)
    normal=[c for c in cs if c["mode_block"]==normal_mode]
    walks={a:closure(a) for a in {c["handler"] for c in cs}}
    def cmds_reaching(addresses,normal_only=False):
        ss=set(addresses); out=[]
        for c in normal if normal_only else cs:
            a=c["handler"]; hit=sorted(ss & walks[a][0])
            if hit:out.append({"high":c["high"],"low":c["low"],"mode":c["mode_block"],"handler":hx(a),"hits":[hx(i) for i in hit]})
        return out
    # Find strategy merges from hardware port stores, regardless of symbol.
    merges=[]
    for a,s in ins.items():
        if op(s)!="stb" or not re.fullmatch(r"(?:0?401h|1ffeh)\[\]",dest(s) or "",re.I):continue
        i=idx[a]; prev=keys[max(0,i-14):i]; storesrc=args(s)[0]
        for k in prev:
            q=args(ins[k]);o=op(ins[k])
            if o!="andb" or len(q)!=2 or q[0]!=storesrc or imm(q[1]) is None:continue
            # This AND is the preserved hardware shadow, not strategy.
            preserve=imm(q[1])&255
            after=[t for t in prev if t>k and op(ins[t])=="orb" and args(ins[t])[:1]==[storesrc]]
            if not after:continue
            mix=args(ins[after[-1]])[1]
            before=[t for t in prev if t<k and op(ins[t])=="andb" and args(ins[t])[:1]==[mix] and len(args(ins[t]))==2 and var(args(ins[t])[1]) is not None]
            if not before:continue
            strat=var(args(ins[before[-1]])[1])
            # Require the two distinct source terms and final OR. Preserve
            # E0 does not disprove writes outside this merge.
            merge={"address":hx(a),"port":dest(s),"strategy":hx(strat),"preserve_mask":preserve,"bit6_preserved_in_shadow":bool(preserve&64),"context":context(a,14,2),"strategy_symbols":[n for n,v in sy.items() if v==strat]}
            producers=[]
            for t in writes(strat):
                q=args(ins[t]); mask=imm(q[1]) if len(q)==2 else None
                if mask is not None and op(ins[t]) in ("orb","andb","xorb"):
                    change=(mask if op(ins[t])!="andb" else ~mask)&255
                    if not change&64:continue
                elif op(ins[t]) in ("orb","andb","xorb","ldb","stb","clrb","notb","ld","st"):
                    pass
                else:continue
                producers.append({"address":hx(t),"instruction":ins[t],"context":context(t,5,7),"normal_command_witnesses":cmds_reaching([t],True)})
            merge["bit6_or_wholebyte_producers"]=producers
            merges.append(merge)
    # All HSO scheduling and dynamic channel writes, not only #02/#22.
    hso=[]
    for a in sorted(set(writes(6)+writes(4))):
        s=ins[a];q=args(s);d=var(dest(s));val=imm(q[1]) if len(q)==2 and d==6 else None
        hso.append({"address":hx(a),"register":hx(d),"constant":val,"channel":val&15 if val is not None else None,"context":context(a,8,9),"normal_command_witnesses":cmds_reaching([a],True)})
    # Independently recover PWM generation via HSO or port. Search each
    # accumulator feeding a carry branch, then physical output consumers.
    pwm=[]
    for a,s in ins.items():
        q=args(s);i=idx[a]
        if op(s)!="addb" or len(q)!=2 or var(q[1]) is None or i+2>=len(keys):continue
        if op(ins[keys[i+1]]) not in ("jc","jnc"):continue
        d=var(q[1]); acc=var(q[0]); after=keys[i+2:i+6]; before=keys[max(0,i-5):i]
        h=[t for t in after if op(ins[t])=="ldb" and args(ins[t])==["INTMEM_6","#22h"]]
        hso0=bool(h and any(op(ins[t])=="ldb" and args(ins[t])==["INTMEM_6","#2"] for t in before))
        ports=[]
        for t in after:
            ss=ins[t]; qq=args(ss)
            if op(ss)!="orb" or len(qq)!=2 or imm(qq[1])!=64:continue
            v=var(qq[0])
            ports.extend(m for m in merges if m["strategy"]==hx(v) and not m["bit6_preserved_in_shadow"] and a<int(m["address"],16)<a+400)
        if not hso0 and not ports:continue
        # All stores to the duty byte; bit-test predecessors include patterns
        # other than the previous adjacent 5-instruction selector.
        dutywriters=[]
        for t in writes(d):
            ctx=context(t,22,3); reads=[]
            for adr,st in ctx[:-1]:
                z=args(st)
                if op(st) in ("jbc","jbs") and len(z)==3 and var(z[0]) is not None and z[1].isdigit():
                    reg,bit=var(z[0]),int(z[1]); set_clear=[]
                    for w in writes(reg):
                        ww=args(ins[w]); mm=imm(ww[1]) if len(ww)==2 else None
                        if mm is not None and op(ins[w]) in ("orb","andb","xorb"):
                            if not ((mm if op(ins[w])!="andb" else ~mm)&(1<<bit)):continue
                        elif op(ins[w]) not in ("stb","ldb","clrb","notb"):continue
                        set_clear.append(w)
                    hits=cmds_reaching(set_clear,True)
                    if hits:reads.append({"address":adr,"flag":hx(reg),"bit":bit,"command_witnesses":hits})
            dutywriters.append({"address":hx(t),"context":ctx,"flag_reads":reads,"normal_command_witnesses":cmds_reaching([t],True)})
        pwm.append({"address":hx(a),"duty":hx(d),"duty_symbols":[n for n,v in sy.items() if v==d],"accumulator":hx(acc) if acc is not None else None,"hso_02_22":hso0,"ports":[m["address"] for m in ports],"duty_writers":dutywriters,"context":context(a,5,8)})
    # Independently audit table mask coverage, plus output references not
    # accounted for by a known strategy merge. No unlabelled output is called
    # an instrument without further semantic/wiring evidence.
    masks=generic_masks(b,ins,cs)
    other=[]
    covered={m["address"] for m in merges}
    for a,s in ins.items():
        if op(s) not in ("st","stb","ldb","ld","orb","andb","xorb"):continue
        dd=dest(s)
        if not dd:continue
        if re.fullmatch(r"(?:0?40[0-9a-f]h|1ff[ef]h)\[\]|IOPORT[12](?:\+\d+)?",dd,re.I) or var(dd) in (0x16,0x17):
            if hx(a) not in covered:other.append({"address":hx(a),"destination":dd,"context":context(a,9,5),"normal_command_witnesses":cmds_reaching([a],True)})
    return {"firmware":r["firmware"],"identity":r["identity"],"rom_sha256":r["sha256"],"known":r["gauge_status"]=="INTEGRE_227","normal_commands":len(normal),"merges":merges,"pwm_output_candidates":pwm,"hso":hso,"other_outputs":other,"generic_masks":masks,"unresolved_command_transfers":[{"handler":hx(a),"addresses":[hx(t) for t in sorted(set(u))]} for a,(v,u) in walks.items() if u],"prior_gauge":prior[r["firmware"]]["labelled_gauge_candidates"],"status":"STATIC_EVIDENCE_REVIEW"}
results=[analyse(r) for r in rows]
def gauge(x):return [p for p in x["pwm_output_candidates"] if "m_tmpgauge_dr" in p["duty_symbols"]]
missing=[x["firmware"] for x in results if x["known"] and not gauge(x)]
# Preserve the findings even if a positive control is missing: fail only
# after saving artifacts (workflow archives results regardless of status).
with gzip.open(O/"preuves_16na_producteurs_lot04.jsonl.gz","wt",encoding="utf-8") as f:
    for x in results:f.write(json.dumps(x,ensure_ascii=False)+"\n")
summary={"firmwares":len(results),"known_recovered":[x["firmware"] for x in results if x["known"] and gauge(x)],"missing_known":missing,"unlabelled_pwm_candidates":[{"firmware":x["firmware"],"pwm":[p for p in x["pwm_output_candidates"] if "m_tmpgauge_dr" not in p["duty_symbols"]]} for x in results if any("m_tmpgauge_dr" not in p["duty_symbols"] for p in x["pwm_output_candidates"])],"new_labelled_gauge":[x["firmware"] for x in results if not x["known"] and gauge(x)],"no_merge":[x["firmware"] for x in results if not x["merges"]],"port_bit6_not_preserved":[x["firmware"] for x in results if any(not m["bit6_preserved_in_shadow"] for m in x["merges"])],"unresolved":[x["firmware"] for x in results if x["unresolved_command_transfers"]],"note":"Conservative graph paths are candidates, not semantic proof. HSO and other output records require separate interpretation. No claim of physical compatibility or exhaustive absence."}
(O/"bilan_16na_producteurs_lot04.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
with (O/"registre_16na_producteurs_lot04.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(["firmware","identity","known","labelled_pwm","unlabelled_pwm","port_merges","bit6_not_preserved","hso_writes","other_outputs","unresolved_handlers","status"])
    for x in results:w.writerow([x["firmware"],x["identity"],x["known"],len(gauge(x)),len(x["pwm_output_candidates"])-len(gauge(x)),len(x["merges"]),sum(not m["bit6_preserved_in_shadow"] for m in x["merges"]),len(x["hso"]),len(x["other_outputs"]),len(x["unresolved_command_transfers"]),x["status"]])
print(json.dumps({k:v for k,v in summary.items() if k!="unlabelled_pwm_candidates"},ensure_ascii=False))
print("Unlabelled PWM:",[x["firmware"] for x in summary["unlabelled_pwm_candidates"]])
assert not missing,missing
