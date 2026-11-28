#!/usr/bin/env python3
"""Read-only reverse search from instrument variables and outputs.
No diagnostic opcode is assumed. A reference is not qualification."""
import csv,json,re,hashlib,collections,bisect
import gzip
from pathlib import Path
R=Path("firmwares-extraits/Firmware Archive")
O=Path("research/instruments-integral-20261127/results")
rows=list(csv.DictReader((O/"inventaire_individuel.csv").open(encoding="utf-8-sig")))
old=list(csv.DictReader((O/"registre_seconde_passe_20261128.csv").open(encoding="utf-8-sig")))
previous={r["path"]:r for r in old}
prior_reverse={(r["family"],r["firmware"]):r for r in csv.DictReader((O/"comparaison_recherche_inverse_20261128.csv").open(encoding="utf-8-sig"))}
assert len(rows)==len(previous)==483
instpat=re.compile(r"tacho|rev.?counter|tmp.?gauge|temp(?:erature)?[_ ]gauge|tempg_pwm|tpg_force|tpg_onoff|instrument[_ ]cluster",re.I)
diagpat=re.compile(r"kp_.*(?:control|override)|diagnostic.*override|actuator",re.I)
def hx(i):return f"{i:04X}"
def read(p):return p.read_bytes().decode("utf-8",errors="replace") if p.exists() else ""
def parse(asm):
    ins={}; labels={}; starts=set()
    for l in asm.splitlines():
        m=re.match(r"^ROM:([0-9A-Fa-f]{4,8})\s+(.*)",l)
        if not m:continue
        a=int(m[1],16); s=m[2].strip()
        label=re.match(r"([\w.$]+):",s)
        if label:labels[label.group(1).lower()]=a
        if "S U B R O U T I N E" in s:continue
        if label and (label.group(1).lower().startswith("sub_") or "CODE XREF" in s and any(c in s for c in ("↑p","↓p"))): starts.add(a)
        if not s or s.startswith(";") or label:continue
        s=s.split(";")[0].strip()
        if not re.match(r"^[a-z][a-z0-9.]*\b",s,re.I) or re.match(r"^(db|dw|dd|ds|dc\.[bwl]|dc[bdw]|equ|org|align|end|endp|proc)\b",s,re.I):continue
        ins.setdefault(a,s)
    return ins,labels,starts
def target(s,labels):
    # Explicit ROM/sub/loc target, not indirect jumps.
    m=re.search(r"\b(?:ROM|sub|loc|locret)_([0-9a-f]+)\b",s,re.I)
    if m:return int(m.group(1),16)
    for token in re.findall(r"\b[\w.$]+\b",s):
        if token.lower() in labels:return labels[token.lower()]
    return None
def tables(b):
    result=[]
    for m in re.finditer(b"\xd0\xd0",b):
        j=m.start(); es=[]
        for k in range(j,min(j+1200,len(b)-3),4):
            hi,lo=b[k:k+2]; a=int.from_bytes(b[k+2:k+4],"little")
            if not 0x2000<=a<0x2000+len(b):break
            es.append((hi,lo,a))
            if (hi,lo)==(0x7f,0):break
        exact={h for h,l,a in es if h==l}
        if len(es)>=15 and {0xd0,0xd1,0xf4}<=exact and es[-1][:2]==(0x7f,0):
            result.append((j,es))
    if not result:return []
    j,es=max(result,key=lambda t:len(t[1]))
    # Keep command ranges rather than only exact opcodes. Preserve mode blocks.
    mode=0; out=[]
    for i,(hi,lo,a) in enumerate(es):
        if hi==lo==0xd0:mode+=1
        if (hi,lo)==(0x7f,0):break
        out.append({"table_address":hx(j+0x2000+i*4),"mode_block":mode,"high":hi,"low":lo,"handler":a})
    return out
def symbols(xf):
    out=[]; flags={}; current=None; pos=0
    for l in xf.splitlines():
        m=re.match(r"\s*(flagbyte\d+)\s+([0-9a-f]{2});([0-9a-f]{2});",l,re.I)
        if m:flags[m[1].lower()]=int(m[2]+m[3],16)
    for l in xf.splitlines():
        m=re.search(r"<<+\s*(flagbyte\d+)\s*>>+",l,re.I)
        if m:current=m[1].lower();pos=0;continue
        # XF3 enumerates bit names immediately after a two-column flag header.
        m=re.match(r"^\s*(flagbyte\d+)\s*,\s*([0-9a-f]{4})\s*$",l,re.I)
        if m:
            current=m[1].lower();flags[current]=int(m[2],16);pos=0;continue
        if l.lstrip().startswith(";"):
            if "<<<" in l:current=None
            continue
        s=l.strip()
        if not s:continue
        name=re.split(r"[\s,]",s)[0]
        a=None; bit=None
        m=re.match(r"[\w\[\]]+\s+([0-9a-f]{2});([0-9a-f]{2});",s,re.I)
        if m:a=int(m[1]+m[2],16)
        m=re.match(r"[\w\[\]]+\s*,\s*([0-9a-f]{4})\s*,\s*(?:S?[BWL]|DEAD)\s*,",s,re.I)
        if m:a=int(m[1],16)
        if current is not None and "," in s and a is None:
            if pos<8:
                a=flags.get(current);bit=pos;pos+=1
            else:
                current=None
        if instpat.search(name) or diagpat.search(name):out.append({"name":name,"address":a,"bit":bit,"definition":s})
    return out
def refs(ins,syms,arch):
    lookup=collections.defaultdict(list)
    for s in syms:
        if s["address"] is not None:lookup[s["address"]].append(s)
    out=collections.defaultdict(list)
    for a,s in ins.items():
        if arch=="80196":
            vals=[int(v,16) for v in re.findall(r"\bINTMEM_([0-9a-f]+)\b",s,re.I)]
        else:
            vals=[int(v,16) for v in re.findall(r"\$([0-9a-f]+)\s*\(a5(?:[,)]|\b)|\(\$([0-9a-f]+)\)\.[wl]",s,re.I) for v in v if v]
        for v in set(vals):
            for sym in lookup.get(v,[]):
                bit=sym["bit"]
                if bit is not None:
                    # Full-byte/word refs kept separate from direct bit test.
                    m=re.match(r"jb[cs]\s+[^,]+,\s*(\d+)\s*,",s,re.I)
                    if m and int(m[1])!=bit:continue
                    m=re.match(r"(orb|andb)\s+[^,]+,\s*#(-?)([0-9a-f]+)(h?)\b",s,re.I)
                    if m:
                        value=int(m[3],16 if m[4] else 10) if m[4] or m[3].isdigit() else 0
                        if m[2]:value=-value
                        changes=value if m[1].lower()=="orb" else ~value
                        if not changes&(1<<bit):continue
                out[sym["name"]].append([hx(a),s])
    return dict(out)
def generic_masks(b,ins,commands):
    out=[]
    # Inspect every handler, infer mask table from indexed add and indirect load,
    # then corroborate paired mask stores. No command-number filter.
    for c in commands:
        a=c["handler"]; w=[(i,ins[i]) for i in sorted(ins) if a<=i<a+65]
        ss=" ".join(s for i,s in w)
        if not ("notb" in ss and "orb" in ss and "andb" in ss and ss.count("stb")>=2):continue
        for i,s in w:
            m=re.match(r"add\s+(\w+),\s*\w+,\s*#([0-9a-f]+)h\b",s,re.I)
            if not m:continue
            ptr=int(m[2],16); off=ptr-0x2000
            if not 0<=off<=len(b)-16:continue
            if not any(re.search(r"\["+re.escape(m[1])+r"\]",q,re.I) for j,q in w if i<j<=i+12):continue
            masks=list(b[off:off+16]); mask=0
            for v in masks:mask|=v
            out.append({**c,"handler":hx(a),"mask_table":hx(ptr),"masks":masks,"union":mask,"bit6_reachable":bool(mask&64),"evidence":[[hx(i),s] for i,s in w]})
    return out
def reachable(ins,labels,commands,seeds):
    # Reverse graph traversal has no arbitrary depth or node budget.
    # Calls retain a conservative continuation edge; indirect transfers
    # remain unresolved. This is reachability, not a proof of dataflow.
    keys=sorted(ins); nxt={a:b for a,b in zip(keys,keys[1:]) if b-a<=12}
    pred=collections.defaultdict(set); unresolved=set()
    for p,s in ins.items():
        op=s.split()[0].lower(); t=target(s,labels)
        if op in ("ret","reti","rts","rte"):continue
        if op in ("br","ljmp","sjmp","jmp","bra","bra.w","bra.s"):
            if t in ins:pred[t].add(p)
            else:unresolved.add(p)
            continue
        if op.startswith("j") or op in ("djnz","lcall","scall","call","jsr","bsr","bsr.w","bsr.s"):
            if t in ins:pred[t].add(p)
            else:unresolved.add(p)
        if p in nxt:pred[nxt[p]].add(p)
    # Multi-source breadth first search: one shortest witness per handler.
    witness={p:None for p in sorted(seeds) if p in ins}
    q=collections.deque(witness)
    while q:
        p=q.popleft()
        for k in sorted(pred[p]):
            if k not in witness:witness[k]=p;q.append(k)
    ans=[]
    for c in commands:
        a=c["handler"]
        if a not in witness:continue
        path=[a]
        while witness[path[-1]] is not None:path.append(witness[path[-1]])
        ans.append({**c,"handler":hx(a),"seed_hits":[hx(path[-1])],"witness_path":[[hx(p),ins[p]] for p in path],"unresolved_graph_transfers":len(unresolved)})
    return ans
def classify_ref(s):
    op=s.split()[0].lower()
    if op in ("jbc","jbs","btst"):return "bit_read"
    if op in ("orb","andb","bset","bclr"):return "bit_change"
    if op.startswith("st") or op.startswith("move") or op.startswith("ld"):return "transfer"
    return "other"
results=[]
for row in rows:
    p=R/row["path"]; b=p.read_bytes(); assert hashlib.sha256(b).hexdigest()==row["sha256"],p
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    asm=read(siblings.get(".asm",p.with_suffix(".asm")));xf=read(siblings.get(".xf3",siblings.get(".xf1",p.with_suffix(".xf1"))))
    ins,labels,starts=parse(asm);arch=row["processor"];syms=symbols(xf);rs=refs(ins,syms,arch)
    cs=tables(b) if arch=="80196" else []
    seeds={int(a,16) for n,rs0 in rs.items() if instpat.search(n) for a,s in rs0}
    # Output evidence retained separately from commands: no assumed pin map.
    outputs=[[hx(a),s] for a,s in ins.items() if re.search(r"\b(?:stb?|orb|andb|ldb?|addb)\b.*(?:\bIOPORT[12]\b|40[0-9A-F]h\[\]|INTMEM_A4\b)|\b(?:move|bset|bclr|btst|clr)(?:\.[bwl])?\b.*\(\$FFFF[0-9A-F]{4}\)\.[wl]",s,re.I)]
    mask=generic_masks(b,ins,cs)
    # On 68K, retain xrefs to KWP override flags and the instrument
    # variables; a protocol dispatch table still has to be resolved.
    reach=reachable(ins,labels,cs,seeds) if cs and seeds else []
    prior=previous[row["path"]]
    r={"family":row["family"],"firmware":row["firmware"],"path":row["path"],"identity":row["identity"],"arch":arch,"prior_gauge":prior["gauge_status"],"prior_tacho":prior["tach_status"],"asm_count":len(ins),"instrument_symbols":[s for s in syms if instpat.search(s["name"])],"override_symbols":[s for s in syms if diagpat.search(s["name"])],"variable_references":rs,"all_dialogue_entries":[{**c,"handler":hx(c["handler"])} for c in cs],"command_to_variable_paths":reach,"generic_masks":mask,"hardware_output_references":outputs}
    r["new_opcode_candidates"]=[c for c in reach if not (c["high"]==c["low"] and c["high"] in (0,0x10,0x63,0x6b,0xf4))]
    r["instrument_reference_types"]={n:dict(collections.Counter(classify_ref(s) for a,s in v)) for n,v in rs.items() if instpat.search(n)}
    r["known_gauge"]=row["gauge_status"]=="INTEGRE_227"
    r["known_tach"]=row["tach_status"]=="INTEGRE_227"
    r["prior_reverse_candidate"]=int(prior_reverse[(r["family"],r["firmware"])]["new_opcode_candidates"])>0
    r["known_command_witnesses"]=[c for c in reach if c["high"]==c["low"] and c["high"] in (0,0x10,0x63,0x6b)]
    r["status"]="NO_ASM" if not ins else "REVERSE_REFERENCES_NEED_PROTOCOL_TRACE" if not cs and syms else "REVERSE_PATH_CANDIDATES" if reach else "NO_BOUNDED_PATH_FOUND"
    results.append(r)
assert len(results)==483
ek=next(r for r in results if r["firmware"]=="EKDMF005")
assert any(g["mask_table"]=="35F0" and g["union"]==63 for g in ek["generic_masks"]),ek["generic_masks"]
assert any(r["firmware"]=="EKDMF009" and any(s["name"]=="m_tmpgauge_dr" for s in r["instrument_symbols"]) for r in results)
# Controls are evaluated against actual code paths, not prefilled flags.
controls=[r for r in results if r["known_gauge"] or r["known_tach"]]
assert len(controls)==56
missing=[]
for r in controls:
    expected=(0,0x10) if r["known_gauge"] else (0x63,0x6b)
    found={c["high"] for c in r["known_command_witnesses"]}
    if not set(expected)<=found:missing.append({"firmware":r["firmware"],"expected":list(expected),"found":sorted(found),"refs":r["instrument_reference_types"]})
assert not missing, json.dumps(missing,ensure_ascii=False)
for name,reg,bit in (("ACNMP001",0xAC,6),("ACNMP003",0xAD,0)):
    r=next(r for r in results if r["firmware"]==name)
    assert any(s["name"]=="d_tpg_force" and s["address"]==reg and s["bit"]==bit for s in r["instrument_symbols"]),name
summary={}
for f in sorted({r["family"] for r in results}):
    rr=[r for r in results if r["family"]==f]
    summary[f]={"firmwares":len(rr),"known_controls_recovered":sum(r["known_gauge"] or r["known_tach"] for r in rr),"new_paths_vs_previous":sum(bool(r["new_opcode_candidates"]) and not r["prior_reverse_candidate"] for r in rr),"lost_paths_vs_previous":sum(not r["new_opcode_candidates"] and r["prior_reverse_candidate"] for r in rr),"with_asm":sum(bool(r["asm_count"]) for r in rr),"with_instrument_symbols":sum(bool(r["instrument_symbols"]) for r in rr),"with_instrument_refs":sum(any(instpat.search(n) and v for n,v in r["variable_references"].items()) for r in rr),"with_all_dialogue_entries":sum(bool(r["all_dialogue_entries"]) for r in rr),"with_new_opcode_candidates":sum(bool(r["new_opcode_candidates"]) for r in rr),"with_generic_masks":sum(bool(r["generic_masks"]) for r in rr),"with_generic_bit6":sum(any(m["bit6_reachable"] for m in r["generic_masks"]) for r in rr),"previous_gauge":dict(collections.Counter(r["prior_gauge"] for r in rr)),"previous_tacho":dict(collections.Counter(r["prior_tacho"] for r in rr)),"status":dict(collections.Counter(r["status"] for r in rr))}
(O/"bilan_recherche_inverse_v2_20261128.json").write_text(json.dumps(summary,indent=2,ensure_ascii=False))
# One JSON line per firmware, compressed for repository size.
import gzip
with gzip.open(O/"preuves_recherche_inverse_v2_20261128.jsonl.gz","wt",encoding="utf-8") as f:
    for r in results:f.write(json.dumps(r,ensure_ascii=False)+"\n")
with (O/"comparaison_recherche_inverse_v2_20261128.csv").open("w",encoding="utf-8-sig",newline="") as f:
    w=csv.writer(f);w.writerow(["family","firmware","previous_gauge","previous_tacho","status","instrument_symbols","instrument_refs","dialogue_entries","new_opcode_candidates","generic_masks","generic_bit6","physical_qualification","known_witnesses","prior_reverse_candidate"])
    for r in results:w.writerow([r["family"],r["firmware"],r["prior_gauge"],r["prior_tacho"],r["status"],len(r["instrument_symbols"]),sum(len(v) for n,v in r["variable_references"].items() if instpat.search(n)),len(r["all_dialogue_entries"]),len(r["new_opcode_candidates"]),len(r["generic_masks"]),any(g["bit6_reachable"] for g in r["generic_masks"]),"NOT_ESTABLISHED_BY_THIS_RUN",len(r["known_command_witnesses"]),r["prior_reverse_candidate"]])
print(json.dumps(summary,indent=2))
