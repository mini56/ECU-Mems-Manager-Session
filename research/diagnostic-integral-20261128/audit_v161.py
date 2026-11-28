#!/usr/bin/env python3
import csv,json,re,hashlib,struct,collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
PRE=Path("research/diagnostic-integral-20261128")
OUT=PRE/"results_v161";OUT.mkdir(exist_ok=True)
# Reuse only the parser definitions from the previously executed immutable audit.
exec((PRE/"audit.py").read_text().split("inventory=[];")[0].replace('OUT=Path("research/diagnostic-integral-20261128/results")','OUT=Path("research/diagnostic-integral-20261128/results_v161")'))
old=list(csv.DictReader((PRE/"results/matrice_capteurs.csv").open(encoding="utf-8-sig")))
idx=collections.defaultdict(list)
for r in old:idx[r["path"]].append(r)
corrections=[]; modes=[];access=[]; details=[]
# Word-read pattern: double selector, zero extend, read block, shift block 8,
# add, compare against 001A, branch, indirect word read. Same-register constraints.
pat=re.compile(rb"\x19\x01(.)\xac\1(.)\xb3\x01(..)(.)\x09\x08\4\x64\2\4\x89\x1a\x00\4\xd3(.)\xa2\4(.)",re.S)
for p in sorted(ROOT.rglob("*")):
    if p.suffix.lower() not in (".hex",".rom",".bin") or not p.is_file():continue
    rel=str(p.relative_to(ROOT));family=str(p.relative_to(ROOT).parent);raw=p.read_bytes()
    prev=idx[rel];sib={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    xf=sib.get(".xf3",sib.get(".xf1"));asm=sib.get(".asm")
    older=family in ("Rover MEMS 1.3","Rover MEMS 1.6/NA","Rover MEMS 1.6 TURBO","Rover MEMS 1.9")
    t,ram,cal,flags=parse_xf(txt(xf));ins=asm_index(txt(asm)) if older and asm else {}
    candidates=[]
    if older:
        for m in pat.finditer(raw):
            for base in (0x2000,0):
                a=m.start()+base
                if a>65535:continue
                for j in hits(raw,b"\x7f\x00"+struct.pack("<H",a)):
                    q=j+4
                    if raw[q:q+2]==b"\xc7\xc7":q+=4
                    if q+8<=len(raw) and raw[q:q+2]==b"\xf2\xf2" and raw[q+4:q+6]==b"\xdc\xdc":
                        candidates.append(dict(handler=hx(a),table=hx(j+base),block_ram=hx(int.from_bytes(m[3],"little")),dc_handler=hx(int.from_bytes(raw[q+6:q+8],"little")),file_offset=hx(m.start()),pattern=m[0].hex(" "),base=base))
    mode=candidates[0] if len(candidates)==1 else {}
    status="WORD_READER_AND_DISPATCH_LINKED" if mode else "UNRESOLVED_OTHER_PROTOCOL_OR_VARIANT"
    modes.append(dict(path=rel,family=family,firmware=p.stem,sha256=hashlib.sha256(raw).hexdigest(),status=status,handler=mode.get("handler",""),table=mode.get("table",""),block_ram=mode.get("block_ram",""),dc_handler=mode.get("dc_handler",""),pattern=mode.get("pattern",""),physical="NOT_TESTED"))
    # Independently reconstruct the six-channel table from the previously linked coolant row.
    coolant=next(r for r in prev if r["sensor"]=="clnt")
    if family=="Rover MEMS 1.9" and coolant["status"]=="TABLE_AND_INDEXED_PRODUCER_LINKED":
        mm=re.search(r"producer=([0-9A-F]+).*base=([0-9A-F]+)",coolant["evidence"])
        prod,table=(int(mm[i],16) for i in (1,2));q=prod-0x2000
        rowbase=table-0x2000
        assert raw[q]==0x93 and raw[q+4]==0x93
        current_reg,logged_reg=raw[q+3],raw[q+7]
        for r in prev:
            k=r["sensor"]
            if r["status"]=="TABLE_AND_INDEXED_PRODUCER_LINKED":continue
            if k not in ("map","bat"):continue
            # Physical ADC channel index verified by common 14-byte table layout;
            # in nine MAP variants the row uses the instantaneous signal symbol.
            ch=2 if k=="map" else 5
            rr=raw[rowbase+14*ch:rowbase+14*(ch+1)]
            lo,hi,sub,cm,lm,lookup,fil,dest=struct.unpack("<HHHBBHHH",rr)
            inst=flags.get("a_map_inst_flt");instlog=flags.get("a_map_inst_log");f=flags.get("a_"+k+"_cur_flt")
            rawdest=ram.get("a_map_raw");batdest=ram.get("a_filt_bat")
            def sites(f,op):
                if not f or f[0] is None or f[0]>255:return []
                v=f[1] if op==0x91 else 255^f[1]
                return [i+0x2000 for i in hits(raw,bytes([op,v,f[0]])) if i+0x2000 in ins and re.search(r"\b(?:orb|andb)\b",ins[i+0x2000])]
            direct_set=sites(f,0x91);direct_clear=sites(f,0x71)
            if k=="map":
                valid=bool(inst and instlog and rawdest and (current_reg,cm)==inst[:2] and (logged_reg,lm)==instlog[:2] and dest==rawdest[0] and direct_set and direct_clear)
                stat="INSTANTANEOUS_ADC_AND_SEPARATE_MAP_FLAG_LINKED" if valid else "UNRESOLVED"
                note="ADC row represents instantaneous MAP; separate current MAP flag has direct set/clear. Exact plausibility condition and filtered transfer require further proof."
            else:
                valid=bool(f and batdest and (current_reg,cm)==f[:2] and dest==batdest[0])
                stat="CURRENT_BATTERY_LINKED_LOG_DISCREPANCY" if valid else "UNRESOLVED"
                note="Current bit matches actual ADC producer; logged byte used by table differs from Rover definition. Do not use logged mapping until separately proven."
            corrections.append(dict(path=rel,firmware=p.stem,sensor=k,status=stat,table=hx(table),producer=hx(prod),row=hx(table+14*ch),row_bytes=rr.hex(" "),actual_current=f"{current_reg:04X}/{cm:02X}",actual_logged=f"{logged_reg:04X}/{lm:02X}",rover_current=r["current"],rover_logged=r["stored"],destination=hx(dest),separate_set=";".join(map(hx,direct_set)),separate_clear=";".join(map(hx,direct_clear)),note=note))
    for r in prev:
        cf=r["current"];a=int(cf.split("/")[0],16) if cf else None
        eligible=bool(mode and a is not None and (a&~1)>=0x1a)
        access.append(dict(path=rel,sensor=r["sensor"],current=cf,source_status=r["status"],mode4_reader=status,aligned_word=hx(a&~1) if eligible else "",block=f"{a>>8:02X}" if eligible else "",selector=f"{(a&255)//2:02X}" if eligible else "",byte="high" if eligible and a&1 else "low" if eligible else "",status="ADDRESS_COMPATIBLE_STATIC_ONLY" if eligible else "UNRESOLVED",hardware="NOT_TESTED"))
assert len(modes)==len(idx)==483 and len(access)==2415 and len(corrections)==99
write_csv(OUT/"variantes_19.csv",corrections);write_csv(OUT/"lecteurs_mode4.csv",modes);write_csv(OUT/"acces_capteurs.csv",access)
summary={"firmwares":len(modes),"mode4":{},"corrections":dict(collections.Counter(r["status"] for r in corrections)),"access_candidates":sum(r["status"]=="ADDRESS_COMPATIBLE_STATIC_ONLY" for r in access)}
for f in sorted(set(r["family"] for r in modes)):summary["mode4"][f]=dict(collections.Counter(r["status"] for r in modes if r["family"]==f))
(OUT/"bilan.json").write_text(json.dumps(summary,indent=2),encoding="utf-8")
print(json.dumps(summary,ensure_ascii=False))
