import base64, csv, gzip, io, json, re, subprocess, sys, tempfile, zipfile, zlib
from pathlib import Path
src=Path(sys.argv[1]).resolve()
out=Path(sys.argv[2]).resolve()
out.mkdir(parents=True,exist_ok=True)
sha=subprocess.check_output(["git","-C",str(src),"rev-parse","HEAD"],text=True).strip()
assert sha=="4b465406cca3a3a619b0396bc8ca85c3af4c9dc0",sha
paths=[p.decode("utf-8") for p in subprocess.check_output(["git","-C",str(src),"ls-files","-z"]).split(b"\0") if p]
# Named ECU, numeric identity, and model reference used by Manager.
patterns={
 "AANMP002":r"AANMP[\s_-]{0,16}002",
 "ID_980002":r"(?<![0-9A-Fa-f])(?:0x)?980002(?:02)?(?![0-9A-Fa-f])",
 "ID_bytes":r"(?<![0-9A-Fa-f])(?:0x)?98[,;:\\s-]{1,16}(?:0x)?0{1,2}[,;:\\s-]{1,16}(?:0x)?0?2(?![0-9A-Fa-f])",
 "MNE101150":r"MNE[\s_-]{0,16}101150",
}
rx={k:re.compile(v,re.I) for k,v in patterns.items()}
hits=[]; coverage=[]; errors=[]
def add(name,layer,where,key,context):
    hits.append([name,layer,str(where),key,context.replace("\0","\\0")[:650]])
def scan_text(name,layer,s):
    for key,r in rx.items():
        previous=0; line=1
        for m in r.finditer(s):
            line+=s.count("\n",previous,m.start()); previous=m.start()
            a=max(0,m.start()-100); b=min(len(s),m.end()+100)
            add(name,layer,line,key,s[a:b].replace("\r"," ").replace("\n"," "))
def scan_raw(name,layer,b):
    for k in ("AANMP002","980002","98000202","MNE101150"):
        for enc in ("ascii","utf-16le","utf-16be"):
            p=k.encode(enc); start=0
            for m in re.finditer(re.escape(p),b,re.I):
                add(name,layer+" "+enc,hex(m.start()),k,b[max(0,m.start()-24):m.end()+24].hex(" "))
def scan(name,b,depth=0):
    if depth>7: errors.append([name,"nested depth limit"]);return
    suffix=name.lower()
    if len(b)>1000000: print("SCAN",name,len(b),flush=True)
    try:
        # Plain text, including scripts, generated resources, and data.
        if suffix.endswith((".qz64",".b64")):
            # Search the decoded content as well; encoded ASCII is not meaningful for split numeric IDs.
            for key in ("AANMP002","980002","98000202","MNE101150"):
                for m in re.finditer(key.encode(),b,re.I):add(name,"encoded literal",hex(m.start()),key,"literal in encoded stream")
        elif b.startswith((b"\xff\xfe",b"\xfe\xff")):
            scan_text(name,"UTF16",b.decode("utf-16"))
        elif b.count(b"\0")<=max(1,len(b)//1000):
            try: scan_text(name,"text",b.decode("utf-8-sig"))
            except UnicodeDecodeError: scan_raw(name,"bytes",b)
        else:
            scan_raw(name,"bytes",b)
        if suffix.endswith(".qz64"):
            d=base64.b64decode(b)
            scan(name+"!qUncompress",zlib.decompress(d[4:]),depth+1)
        elif suffix.endswith(".b64"):
            scan(name+"!base64",base64.b64decode(b),depth+1)
        elif b.startswith(b"PK\x03\x04"):
            with zipfile.ZipFile(io.BytesIO(b)) as z:
                for member in z.infolist():
                    if not member.is_dir():
                        scan_text(name+"!"+member.filename,"member-name",member.filename)
                        scan(name+"!"+member.filename,z.read(member),depth+1)
        elif b.startswith(b"\x1f\x8b") and not suffix.endswith(".part"):
            scan(name+"!gzip",gzip.decompress(b),depth+1)
        elif b.startswith(b"%PDF"):
            with tempfile.TemporaryDirectory() as td:
                p=Path(td)/"in.pdf";p.write_bytes(b)
                cmd=subprocess.run(["pdftotext","-layout",str(p),"-"],capture_output=True,timeout=60)
                if cmd.returncode: raise ValueError("pdftotext: "+cmd.stderr.decode(errors="replace")[:160])
                scan_text(name,"PDF text",cmd.stdout.decode("utf-8",errors="replace"))
        elif b.startswith(b"\x89PNG\r\n\x1a\n"):
            pos=8
            while pos+12<=len(b):
                n=int.from_bytes(b[pos:pos+4],"big");typ=b[pos+4:pos+8];d=b[pos+8:pos+8+n]
                if typ==b"tEXt":scan_text(name,"PNG metadata",d.decode("latin1"))
                elif typ==b"zTXt":
                    key,rest=d.split(b"\0",1);scan_text(name,"PNG metadata",key.decode("latin1")+" "+zlib.decompress(rest[1:]).decode("latin1"))
                elif typ==b"iTXt":
                    key,rest=d.split(b"\0",1);flag=rest[0];tail=rest[2:].split(b"\0",2)[-1];scan_text(name,"PNG metadata",key.decode("latin1")+" "+(zlib.decompress(tail) if flag else tail).decode("utf-8",errors="replace"))
                pos+=12+n
    except Exception as exc: errors.append([name,str(exc)[:300]])
for index,name in enumerate(paths):
    if index%50==0: print("PROGRESS",index,len(paths),name,flush=True)
    before=len(hits); error_before=len(errors)
    try:
        scan_text(name,"path",name)
        b=(src/name).read_bytes();scan(name,b)
        coverage.append([name,len(b),len(hits)-before,"error" if len(errors)>error_before else "scanned"])
    except Exception as exc: errors.append([name,str(exc)]);coverage.append([name,0,0,"error"])
parts=sorted(src.glob("database/documentary/parts/*.gz.part"))
if parts:
    try:
        b=b"".join(p.read_bytes() for p in parts)
        scan("database/documentary/parts/REASSEMBLED.gz",b)
    except Exception as exc: errors.append(["database/documentary/parts/REASSEMBLED.gz",str(exc)])
def write_csv(name,header,rows):
    with (out/name).open("w",encoding="utf-8",newline="") as f:
        w=csv.writer(f,delimiter=";");w.writerow(header);w.writerows(rows)
write_csv("occurrences.csv",["path","layer","line_or_offset","pattern","context"],hits)
write_csv("coverage.csv",["path","bytes","hits","status"],coverage)
write_csv("errors.csv",["path","error"],errors)
counts={}
for h in hits: counts[h[0]]=counts.get(h[0],0)+1
summary={"source_commit":sha,"tracked_files":len(paths),"scanned_files":sum(x[3]=="scanned" for x in coverage),"errors":len(errors),"hits":len(hits),"matched_paths":len(counts),"counts":counts,"limitations":["No OCR of pixels or disassembly of executable binaries; exact byte strings searched.","No external update packs not in the tracked snapshot; encrypted or unsupported nested formats may hide text.","Numeric references and MNE101150 are candidates, not proof of a firmware-specific condition."]}
(out/"summary.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding="utf-8")
lines=["# Audit intégral des occurrences — SESSION 231",f"Commit : {sha}",f"Fichiers suivis : {len(paths)} ; traités sans erreur : {summary['scanned_files']} ; erreurs : {len(errors)}.",f"Occurrences : {len(hits)} dans {len(counts)} chemins (y compris données décompressées).","Inventaire sans sélection fonctionnelle : occurrences.csv ; couverture exhaustive des chemins : coverage.csv ; erreurs : errors.csv.","Les occurrences binaires et numériques sont des pistes, non des restrictions prouvées.","Limites : "+" ; ".join(summary["limitations"]),"","| Fichier / contenu | Occurrences |","|---|---:|"]+[f"| {p} | {n} |" for p,n in sorted(counts.items())]
(out/"README.md").write_text("\n".join(lines)+"\n",encoding="utf-8")
print(json.dumps({k:v for k,v in summary.items() if k!="counts"},ensure_ascii=False))
