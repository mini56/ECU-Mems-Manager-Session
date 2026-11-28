#!/usr/bin/env python3
"""MEMS 1.6 NA only: output-first inventory, paired flag consumers and restoration.
Read-only analysis; no physical qualification by automatic pattern match."""
import ast,csv,gzip,hashlib,json,re,collections
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive")
O=Path("research/instruments-integral-20261127/results")
# Reuse the audited decoder, without executing its multi-family analysis.
source=Path("research/instruments-integral-20261127/audit_reverse.py")
tree=ast.parse(source.read_text())
names={"hx","read","parse","target","tables","symbols","refs","generic_masks","reachable"}
import bisect
instpat=re.compile(r"tacho|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tempg_pwm|tpg_force|tpg_onoff|instrument[_ ]cluster",re.I)
diagpat=re.compile(r"kp_.*(?:control|override)|diagnostic.*override|actuator",re.I)
for node in tree.body:
    if isinstance(node,ast.FunctionDef) and node.name in names:
        exec(compile(ast.Module(body=[node],type_ignores=[]),str(source),"exec"))
rows=[r for r in csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")) if r["family"]=="Rover MEMS 1.6"]
assert len(rows)==123 and len({r["firmware"] for r in rows})==123
def var(s):
    m=re.fullmatch(r"(?:\(?)INTMEM_([0-9a-f]+)(?:\+(\d+))?(?:\))?(?:\[\])?",s.strip(),re.I)
    return int(m[1],16)+int(m[2] or 0) if m else None
def imm(s):
    m=re.fullmatch(r"#(-?)([0-9a-f]+)(h?)",s.strip(),re.I)
    if not m:return None
    try:return (-1 if m[1] else 1)*int(m[2],16 if m[3] else 10)
    except ValueError:return None
def op(s):return s.split()[0].lower()
def args(s):return [x.strip() for x in s.split(None,1)[1].split(",")] if len(s.split(None,1))>1 else []
def onebit(x):return x.bit_length()-1 if x and x>0 and x&(x-1)==0 else None
def xfmap(xf):
    d={}
    for l in xf.splitlines():
        m=re.match(r"\s*([\w\[\]]+)\s+([0-9a-f]{2});([0-9a-f]{2});",l,re.I)
        if m:d[m[1].lower()]=int(m[2]+m[3],16)
        m=re.match(r"\s*([\w\[\]]+)\s*,\s*([0-9a-f]{4})\s*,\s*(?:S?[BWL]|DEAD)\s*,",l,re.I)
        if m:d[m[1].lower()]=int(m[2],16)
    return d
def analyse(r):
    p=R/r["path"]; b=p.read_bytes(); assert hashlib.sha256(b).hexdigest()==r["sha256"],p
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read(siblings.get(".asm",p.with_suffix(".asm")));xf=read(siblings.get(".xf3",siblings.get(".xf1",p.with_suffix(".xf1"))))
    ins,labels,_=parse(asm); assert ins, p
    cs=tables(b); assert cs,p
    keys=sorted(ins); pos={a:i for i,a in enumerate(keys)}
    def seq(a,before=0,after=10):
        i=pos[a];return [[hx(k),ins[k]] for k in keys[max(0,i-before):i+after]]
    def window(a,n=30):
        return [[hx(k),ins[k]] for k in keys if a<=k<a+n]
    sy=xfmap(xf); symbols0=symbols(xf); refs0=refs(ins,symbols0,"80196")
    # Start with real output registers, not names or opcode numbers.
    # Include aliases used by IDA for 80196 HSO and I/O registers.
    hw=[]
    for a,s in ins.items():
        if op(s) not in ("st","stb","ld","ldb","orb","andb","xorb","add","addb","sub","subb","clrb","clr","incb","decb"):continue
        aa=args(s)
        if not aa:continue
        dest=aa[1] if op(s) in ("st","stb") and len(aa)>1 else aa[0]
        if re.search(r"^(?:0?40[0-9a-f]h|1ff[ef]h)\[\]$|^(?:IOPORT[12]|HSO_[A-Z]+|INT_PEND)(?:\+\d+)?$",dest,re.I) or var(dest) in (4,5,6,7,0x16,0x17):
            hw.append({"address":hx(a),"destination":dest,"context":seq(a,10,4)})
    # Search every pair of adjacent bit tests selecting 0/255 and writing a
    # byte. No preset flag register, variable address or command opcode.
    consumers=[]
    for i in range(len(keys)-4):
        a0,a1,a2,a3,a4=keys[i:i+5]
        s0,s1,s2,s3,s4=[ins[k] for k in (a0,a1,a2,a3,a4)]
        if [op(s) for s in (s0,s1,s2,s3,s4)]!=["jbc","clrb","jbc","decb","stb"]:continue
        x,y,z,w,v=map(args,(s0,s1,s2,s3,s4))
        if len(x)!=3 or len(z)!=3 or len(v)!=2 or y!=w or y!=v[:1]:continue
        if target(s0,labels)!=a4 or target(s2,labels)!=a4:continue
        flag0,flag1=var(x[0]),var(z[0]); d=var(v[1])
        if None in (flag0,flag1,d) or not x[1].isdigit() or not z[1].isdigit():continue
        bit0,bit1=int(x[1]),int(z[1])
        # PWM duty consumers: accumulate into byte, use carry to select output.
        pwm=[]
        for a,s in ins.items():
            q=args(s)
            if op(s)!="addb" or len(q)!=2 or var(q[1])!=d:continue
            ctx=seq(a,4,12); following=[line for _,line in ctx[4:]]
            if len(following)<3 or op(following[1]) not in ("jnc","jc"):continue
            pwm.append({"address":hx(a),"accumulator":q[0],"context":ctx})
        # All actual dialogue handlers that write either bit, across modes.
        commands=[]
        for c in cs:
            evidence=[]
            for a0s,s in window(c["handler"],24):
                q=args(s)
                if len(q)!=2 or op(s) not in ("orb","andb"):continue
                vv=imm(q[1]); reg=var(q[0])
                if vv is None:continue
                mask=vv if op(s)=="orb" else ~vv
                if (reg==flag0 and mask&(1<<bit0)) or (reg==flag1 and mask&(1<<bit1)):
                    evidence.append([a0s,s])
            if evidence:commands.append({**c,"handler":hx(c["handler"]),"evidence":evidence})
        # Restrict restoration seeds to actual bit-clearing instructions, then
        # reverse-traverse explicit calls/branches from all dialogue entries.
        clear=[]
        for a,s in ins.items():
            q=args(s)
            if op(s)=="andb" and len(q)==2 and var(q[0])==flag0:
                vv=imm(q[1])
                if vv is not None and not vv&(1<<bit0):clear.append(a)
        restor=reachable(ins,labels,[c for c in cs if c["high"]==c["low"]==0xf4],set(clear))
        consumers.append({"address":hx(a0),"flag":hx(flag0),"flag_bit":bit0,"direction_flag":hx(flag1),"direction_bit":bit1,"duty":hx(d),"duty_symbol":[k for k,v in sy.items() if v==d],"consumer":seq(a0,3,8),"pwm":pwm,"command_candidates":commands,"f4_clear_witnesses":restor})
    # Identify explicit electrical routing of the PWM candidate: direct HSO
    # command versus port bit-6 strategy with a subsequent output store.
    for c in consumers:
        for w in c["pwm"]:
            a=int(w["address"],16); ctx=seq(a,5,95); w["output_context"]=ctx
            # Positive examples: HSO command 02/22 then event time;
            # or set bit 6 in strategy then merge into actual 0401 port.
            hso=False; port=False
            for j,(_,s) in enumerate(ctx):
                q=args(s)
                if op(s)=="ldb" and len(q)==2 and var(q[0])==6 and imm(q[1])==0x22:
                    hso=any(op(t)=="add" and args(t) and var(args(t)[0])==4 for _,t in ctx[j+1:j+5])
                if op(s)=="orb" and len(q)==2 and var(q[0])==sy.get("io_strat_req") and imm(q[1])==64:
                    port=any(op(t)=="stb" and len(args(t))==2 and re.fullmatch(r"0?401h\[\]",args(t)[1],re.I) for _,t in ctx[j+1:j+65])
            w["hso_02_22_candidate"]=hso;w["port_0401_bit6_candidate"]=port
    masks=generic_masks(b,ins,cs)
    # Preserve every command entry, not only known commands, with actual
    # handler instructions, and explicit output/control-flow witnesses.
    output_seeds={int(h["address"],16) for h in hw}
    command_output=reachable(ins,labels,cs,output_seeds)
    commands=[{**c,"handler":hx(c["handler"]),"entry":window(c["handler"],35)} for c in cs]
    gauge=[c for c in consumers if c["pwm"] and any(w["hso_02_22_candidate"] or w["port_0401_bit6_candidate"] for w in c["pwm"])]
    labelled=[c for c in gauge if "m_tmpgauge_dr" in c["duty_symbol"]]
    # Preserve all indirect addressing and unresolved branches for later review.
    indirect=[[hx(a),s] for a,s in ins.items() if re.search(r"\[(?:INTMEM_[0-9a-f]+(?:\+\d+)?|[A-Z][A-Z0-9_]+)\]",s,re.I)]
    tacho=[s for s in symbols0 if re.search(r"tacho|rev.?counter",s["name"],re.I)]
    return {"firmware":r["firmware"],"identity":r["identity"],"rom_sha256":r["sha256"],"asm_sha256":hashlib.sha256(asm.encode()).hexdigest(),"instructions":len(ins),"known_gauge":r["gauge_status"]=="INTEGRE_227","instrument_symbols":symbols0,"instrument_refs":refs0,"xf_io_symbols":{k:hx(v) for k,v in sy.items() if re.search(r"gauge|tacho|io_forc|io_strat|pwm|fault_mask",k)},"all_commands":commands,"hardware_writes":hw,"command_to_hardware_witnesses":command_output,"generic_masks":masks,"all_flag_consumers":consumers,"gauge_output_candidates":gauge,"labelled_gauge_candidates":len(labelled),"tacho_symbols":tacho,"indirect_addressing":indirect,"qualification":"REVIEW_REQUIRED"}
results=[analyse(r) for r in rows]
for n in ("AANMP002","EKDMF009"):
    x=next(x for x in results if x["firmware"]==n)
    assert x["labelled_gauge_candidates"], (n,x["all_flag_consumers"])
assert len(results)==123
with gzip.open(O/"preuves_16na_sorties_lot02.jsonl.gz","wt",encoding="utf-8") as f:
    for x in results:f.write(json.dumps(x,ensure_ascii=False)+"\n")
cols=["firmware","identity","known_gauge","instructions","dialogue_entries","hardware_writes","command_to_hardware_witnesses","generic_bit6","flag_consumers","gauge_output_candidates","labelled_gauge_candidates","tacho_symbols","qualification"]
with (O/"registre_16na_sorties_lot02.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(cols)
    for x in results:w.writerow([x["firmware"],x["identity"],x["known_gauge"],x["instructions"],len(x["all_commands"]),len(x["hardware_writes"]),len(x["command_to_hardware_witnesses"]),any(m["bit6_reachable"] for m in x["generic_masks"]),len(x["all_flag_consumers"]),len(x["gauge_output_candidates"]),x["labelled_gauge_candidates"],len(x["tacho_symbols"]),x["qualification"]])
summary={"firmwares":len(results),"known_gauge":sum(x["known_gauge"] for x in results),"known_gauge_recovered":[x["firmware"] for x in results if x["known_gauge"] and x["labelled_gauge_candidates"]],"new_gauge_candidates":[{"firmware":x["firmware"],"identity":x["identity"],"consumers":x["gauge_output_candidates"]} for x in results if not x["known_gauge"] and x["gauge_output_candidates"]],"generic_bit6":[x["firmware"] for x in results if any(m["bit6_reachable"] for m in x["generic_masks"])],"tacho_symbols":[x["firmware"] for x in results if x["tacho_symbols"]],"note":"Output candidates and reachability are not safe integration qualification; review exact command, calibration, identity and restoration. All 123 retained."}
(O/"bilan_16na_sorties_lot02.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False),encoding="utf-8")
print(json.dumps({k:v for k,v in summary.items() if k!="new_gauge_candidates"},ensure_ascii=False))
print("New candidates:",[x["firmware"] for x in summary["new_gauge_candidates"]])
