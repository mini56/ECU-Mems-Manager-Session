#!/usr/bin/env python3
import csv,json,re,hashlib,collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
PRE=Path("research/diagnostic-integral-20261128")
OUT=PRE/"results_v168";OUT.mkdir(parents=True,exist_ok=True)
scope=json.loads((PRE/"results_v165/perimetre.json").read_text())
inv=list(csv.DictReader((PRE/"results_v165/signatures/couverture_449.csv").open(encoding="utf-8-sig")))
prior=list(csv.DictReader((PRE/"results_v166/repertoire_signatures.csv").open(encoding="utf-8-sig")))
assert len(inv)==449 and all(x["family"] in scope["included"] for x in inv) and len(prior)==30837
anchors={x["path"]:x for x in json.loads((PRE/"results_v164/matrice_483.json").read_text()) if x["family"] in scope["included"] and x["status"]=="SERVICE23_READER_LINKED"}
assert len(anchors)==104
by=collections.defaultdict(list)
for r in prior:by[r["path"]].append(r)
def hx(n):return f"{n:04X}"
def imm(s):return int(s[1:],16) if s.startswith("$") else int(s)
def field(op,args,buf):
    m=re.fullmatch(r"#(\$[0-9A-F]+|\d+),\$([0-9A-F]+)\(d([0-7])\.w\)",args)
    return (imm(m[1]),int(m[3])) if op=="cmpi.b" and m and int(m[2],16)==buf else None
def target(s):
    m=re.fullmatch(r"(?:loc(?:ret)?|sub)_([0-9A-F]+)",s)
    return int(m[1],16) if m else None
def merge(a,b):return tuple(x if x==y else None for x,y in zip(a,b))
rows=[];coverage=[];details=[];new_sites=[];chunks_audit=[]
for f in inv:
    rel=f["path"];rr=by[rel];found={};issue=""
    if rel in anchors:
        path=ROOT/rel;raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==f["sha256"]
        q=next(x for x in anchors[rel]["candidates"] if x.get("verified"))
        base=int(q["base"],16);d=int(q["dispatch"],16);buf=int(q["buffer"],16)
        s=(path.parent/f["asm"]).read_text(encoding="latin-1")
        decl=[m for m in re.finditer(r"(?m)^ROM:([0-9A-F]{8})[ \t]+(sub_[0-9A-F]+):[^\r\n]*",s) if int(m[1],16)<=d]
        assert decl,rel
        m=decl[-1];end=re.search(r"(?m)^.*; End of function "+re.escape(m[2])+r"[ \t]*$",s[m.end():]);assert end,rel
        body=s[m.start():m.end()+end.end()]
        chunk_ranges=[]
        for cm in re.finditer(r"(?m)^ROM:([0-9A-F]{8})[ \t]+; START OF FUNCTION CHUNK FOR "+re.escape(m[2])+r"[ \t]*$",s):
            ce=re.search(r"(?m)^ROM:([0-9A-F]{8})[ \t]+; END OF FUNCTION CHUNK FOR "+re.escape(m[2])+r"[ \t]*$",s[cm.end():])
            if not ce:
                raise ValueError("Unclosed function chunk "+rel)
            chunk=s[cm.start():cm.end()+ce.end()]
            chunk_ranges.append((int(cm[1],16),int(ce[1],16)))
            body+="\n"+chunk
        chunks_audit.append(dict(path=rel,firmware=f["firmware"],function=m[2],chunks=len(chunk_ranges),ranges=";".join(hx(x)+"-"+hx(y) for x,y in chunk_ranges)))
        known={int(x["site"],16) for x in rr}
        rr=list(rr)
        for z in re.finditer(r"(?m)^ROM:([0-9A-F]{8})[ \t]+cmpi\.b[ \t]+(#(?:\$[0-9A-F]+|\d+),\$[0-9A-F]+\(d[0-7]\.w\))",body):
            a=int(z[1],16)
            if a in known or not any(lo<=a<=hi for lo,hi in chunk_ranges):continue
            ff=field("cmpi.b",z[2],buf)
            if not ff:continue
            code,reg=ff;sig=raw[a-base:a-base+8]
            if len(sig)!=8 or sig[:4]!=bytes([12,48,0,code]) or int.from_bytes(sig[6:8],"big")!=buf:continue
            entry=dict(path=rel,family=f["family"],firmware=f["firmware"],mode="SERVICE",command=f"{code:02X}",site=hx(a),handler="",signature=sig.hex(" ").upper(),status="SERVICE_COMPARE_CANDIDATE",kind="BUFFER_COMPARE_CHUNK",detail=z[0].strip(),physical="NOT_TESTED",previous_status="NEW_CHUNK_SITE",previous_mode="",context_proof="")
            rr.append(entry);new_sites.append(entry);known.add(a)
        ins={int(z[1],16):(z[2],(z[3] or "").strip()) for z in re.finditer(r"(?m)^ROM:([0-9A-F]{8})[ \t]+([a-z][a-z0-9.]*)(?:[ \t]+([^;\r\n]+))?[ \t]*(?:;[^\r\n]*)?$",body)}
        addresses=sorted(ins);nxt=dict(zip(addresses,addresses[1:]));start=int(m[1],16)
        for lo,hi in chunk_ranges:
            if hi in nxt:nxt.pop(hi)
            before=max((a for a in addresses if a<lo),default=None)
            if before is not None and nxt.get(before)==lo:nxt.pop(before)
        assert start in ins,rel
        # Abstract domain: low word d0..d7, matched primary service, header, pending compare offset/value.
        states={start:(None,)*12};work=collections.deque([start])
        while work:
            a=work.popleft();st=states[a];op,args=ins[a];v=list(st);fall=nxt.get(a)
            ff=field(op,args,buf);pending=(st[10],st[11]);v[10]=v[11]=None
            if ff:v[10]=st[ff[1]];v[11]=ff[0]
            dest=re.search(r"(?:^|,)\s*d([0-7])$",args)
            if op.startswith(("bsr","jsr")):v[:2]=[None,None]
            elif dest and not op.startswith(("cmp","tst","btst")):
                reg=int(dest[1]);z=re.fullmatch(r"#(\$[0-9A-F]+|-?\d+),d[0-7]",args)
                if op in ("move.w","move.l","moveq") and z:v[reg]=imm(z[1])&65535
                elif op in ("clr.w","clr.l"):v[reg]=0
                else:v[reg]=None
            last=args.rsplit(",",1)[-1].strip()
            direct={"($%X).%s"%(buf+i,k) for i in (0,1) for k in ("l","w")}
            if not op.startswith(("cmp","tst","btst")) and (last in direct or re.fullmatch(r"\$"+f"{buf:X}"+r"\(d[0-7]\.w\)",last)):v[8]=v[9]=None
            edges=[]
            if op in ("rts","rte","rtr"):pass
            elif op.startswith(("bra","jmp")):edges=[(target(args),tuple(v))]
            elif re.match(r"b(?:eq|ne|cc|cs|ge|gt|hi|le|ls|lt|mi|pl|vc|vs|hs|lo)\.",op):
                take=v.copy();skip=v.copy()
                if pending[0] in (0,1) and pending[1] is not None and op.startswith(("beq","bne")):
                    equal=take if op.startswith("beq") else skip
                    equal[8 if pending[0]==1 else 9]=pending[1]
                edges=[(target(args),tuple(take)),(fall,tuple(skip))]
            elif op.startswith("db"):edges=[(target(args.rsplit(",",1)[-1]),tuple(v)),(fall,tuple(v))]
            else:edges=[(fall,tuple(v))]
            for b,x in edges:
                if b not in ins:continue
                new=merge(states[b],x) if b in states else x
                if b not in states or new!=states[b]:states[b]=new;work.append(b)
        df=field(*ins.get(d,("","")),buf)
        if not(df and d in states and states[d][df[1]]==1):issue="Positive service23 control not established; preserve candidates"
        else:
            for r in rr:
                if r["status"]!="SERVICE_COMPARE_CANDIDATE":continue
                a=int(r["site"],16);st=states.get(a);ff=field(*ins.get(a,("","")),buf)
                if not st or not ff:continue
                code,reg=ff;offset=st[reg]
                assert raw[a-base:a-base+8]==bytes.fromhex(r["signature"])
                branch=ins.get(nxt.get(a),("",""));bt=target(branch[1]);fall=nxt.get(nxt.get(a))
                if not branch[0].startswith(("beq.","bne.")) or bt not in ins or fall not in ins:continue
                if offset==1:status="PRIMARY_SERVICE_FIELD_LINKED";mode="SERVICE"
                elif offset==2 and st[8] is not None:status="SUBFUNCTION_FIELD_LINKED";mode=f"SERVICE_{st[8]:02X}"
                elif offset==2:status="SECOND_FIELD_PARENT_UNRESOLVED";mode="SECOND_FIELD"
                elif offset==0:status="FRAME_HEADER_COMPARISON";mode="FRAME_HEADER"
                elif offset is not None:status="OTHER_FRAME_FIELD_LINKED";mode=f"FRAME_OFFSET_{offset}"
                else:continue
                eq=bt if branch[0].startswith("beq") else fall
                detail=f"buffer={hx(buf)}; index=d{reg}.w={offset}; parent={hx(st[8]) if st[8] is not None else '?'}; header={hx(st[9]) if st[9] is not None else '?'}; equal_path={hx(eq)}; branch={branch[0]} {branch[1]}; function={m[2]}"
                found[a]=(mode,status,detail)
                details.append(dict(path=rel,firmware=f["firmware"],site=hx(a),signature=r["signature"],mode=mode,command=f"{code:02X}",status=status,proof=detail))
    n=0
    for r0 in rr:
        r=dict(r0);r["v166_status"]=r["status"];r["v166_mode"]=r["mode"];r["field_proof"]=""
        if r["status"]=="SERVICE_COMPARE_CANDIDATE" and int(r["site"],16) in found:
            r["mode"],r["status"],r["field_proof"]=found[int(r["site"],16)];n+=1
        rows.append(r)
    coverage.append(dict(path=rel,family=f["family"],firmware=f["firmware"],candidates=sum(x["status"]=="SERVICE_COMPARE_CANDIDATE" for x in rr),reclassified=n,remaining=sum(x["status"]=="SERVICE_COMPARE_CANDIDATE" for x in rr)-n,issue=issue))
assert len(rows)==len(prior)+len(new_sites)
assert len(chunks_audit)==104
assert any(x["firmware"].lower()=="kmj7v001" and x["site"]=="10D14E" and x["status"]=="PRIMARY_SERVICE_FIELD_LINKED" for x in details)
assert any(x["firmware"].lower()=="kmj7v001" and x["site"]=="10D4A2" and x["status"]=="PRIMARY_SERVICE_FIELD_LINKED" for x in details)
assert any(x["firmware"].lower()=="kmj7v001" and x["site"]=="10D4E4" and x["mode"]=="SERVICE_27" for x in details)
assert any(x["firmware"].lower()=="cs73p001" and x["mode"]=="SERVICE_1A" and x["command"]=="87" for x in details)
assert any(x["firmware"].lower()=="adjmp002" and x["mode"]=="SERVICE_27" and x["command"]=="01" for x in details)
def write(name,rs):
    with (OUT/name).open("w",encoding="utf-8-sig",newline="") as h:
        w=csv.DictWriter(h,fieldnames=list(rs[0]));w.writeheader();w.writerows(rs)
write("chunks_68k.csv",chunks_audit);write("repertoire_signatures.csv",rows);write("preuves_champs_68k.csv",details);write("couverture_449.csv",coverage)
g=collections.defaultdict(list)
for r in rows:
    if r["command"]:g[(r["family"],r["mode"],r["command"],r["status"])].append(r)
write("index_commandes.csv",[dict(family=k[0],mode=k[1],command=k[2],status=k[3],firmwares=len({x["path"] for x in a}),occurrences=len(a)) for k,a in sorted(g.items())])
b={"firmwares":449,"68k_firmwares":104,"entries":len(rows),"reclassified":len(details),"new_chunk_sites":len(new_sites),"firmwares_with_chunks":sum(x["chunks"]>0 for x in chunks_audit),"statuses":dict(collections.Counter(x["status"] for x in rows)),"physical":"NOT_TESTED","complete":False}
(OUT/"bilan.json").write_text(json.dumps(b,indent=2),encoding="utf-8")
for lang in ("FR","EN"):
    body=("# V1.68 — Qualification des champs de trame 68K\n\n" if lang=="FR" else "# V1.68 — 68K frame field qualification\n\n")+json.dumps(b,indent=2)+"\n\n"+("Blocs annexes inclus ; indices prouvés par propagation de constantes et branches, pas une validation des fonctions ni de la trame série complète. Les 295 sélecteurs 80196 restent à traiter. Aucun diesel, aucune modification Manager/DLL." if lang=="FR" else "Detached chunks included; indices established by constant propagation and branches, not proof of function semantics or full wire frames. The 295 80196 selectors remain pending. No diesel or Manager/DLL changes.")
    (OUT/("note_"+lang+".md")).write_text(body,encoding="utf-8")
print(json.dumps(b))
