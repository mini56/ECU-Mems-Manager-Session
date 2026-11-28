#!/usr/bin/env python3
"""Explain each existing V2 witness, exact generic masks, and periodic output candidates.
Exclusive 1.6 NA, read-only. No profile inferred from reachability."""
import ast,csv,gzip,hashlib,json,re,collections
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive")
O=Path("research/instruments-integral-20261127/results")
instpat=re.compile(r"tacho|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tempg_pwm|tpg_force|tpg_onoff|instrument[_ ]cluster",re.I)
diagpat=re.compile(r"kp_.*(?:control|override)|diagnostic.*override|actuator",re.I)
def load(path,names):
    for n in ast.parse(Path(path).read_text()).body:
        if isinstance(n,ast.FunctionDef) and n.name in names:
            exec(compile(ast.Module(body=[n],type_ignores=[]),path,"exec"),globals())
load("research/instruments-integral-20261127/audit_reverse.py",{"hx","read","parse","target","tables","symbols","reachable"})
load("research/instruments-integral-20261127/audit_16na_outputs.py",{"var","imm","op","args","xfmap"})
def readjsonl(p):
    with gzip.open(O/p,"rt",encoding="utf-8") as f:return list(map(json.loads,f))
v2={r["firmware"]:r for r in readjsonl("preuves_recherche_inverse_v2_20261128.jsonl.gz") if r["family"]=="Rover MEMS 1.6"}
p3={r["firmware"]:r for r in readjsonl("preuves_16na_sorties_lot03.jsonl.gz")}
p5={r["firmware"]:r for r in readjsonl("preuves_16na_confrontation_lot05.jsonl.gz")}
rows=[r for r in csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")) if r["family"]=="Rover MEMS 1.6"]
assert len(rows)==len(v2)==len(p3)==len(p5)==123
wordops={"ld","st","and","or","xor","cmp","add","sub","not","clr","push","pop","inc","dec","shl","shr","mulu","divu"}
byteops={"ldb","stb","andb","orb","xorb","cmpb","addb","subb","notb","clrb","incb","decb","shlb","shrb","ldbze","ldbs"}
def dest(s):
    q=args(s);o=op(s)
    if not q or o not in (wordops|byteops)-{"cmp","cmpb","push","mulu","divu"}:return None
    return q[1] if o in ("st","stb") and len(q)>1 else q[0]
def access(s,reg,bit):
    o=op(s);q=args(s); d=dest(s); dr=var(d) if d else None
    if o in ("jbc","jbs") and len(q)==3 and var(q[0])==reg and q[1]==str(bit):return "BIT_TEST"
    if o in ("orb","andb","xorb") and dr==reg and len(q)==2 and imm(q[1]) is not None:
        m=imm(q[1]); change=(m if o!="andb" else ~m)&255
        return "SET" if o=="orb" and change&(1<<bit) else "CLEAR" if o=="andb" and change&(1<<bit) else "TOGGLE" if o=="xorb" and change&(1<<bit) else "OTHER_BIT"
    if dr==reg or o in wordops and dr is not None and dr+1==reg:return "WHOLE_WRITE"
    if any(var(x)==reg or o in wordops and var(x) is not None and var(x)+1==reg for x in q):return "READ_OR_TRANSFER"
    return "UNRELATED"
def analyse(r):
    n=r["firmware"];p=R/r["path"];b=p.read_bytes();assert hashlib.sha256(b).hexdigest()==r["sha256"],n
    sib={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    ins,labels,_=parse(read(sib.get(".asm",p.with_suffix(".asm"))))
    xf=read(sib.get(".xf3",sib.get(".xf1",p.with_suffix(".xf1")))); sy=xfmap(xf)
    keys=sorted(ins);ix={a:i for i,a in enumerate(keys)}
    cs=tables(b);mode=next(c["mode_block"] for c in cs if c["high"]==c["low"]==0xd1)
    norm=[c for c in cs if c["mode_block"]==mode]
    def ctx(a,before=4,after=10):
        i=ix[a];return [[hx(k),ins[k]] for k in keys[max(0,i-before):i+after]]
    def raw(a,k=4):return b[a-8192:a-8192+k].hex(" ")
    def straight(entry):
        # Follow unconditional branch instructions; stop at conditional/indirect
        # transfers and calls, not at the adjacent command's fallthrough.
        result=[];seen=set();a=entry
        while a in ins and a not in seen and len(result)<60:
            seen.add(a);s=ins[a];result.append([hx(a),s]);o=op(s)
            if o in ("ljmp","sjmp","jmp","br"):
                a=target(s,labels)
                if a is None:break
                continue
            if o.startswith("j") or o in ("lcall","scall","call","ret","reti","djnz"):break
            i=ix[a]
            if i+1>=len(keys) or keys[i+1]-a>8:break
            a=keys[i+1]
        return result
    flags=[s for s in symbols(xf) if s["name"] in ("d_tpg_force","d_tpg_onoff") and s["address"] is not None and s["bit"] is not None]
    paths=[]
    for c in v2[n]["new_opcode_candidates"]:
        w=c.get("witness_path",[])
        end=w[-1] if w else ["",""]
        classification=[{"name":f["name"],"access":access(end[1],f["address"],f["bit"])} for f in flags if end[1]]
        paths.append({"command":[c["high"],c["low"]],"mode":c["mode_block"],"handler":c["handler"],"terminal":end,"terminal_classification":classification,"call_sites":[x for x in w if op(x[1]) in ("lcall","scall","call")],"witness":w})
    direct=[]
    for c in norm:
        if c["high"]!=c["low"] or c["high"] not in (0,16,99,107,244):continue
        w=straight(c["handler"])
        direct.append({"command":c["high"],"handler":hx(c["handler"]),"path":w,"flag_accesses":[{"flag":f["name"],"address":a,"access":k} for a,s in w for f in flags if (k:=access(s,f["address"],f["bit"])) not in ("UNRELATED","OTHER_BIT")]})
    # Identify the actual index AND #0F, table read and both flag stores.
    # Decode all supported command ranges rather than guessing table extent.
    generic=[]
    for i,a in enumerate(keys[:-18]):
        s=ins[a];q=args(s)
        if op(s)!="andb" or len(q)!=3 or imm(q[2])!=15:continue
        add=ins[keys[i+1]];z=args(add)
        if op(add)!="add" or len(z)!=3 or z[1]!=q[0] or imm(z[2]) is None:continue
        base=imm(z[2]);off=base-8192
        if not 0<=off<=len(b)-16:continue
        w=keys[max(0,i-1):i+18];lines=[ins[k] for k in w]
        if not any(op(x)=="notb" for x in lines) or sum(op(x)=="stb" for x in lines)<2:continue
        if not any(op(x)=="ldb" and len(args(x))==2 and args(x)[1]=="["+z[0]+"]" for x in lines):continue
        entries=[c for c in norm if keys[max(0,i-1)]<=c["handler"]<=a]
        if not entries:continue
        masks=list(b[off:off+16]); actual=[]
        for c in entries:
            for cmd in range(min(c["high"],c["low"]),max(c["high"],c["low"])+1):actual.append([cmd,masks[cmd&15]])
        generic.append({"entry":hx(a),"base":hx(base),"masks":masks,"supported_command_masks":actual,"supported_bit6_commands":[cmd for cmd,m in actual if m&64],"code":ctx(a,1,15)})
    # A periodic square wave may use a shadow XOR or HSO rather than
    # a named tacho symbol. Search every XOR and port writer, retaining
    # actual local sequences and timer arithmetic as evidence to review.
    # Distinguish XOR of a RAM shadow from direct I/O.
    periodic=[]
    for i,a in enumerate(keys):
        s=ins[a];q=args(s)
        if op(s)!="xorb" or len(q)!=2:continue
        mask=imm(q[1])
        if mask is None or (mask&255)==0 or (mask&255)&((mask&255)-1):continue
        w=keys[max(0,i-12):i+20]
        outs=[k for k in w if op(ins[k]) in ("stb","st") and len(args(ins[k]))==2 and re.fullmatch(r"(?:0?40[0-9a-f]h|1ff[ef]h)\[\]|IOPORT[12]",args(ins[k])[1],re.I)]
        if not outs:continue
        # Require direct same temporary to output or same operand in
        # the shadow load following the XOR; otherwise label as unrelated proximity.
        tied=[k for k in outs if args(ins[k])[0]==q[0]]
        if not tied:continue
        periodic.append({"xor":hx(a),"mask":mask&255,"output_stores":[hx(k) for k in tied],"context":ctx(a,12,20),"raw":raw(a)})
    # Candidate signal generation via HSO on/off pairs, irrespective of
    # channel. List local scheduling sequence only when both polarities occur.
    hso=[]
    for i,a in enumerate(keys):
        s=ins[a];q=args(s)
        if op(s)!="ldb" or len(q)!=2 or var(q[0])!=6 or imm(q[1]) is None:continue
        x=imm(q[1])&255
        for k in keys[i+1:i+18]:
            z=ins[k];v=args(z)
            if op(z)!="ldb" or len(v)!=2 or var(v[0])!=6 or imm(v[1]) is None:continue
            y=imm(v[1])&255
            if x^y==0x20:
                hso.append({"first":hx(a),"second":hx(k),"channel":x&15,"context":ctx(a,6,24)})
                break
    return {"firmware":n,"identity":r["identity"],"sha256":r["sha256"],"known":r["gauge_status"]=="INTEGRE_227","known_recovered":bool(p3[n]["labelled_gauge_candidates"]),"v2_paths":paths,"exact_commands":direct,"generic":generic,"periodic_xor_candidates":periodic,"hso_polarity_pairs":hso,"unresolved_computed_transfers":p5[n]["computed_transfers"],"status":"REVIEW_REQUIRED"}
res=[analyse(r) for r in rows]
assert len(res)==123 and sum(x["known"] and x["known_recovered"] for x in res)==15
assert sum(bool(x["v2_paths"]) and not x["known"] for x in res)==104
with gzip.open(O/"preuves_16na_commandes_reelles_lot07.jsonl.gz","wt",encoding="utf-8") as f:
    for x in res:f.write(json.dumps(x,ensure_ascii=False)+"\n")
summary={"firmwares":len(res),"known_recovered":sum(x["known"] and x["known_recovered"] for x in res),"previous_paths":sum(len(x["v2_paths"]) for x in res if not x["known"]),"terminal_classes":dict(collections.Counter(c["access"] for x in res if not x["known"] for p in x["v2_paths"] for c in p["terminal_classification"])),"generic_tables":sum(bool(x["generic"]) for x in res),"generic_bit6":[x["firmware"] for x in res if any(g["supported_bit6_commands"] for g in x["generic"])],"periodic_xor":[{"firmware":x["firmware"],"candidates":x["periodic_xor_candidates"]} for x in res if x["periodic_xor_candidates"]],"hso_channel_counts":dict(collections.Counter(str(z["channel"]) for x in res for z in x["hso_polarity_pairs"])),"limitations":"Exact masks and witness terminal semantics only. No physical qualification. Other periodic or computed paths remain unresolved."}
(O/"bilan_16na_commandes_reelles_lot07.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2))
with (O/"registre_16na_commandes_reelles_lot07.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(["firmware","identity","known","v2_paths","v2_terminal_classes","generic_bases","generic_bit6","periodic_xor","hso_channels","unresolved_computed"])
    for x in res:w.writerow([x["firmware"],x["identity"],x["known"],len(x["v2_paths"]),json.dumps(dict(collections.Counter(c["access"] for p in x["v2_paths"] for c in p["terminal_classification"]))),";".join(g["base"] for g in x["generic"]),";".join(str(c) for g in x["generic"] for c in g["supported_bit6_commands"]),len(x["periodic_xor_candidates"]),";".join(str(c) for c in sorted(set(z["channel"] for z in x["hso_polarity_pairs"]))),len(x["unresolved_computed_transfers"])])
print(json.dumps({k:v for k,v in summary.items() if k!="periodic_xor"},ensure_ascii=False))
print("XOR candidates:",[x["firmware"] for x in summary["periodic_xor"]])
