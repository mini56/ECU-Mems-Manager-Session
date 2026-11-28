#!/usr/bin/env python3
"""Output-first experiment: 1.6 NA and 1.9 only. Static evidence, never ECU access."""
import ast,collections,csv,gzip,json,re,hashlib
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive")
O=Path("research/instruments-integral-20261127/results")
# Reuse only the assembler decoder; no prior detection logic.
p=O.parent/"audit_16na_asm_complete.py"
for node in ast.parse(p.read_text()).body:
    if isinstance(node,ast.FunctionDef) and node.name in ("num","reg","parse"):
        exec(compile(ast.Module(body=[node],type_ignores=[]),str(p),"exec"))
def rr(s):
    s=s.strip()
    if s.upper()=="ZERO_REG":return 0
    if s.upper()=="IOPORT1" or s.upper()=="IOPORT1[]":return 0x0f
    if s.upper()=="IOPORT2" or s.upper()=="IOPORT2[]":return 0x10
    return reg(s)
def hx(x):return f"{x:04X}"
def imm(s):
    return num(s) if s.startswith("#") else None
def physical(s):
    a=rr(s)
    if a in (4,6,0x0f,0x10,0x16,0x17):return hx(a)
    if re.fullmatch(r"(?:0?40[0-9a-f]h|1ff[ef]h)\[\]",s,re.I):return s
    return None
stores={"st","stb"}
mut={"ld","ldb","ldbze","ldbs","add","addb","addc","addcb","sub","subb","subc","subcb","and","andb","or","orb","xor","xorb","clr","clrb","inc","incb","dec","decb","not","notb","neg","negb","shl","shlb","shr","shrb","mulu","mul","divu","div"}
word={"ld","st","add","addc","sub","subc","and","or","xor","clr","inc","dec","not","neg","shl","shr","mulu","mul","divu","div","ldbze","ldbs"}
def dest(o,q):
    if o in stores:return q[1] if len(q)==2 else ""
    return q[0] if o in mut and q else ""
def source(o,q):
    if o in stores:return q[:1]
    if o in mut:
        if o in ("ld","ldb","ldbze","ldbs","clr","clrb"):return q[1:]
        return q if len(q)<=2 else q[1:]
    return q
def table(b,db):
    def byte(a):
        if b is not None and 0x2000<=a<0x2000+len(b):return b[a-0x2000]
        return db.get(a)
    starts=([i+0x2000 for i in range(len(b)-1) if b[i:i+2]==b"\xd0\xd0"] if b is not None else [a for a,v in db.items() if v==0xd0 and db.get(a+1)==0xd0])
    ts=[]
    for a in starts:
        es=[]
        for k in range(a,a+1200,4):
            q=[byte(k+i) for i in range(4)]
            if None in q:break
            h,l,x,y=q;t=x+256*y
            if not 0x2000<=t<0xA000:break
            es.append((k,h,l,t))
            if (h,l)==(0x7f,0):break
        if len(es)>=15 and es[-1][1:3]==(0x7f,0) and {0xd0,0xd1,0xf4}<={h for k,h,l,t in es if h==l}:ts.append(es)
    return max(ts,key=len,default=[])
def analyse(p,family,known_gauge,known_tacho):
    b=p.with_suffix(".hex").read_bytes() if p.with_suffix(".hex").exists() else None
    ins,db=parse(p.read_text(errors="replace")); es=table(b,db)
    if es:
        ins={a:v for a,v in ins.items() if not es[0][0]<=a<es[-1][0]+4}
    keys=sorted(ins);ix={a:i for i,a in enumerate(keys)}
    def ctx(i,lo=12,hi=12):return keys[max(0,i-lo):i+hi+1]
    def evidence(addrs):return [[hx(a),ins[a][2]] for a in sorted(set(addrs)) if a in ins]
    def d(a):return dest(*ins[a][:2])
    def rd(a):return {r for q in source(*ins[a][:2]) if (r:=rr(q)) is not None}
    def target(q):
        m=re.fullmatch(r"(?:ROM|sub|loc|locret)_([0-9a-f]+)",q[-1],re.I) if q else None
        return int(m[1],16) if m else None
    # Index real destination bytes, including word aliases.
    writers=collections.defaultdict(set)
    for a,(o,q,s) in ins.items():
        r=rr(d(a))
        if r is not None:
            writers[r].add(a)
            if o in word:writers[r+1].add(a)
    outputs=[a for a in keys if physical(d(a))]
    ports=[a for a in outputs if ins[a][0] in stores and physical(d(a)) not in ("0004","0006")]
    port_sources=collections.defaultdict(list)
    for a in ports:
        o,q,s=ins[a]; r=rr(q[0])
        if r is not None:port_sources[r].append(a)
        # Capture shadow/strategy merges. Distinguish direct source and
        # input registers of the preceding computation, not fixed bits.
        for k in ctx(ix[a],14,0):
            if ins[k][0] in ("orb","or","andb","and"):
                for r in rd(k):port_sources[r].append(a)
    gens=[]
    def addgen(kind,a,outs,regs,ev,details):
        gens.append({"kind":kind,"anchor":hx(a),"outputs":[hx(k) for k in sorted(set(outs))],"input_registers":sorted(r for r in regs if r>=0x40),"signal_evidence":evidence(ev),"details":details})
    # Start at each port write. Detect a state inversion and advancement
    # of the same deadline word, no fixed port bit or command number.
    for a in ports:
        i=ix[a]; q=ins[a][1]; shadow=rr(q[0])
        if shadow is None:continue
        prev=ctx(i,10,0)[:-1]; following=ctx(i,0,12)[1:]
        changes=[k for k in prev if rr(d(k))==shadow and ins[k][0] in ("xor","xorb","not","notb") and not any(rr(d(j))==shadow for j in prev if j>k)]
        if not changes:continue
        deadlines=[]
        for k in following:
            o,qk,_=ins[k]
            if o not in ("add","addc","addb") or len(qk)<2:continue
            r=rr(qk[0]); timers=rd(k)-{r}
            for j in following:
                if j<=k or ins[j][0] not in stores or rr(ins[j][1][0])!=r:continue
                if rr(d(j)) in timers:deadlines.append((k,j,rr(d(j))))
        if deadlines:
            local=ctx(i,12,12)
            inputs=set()
            for k in local:
                if ins[k][0] in ("ld","ldb","cmp","cmpb","andb","jbc","jbs"):
                    inputs|=rd(k)
            addgen("PERIODIC_PORT",a,[a],inputs,local,{"shadow":shadow,"changes":evidence(changes),"deadlines":[[hx(k),hx(j),hx(t)] for k,j,t in deadlines]})
        else:
            addgen("PORT_INVERSION_UNRESOLVED",a,[a],set(),ctx(i,8,6),{})
    # From each HSO/port sink backwards to the arithmetic producer.
    # Detect carry modulation, arbitrary channel/mask, including three operands.
    for i,a in enumerate(keys):
        o,q,s=ins[a]
        if o!="addb" or len(q)<2 or i+1>=len(keys) or ins[keys[i+1]][0] not in ("jc","jnc"):continue
        duty=rr(q[-1])
        if duty is None:continue
        before=ctx(i,7,0)[:-1]; after=ctx(i,0,9)[1:]
        hs=[]
        for k in before:
            if rr(d(k))!=6 or len(ins[k][1])!=2:continue
            v=imm(ins[k][1][1])
            if v is None:continue
            for j in after:
                if rr(d(j))!=6 or len(ins[j][1])!=2:continue
                w=imm(ins[j][1][1])
                if w is not None and (v^w)==0x20:
                    times=[t for t in after if t>j and rr(d(t))==4]
                    if times:hs.extend([k,j,*times])
        if hs:addgen("CARRY_HSO",a,hs,{duty},ctx(i,7,9),{"duty":duty})
        for k in after[:5]:
            oo,qq,_=ins[k]; r=rr(d(k))
            if oo not in ("orb","andb","xorb") or r is None or len(qq)!=2 or imm(qq[1]) is None:continue
            mask=imm(qq[1])&255
            if oo=="andb":mask=(~mask)&255
            if not mask:continue
            paired=[]
            for j in before:
                oj,qj,_=ins[j]
                if rr(d(j))!=r or len(qj)!=2 or imm(qj[1]) is None:continue
                m=imm(qj[1])&255
                if oj=="andb" and ((~m)&255)&mask or oj=="orb" and m&mask:paired.append(j)
            pp=[t for t in port_sources.get(r,[]) if k<=t<k+600]
            if pp:
                addgen("CARRY_PORT",a,pp,{duty},ctx(i,7,9)+[j for t in pp for j in ctx(ix[t],14,2)],{"duty":duty,"strategy":r,"mask":mask,"paired":evidence(paired)})
    # Collect the control dependencies of duty/period producers. Follow the
    # producer's writes back to tested flags. Include actual write sites, not
    # just references to a symbol. Diagnostic roots enumerated only now.
    relevant=set()
    for g in gens:
        g["control_registers"]=[]
        rr0=set(g["input_registers"]); flags=set()
        for r in rr0:
            for w in writers[r]:
                for k in ctx(ix[w],12,0):
                    if ins[k][0] in ("jbc","jbs"):
                        flags|={v for v in rd(k) if v>=0x40}
        g["control_registers"]=sorted(flags-rr0)
        # Separate direct producers from one-hop gating flags.
        g["input_writers"]={hx(r):evidence(writers[r]) for r in sorted(rr0)}
        g["gate_writers"]={hx(r):evidence(writers[r]) for r in sorted(flags-rr0)}
        relevant|={a for r in rr0|flags for a in writers[r]}
    # Resolve forward control flow from EVERY diagnostic entry. No test opcode
    # filter. Reachability is conservative (calls may return; computed jumps
    # are unresolved), and is not a proof of semantic ownership.
    edges={}; unresolved=set()
    for i,a in enumerate(keys):
        o,q,s=ins[a]; t=target(q); ee=[]
        if o not in ("ret","reti","rts","rte"):
            branch=o.startswith("j") or o in ("djnz","br","lcall","scall","call","ljmp","sjmp")
            if branch:
                if t in ins:ee.append(t)
                else:unresolved.add(a)
            if o not in ("br","ljmp","sjmp","jmp") and i+1<len(keys) and keys[i+1]-a<=8:ee.append(keys[i+1])
        edges[a]=ee
    links=collections.defaultdict(list); mode=0
    normal=None
    entries=[]
    for k,h,l,t in es:
        if h==l==0xd0:mode+=1
        if h==l==0xd1:normal=mode
        if (h,l)!=(0x7f,0):entries.append((k,h,l,t,mode))
    closure={}
    for k,h,l,t,m in entries:
        if t not in closure:
            seen=set();todo=[t]
            while todo:
                z=todo.pop()
                if z in seen or z not in edges:continue
                seen.add(z);todo.extend(edges[z])
            closure[t]=seen&relevant
        for a in closure[t]:links[a].append([hx(k),h,l,hx(t),m])
    for g in gens:
        ls=[]
        for group in ("input_writers","gate_writers"):
            for r,ws in g[group].items():
                for adr,st in ws:
                    for cmd in links[int(adr,16)]:
                        ls.append({"register":r,"write":adr,"instruction":st,"dependency":group,"entry":cmd,"normal":cmd[-1]==normal})
        g["diagnostic_links"]=ls
    # Only the final comparison uses the historical opcodes and labels.
    def controls(kinds,pair):
        return any(g["kind"] in kinds and set(pair)<={x["entry"][1] for x in g["diagnostic_links"] if x["normal"] and x["entry"][1]==x["entry"][2]} for g in gens)
    return {"family":family,"firmware":p.stem,"rom":b is not None,"asm_sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"rom_sha256":hashlib.sha256(b).hexdigest() if b is not None else None,"known_gauge":known_gauge,"known_tacho":known_tacho,"gauge_control_recovered":controls({"CARRY_HSO","CARRY_PORT"},[0,0x10]),"tacho_control_recovered":controls({"PERIODIC_PORT"},[0x63,0x6b]),"instruction_count":len(ins),"hardware_outputs":evidence(outputs),"diagnostic_entries":entries,"unresolved_transfers":len(unresolved),"generators":gens}
rows=list(csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")))
known={(x["family"],x["firmware"]):(x["gauge_status"]=="INTEGRE_227",x["tach_status"]=="INTEGRE_227") for x in rows}
families=[("Rover MEMS 1.6",R/"Rover MEMS 1.6/NA",125,123),("Rover MEMS 1.9",R/"Rover MEMS 1.9",217,215)]
results=[]
with gzip.open(O/"preuves_sorties_physiques_lot16.jsonl.gz","wt",encoding="utf-8") as f:
    for family,path,na,nr in families:
        ps=sorted(path.glob("*.asm"))
        assert len(ps)==na and sum(p.with_suffix(".hex").exists() for p in ps)==nr,(family,len(ps))
        for p in ps:
            row=analyse(p,family,*known.get((family,p.stem),(False,False)))
            f.write(json.dumps(row,ensure_ascii=False)+"\n")
            row["generator_counts"]=dict(collections.Counter(g["kind"] for g in row["generators"]))
            row["unqualified_linked_generators"]=[{"kind":g["kind"],"anchor":g["anchor"],"outputs":g["outputs"],"commands":sorted({x["entry"][1] for x in g["diagnostic_links"] if x["normal"] and x["entry"][1]==x["entry"][2]})} for g in row["generators"] if g["kind"]!="PORT_INVERSION_UNRESOLVED" and any(x["normal"] for x in g["diagnostic_links"])]
            for k in ("generators","hardware_outputs","diagnostic_entries"):row.pop(k)
            results.append(row)
        print(family,na,"completed",flush=True)
controls=[r for r in results if r["known_gauge"] or r["known_tacho"]]
assert len(controls)==56
missing=[r["firmware"] for r in controls if not ((r["known_gauge"] and r["gauge_control_recovered"]) or (r["known_tacho"] and r["tacho_control_recovered"]))]
summary={"asm":len(results),"rom":sum(r["rom"] for r in results),"known_controls":len(controls),"recovered":len(controls)-len(missing),"missing":missing,"orphans":[r["firmware"] for r in results if not r["rom"]],"method_status":"CONTROL_GATE_PASSED" if not missing else "METHOD_NOT_VALIDATED","new_integration_authorized":0,"note":"Linked generators are unqualified static leads; no opcode or symbol required for discovery. Other output architectures and unresolved transfers remain open.","families":{}}
for family,_,_,_ in families:
    rs=[r for r in results if r["family"]==family]
    summary["families"][family]={"asm":len(rs),"rom":sum(r["rom"] for r in rs),"gauge_controls_recovered":sum(r["known_gauge"] and r["gauge_control_recovered"] for r in rs),"tacho_controls_recovered":sum(r["known_tacho"] and r["tacho_control_recovered"] for r in rs),"with_linked_generators":sum(bool(r["unqualified_linked_generators"]) for r in rs),"outside_catalogue":[r["firmware"] for r in rs if not r["known_gauge"] and not r["known_tacho"] and r["unqualified_linked_generators"]]}
(O/"bilan_sorties_physiques_lot16.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
(O/"registre_sorties_physiques_lot16.json").write_text(json.dumps(results,indent=2,ensure_ascii=False))
print(json.dumps(summary,ensure_ascii=False))
