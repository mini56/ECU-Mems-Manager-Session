#!/usr/bin/env python3
"""Cross-check every previously detected 1.6 NA candidate against ROM/ASM.
Do not collapse reachability to compatibility. No ECU I/O."""
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
load("research/instruments-integral-20261127/audit_reverse.py",{"hx","read","parse","target","tables","symbols","refs","generic_masks","reachable"})
load("research/instruments-integral-20261127/audit_16na_outputs.py",{"var","imm","op","args","xfmap","onebit"})
def jsonl(name):
    with gzip.open(O/name,"rt",encoding="utf-8") as f:return list(map(json.loads,f))
rows=[r for r in csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")) if r["family"]=="Rover MEMS 1.6"]
v2={r["firmware"]:r for r in jsonl("preuves_recherche_inverse_v2_20261128.jsonl.gz") if r["family"]=="Rover MEMS 1.6"}
p4={r["firmware"]:r for r in jsonl("preuves_16na_producteurs_lot04.jsonl.gz")}
p3={r["firmware"]:r for r in jsonl("preuves_16na_sorties_lot03.jsonl.gz")}
assert len(rows)==len(v2)==len(p3)==len(p4)==123
def analyse(r):
    name=r["firmware"]; p=R/r["path"]; b=p.read_bytes(); assert hashlib.sha256(b).hexdigest()==r["sha256"],p
    sib={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read(sib.get(".asm",p.with_suffix(".asm")))
    xf=read(sib.get(".xf3",sib.get(".xf1",p.with_suffix(".xf1"))))
    ins,labels,_=parse(asm); cs=tables(b); sy=xfmap(xf); keys=sorted(ins); idx={a:i for i,a in enumerate(keys)}
    assert ins and cs
    normal_mode=next(c["mode_block"] for c in cs if c["high"]==c["low"]==0xd1)
    normal=[c for c in cs if c["mode_block"]==normal_mode]
    def ctx(a,lo=6,hi=10):
        i=idx[a];return [[hx(k),ins[k]] for k in keys[max(0,i-lo):i+hi]]
    def raw(a,n=4):
        return b[a-0x2000:a-0x2000+n].hex(" ")
    def cmd_witness(seeds):
        return reachable(ins,labels,normal,set(seeds)) if seeds else []
    # Trace named flags independently from V2, preserving bit tests,
    # whole-byte and overlapping word accesses, plus raw ROM witnesses.
    flag_symbols=[s for s in symbols(xf) if s["name"] in ("d_tpg_force","d_tpg_onoff") and s["address"] is not None and s["bit"] is not None]
    flags=[]
    for f in flag_symbols:
        reg,bit=f["address"],f["bit"]; direct=[]; wide=[]; mutations=[]; unresolved_raw=[]
        for a,s in ins.items():
            q=args(s);o=op(s)
            if not q:continue
            if o in ("jbc","jbs") and len(q)==3 and var(q[0])==reg and q[1]==str(bit):
                direct.append({"address":hx(a),"instruction":s,"raw":raw(a,3),"context":ctx(a)})
            if o in ("orb","andb","xorb") and len(q)==2 and var(q[0])==reg and imm(q[1]) is not None:
                m=imm(q[1]); change=m if o!="andb" else ~m
                if change&(1<<bit):mutations.append(a)
            # 80196 word operands may read the flag byte via the preceding
            # address. Keep byte-wide transfers too, not only JBC/JBS.
            if o in ("ld","st","and","or","xor","cmp","add","sub","not","clr","push","pop","ldb","stb","andb","orb","xorb","cmpb","notb","clrb"):
                word=o in ("ld","st","and","or","xor","cmp","add","sub","not","clr","push","pop")
                if any(var(t)==reg or word and var(t)==reg-1 for t in q):
                    wide.append({"address":hx(a),"instruction":s,"raw":raw(a),"context":ctx(a,2,3)})
        # Exact JBC/JBS encoding (30+bit,38+bit) checked against ASM, so
        # a match in data is not silently promoted to executable code.
        for opcode in (0x30+bit,0x38+bit):
            for m in re.finditer(re.escape(bytes([opcode,reg&255])),b):
                a=m.start()+0x2000
                if not any(x["address"]==hx(a) for x in direct):unresolved_raw.append({"address":hx(a),"raw":raw(a,3),"asm":ins.get(a)})
        flags.append({"symbol":f,"direct_bit_reads":direct,"whole_byte_word_accesses":wide,"raw_unmatched_bit_patterns":unresolved_raw,"mutation_commands":cmd_witness(mutations)})
    # Inspect every strategy merge with output bit 6 unmasked, not just
    # those where the conventional PWM structure was found.
    bit6=[]
    for m in p4[name]["merges"]:
        if m["bit6_preserved_in_shadow"]:continue
        reg=int(m["strategy"],16)
        producers=[]
        for a,s in ins.items():
            q=args(s);o=op(s)
            if len(q)<1:continue
            d=q[1] if o in ("st","stb") and len(q)>1 else q[0]
            if var(d) not in (reg,reg-1):continue
            if o in ("orb","andb","xorb") and len(q)==2 and var(d)==reg and imm(q[1]) is not None:
                mask=imm(q[1]);change=(mask if o!="andb" else ~mask)&255
                if not change&64:continue
            elif o not in ("ld","st","ldb","stb","or","and","xor","not","clr","orb","andb","xorb","notb","clrb"):continue
            producers.append({"address":hx(a),"instruction":s,"context":ctx(a),"raw":raw(a)})
        bit6.append({"merge":m,"all_bit6_or_word_producers":producers,"command_witnesses":cmd_witness([int(x["address"],16) for x in producers])})
    # Enumerate direct output stores for alternate instrument paths and
    # computed ROM table branch targets. A finite table is considered
    # resolved only if every decoded target is valid; otherwise retain it.
    computed=[]
    for a,s in ins.items():
        if op(s) not in ("br","jmp") or "[" not in s:continue
        q=args(s);i=idx[a];prev=keys[max(0,i-6):i]
        tab=[]
        for k in prev:
            z=args(ins[k])
            if op(ins[k])!="ld" or len(z)!=2 or not q or "["+z[0]+"]"!=q[0]:continue
            mm=re.fullmatch(r"([0-9A-F]+)h\[(.+)\]",z[1],re.I)
            if not mm:continue
            base=int(mm[1],16); off=base-0x2000
            if off<0 or off>=len(b)-1:continue
            targets=[]
            for j in range(off,min(off+64,len(b)-1),2):
                t=int.from_bytes(b[j:j+2],"little")
                if t not in ins:break
                targets.append(hx(t))
            tab.append({"table":hx(base),"candidate_targets":targets,"extent_proven":False})
        computed.append({"address":hx(a),"context":ctx(a),"table_candidates":tab,"status":"UNRESOLVED_EXTENT_OR_TARGET"})
    prev=v2[name]
    known=r["gauge_status"]=="INTEGRE_227"
    oldpaths=prev["new_opcode_candidates"]
    # Every previous candidate is preserved, including the full witness
    # instruction chain, whether or not a new PWM pattern was found.
    status="HISTORICAL_PROFILE_REFOUND" if known and p3[name]["labelled_gauge_candidates"] else "REVIEW_BIT6_PATH" if bit6 else "REVIEW_PRIOR_REVERSE_PATHS" if oldpaths else "REVIEW_NO_PRIOR_PATH"
    return {"firmware":name,"identity":r["identity"],"rom_sha256":r["sha256"],"known":known,"v2_status":prev["status"],"v2_alternative_paths":oldpaths,"v2_instrument_refs":prev["variable_references"],"named_flags":flags,"bit6_paths":bit6,"pwm_output_candidates":p4[name]["pwm_output_candidates"],"generic_masks":p4[name]["generic_masks"],"hso_output_writes":p4[name]["hso"],"other_output_writes":p4[name]["other_outputs"],"computed_transfers":computed,"status":status}
res=[analyse(r) for r in rows]
assert len(res)==123
assert sum(x["known"] for x in res)==15
assert sum(x["status"]=="HISTORICAL_PROFILE_REFOUND" for x in res)==15
previous=[x for x in res if not x["known"] and x["v2_alternative_paths"]]
assert len(previous)==104, len(previous)
other=[x for x in res if not x["known"] and not x["v2_alternative_paths"]]
assert len(other)==4
summary={"firmwares":len(res),"known_refound":15,"prior_104_candidates_preserved":[x["firmware"] for x in previous],"four_without_prior_path":[x["firmware"] for x in other],"bit6_nonhistorical":[{"firmware":x["firmware"],"paths":[{"port":z["merge"]["port"],"address":z["merge"]["address"],"strategy":z["merge"]["strategy"],"producer_count":len(z["all_bit6_or_word_producers"]),"command_count":len(z["command_witnesses"])} for z in x["bit6_paths"]]} for x in res if not x["known"] and x["bit6_paths"]],"nonhistorical_direct_flag_consumers":[{"firmware":x["firmware"],"flags":[{"symbol":f["symbol"],"reads":f["direct_bit_reads"]} for f in x["named_flags"] if f["direct_bit_reads"]]} for x in res if not x["known"] and any(f["direct_bit_reads"] for f in x["named_flags"])],"nonhistorical_computed_transfers":sum(bool(x["computed_transfers"]) for x in res if not x["known"]),"note":"Review evidence per command and output; neither a graph witness nor a port bit proves a test of the instrument. No incompatible classification from an unresolved transfer."}
with gzip.open(O/"preuves_16na_confrontation_lot05.jsonl.gz","wt",encoding="utf-8") as f:
    for x in res:f.write(json.dumps(x,ensure_ascii=False)+"\n")
(O/"bilan_16na_confrontation_lot05.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
with (O/"registre_16na_confrontation_lot05.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(["firmware","identity","known","v2_paths","direct_flag_reads","whole_byte_word_refs","bit6_paths","computed_transfers","status"])
    for x in res:w.writerow([x["firmware"],x["identity"],x["known"],len(x["v2_alternative_paths"]),sum(len(f["direct_bit_reads"]) for f in x["named_flags"]),sum(len(f["whole_byte_word_accesses"]) for f in x["named_flags"]),len(x["bit6_paths"]),len(x["computed_transfers"]),x["status"]])
print(json.dumps(summary,ensure_ascii=False))
