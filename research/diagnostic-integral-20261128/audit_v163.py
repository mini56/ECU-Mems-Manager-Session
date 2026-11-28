#!/usr/bin/env python3
"""V1.63: all images, previous evidence + 68K service 23; no ECU commands sent."""
import csv, json, re, hashlib, struct, collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
PRE=Path("research/diagnostic-integral-20261128")
OUT=PRE/"results_v163"; OUT.mkdir(exist_ok=True)
old=list(csv.DictReader((PRE/"results_v161/lecteurs_mode4.csv").open(encoding="utf-8-sig")))
prior=json.loads((PRE/"results_v162/lecteurs_13_turbo.json").read_text())
old7={x["sha"].lower():x for x in prior}
pat=re.compile(rb"\x0c\x30\x00\x23\x01\xa0(..)\x66\xff\x00\x00\x00\x0e\x61\x00(..)",re.S)
head=re.compile(rb"\x42\x82\x42\x81\x12\x39\x00\x00(..)\x36\x3c\x00\x10\xe7\xa9\x84\x81\x42\x81\x12\x39\x00\x00(..)\xe1\x89\x84\x81\x42\x81\x12\x39\x00\x00(..)\x84\x81\x20\x42\x42\x81\x12\x39\x00\x00(..)\xd2\x82\x04\x81\x00\x00\x00\x01\x26\x01",re.S)
def hx(n):return f"{n:06X}"
def code(s):
    return s.split(";",1)[0].strip()
def scan(raw,asm):
    text=asm.read_text(encoding="latin-1") if asm else ""
    base_m=re.search(r"Range:\s*([0-9A-F]+)h\s*-",text,re.I)
    base=int(base_m[1],16) if base_m else None
    proc=re.search(r"Processor\s*:\s*([^\r\n]+)",text)
    proc=proc[1].strip() if proc else ""
    out=[]
    for m in pat.finditer(raw):
        d=m.start()
        if d%2:continue
        t=d+16+struct.unpack(">h",m[2])[0]
        if t<0 or t>=len(raw):continue
        h=head.match(raw,t)
        if not h:continue
        buf=int.from_bytes(m[1],"big")
        if [int.from_bytes(g,"big") for g in h.groups()]!=list(range(buf+2,buf+6)):continue
        rec=dict(dispatch_offset=hx(d),handler_offset=hx(t),buffer=hx(buf),dispatch_bytes=m[0].hex(" "),handler_bytes=h[0].hex(" "),base=hx(base) if base is not None else "",processor=proc)
        if base is not None:
            rec.update(dispatch=hx(base+d),handler=hx(base+t))
            # Verify ASM dispatch, actual subroutine boundary, memory copy and reply service.
            sr=re.search(r"(?m)^ROM:"+f"{base+t:08X}"+r"\s+(sub_[0-9A-F]+):[^\r\n]*",text)
            if sr:
                stop=re.search(r"(?m)^.*; End of function "+re.escape(sr[1])+r"\s*$",text[sr.end():])
                body=text[sr.start():sr.end()+stop.end()] if stop else ""
                cp=re.search(r"(?m)^ROM:([0-9A-F]+)\s+move\.b\s+\(a0\)\+,\(a1\)",body)
                dp=re.search(r"(?m)^ROM:"+f"{base+d:08X}"+r"\s+cmpi\.b\s+#\$23,",text)
                reply=re.search(r"addi\.b\s+#\$40,",body)
                if cp:
                    off=int(cp[1],16)-base
                    rec["copy"]=cp[1]
                    rec["copy_bytes"]=raw[off:off+2].hex(" ")
                    rec["copy_binary_verified"]=raw[off:off+2]==b"\x12\x98"
                rec["asm_dispatch_verified"]=bool(dp)
                rec["asm_reply_verified"]=bool(reply)
                rec["asm_bounds"]=[code(x) for x in body.splitlines() if re.search(r"cmpi\.[lw]\s+#",x)]
                rec["asm_address_fields"]=[code(x) for x in body.splitlines() if re.search(r"move\.b\s+\(\$[0-9A-F]+\)\.l,d1",x)]
                rec["verified"]=bool(dp and cp and reply and rec.get("copy_binary_verified") and rec["asm_bounds"])
        out.append(rec)
    return proc,out
rows=[]
paths={str(p.relative_to(ROOT)):p for p in ROOT.rglob("*") if p.is_file() and p.suffix.lower() in (".bin",".hex",".rom")}
assert set(paths)=={r["path"] for r in old} and len(old)==483
for r in old:
    p=paths[r["path"]]; raw=p.read_bytes()
    sha=hashlib.sha256(raw).hexdigest()
    assert sha==r["sha256"]
    row=dict(path=r["path"],family=r["family"],firmware=r["firmware"],sha256=sha,physical="NOT_TESTED",previous_status=r["status"])
    if r["status"]=="WORD_READER_AND_DISPATCH_LINKED":
        row.update(status="PREVIOUS_WORD_READER_LINKED",previous_handler=r["handler"],previous_table=r["table"],previous_dc=r["dc_handler"])
    elif hashlib.sha1(f"blob {len(raw)}\0".encode()+raw).hexdigest() in old7:
        q=old7[hashlib.sha1(f"blob {len(raw)}\0".encode()+raw).hexdigest()]
        row.update(status="PREVIOUS_WORD_READER_LINKED",previous_handler=q["handler"],previous_table=q["table"],previous_dc=q["dc"])
    else:
        sib=next((s for s in p.parent.iterdir() if s.stem.lower()==p.stem.lower() and s.suffix.lower()==".asm"),None)
        proc,found=scan(raw,sib)
        verified=[x for x in found if x.get("verified")]
        row.update(processor=proc,candidates=found,status="SERVICE23_READER_LINKED" if len(verified)==1 else "UNRESOLVED_OTHER_PROTOCOL_OR_VARIANT")
    rows.append(row)
assert sum(x["status"]=="PREVIOUS_WORD_READER_LINKED" for x in rows)==345
# Positive controls are independent ASM observations. A failure is visible, never a silent zero.
controls={"adjmp002":("10BB2C","10BEBA"),"cs73p001":("1227A2","123102"),"ds43i003":("123A5E","1243BE")}
for r in rows:
    if r["firmware"].lower() in controls:
        d,t=controls[r["firmware"].lower()]
        assert any(x.get("verified") and (x.get("dispatch"),x.get("handler"))==(d,t) for x in r.get("candidates",[])),r
summary={f:dict(collections.Counter(r["status"] for r in rows if r["family"]==f)) for f in sorted({r["family"] for r in rows})}
(OUT/"matrice_483.json").write_text(json.dumps(rows,indent=2,ensure_ascii=False),encoding="utf-8")
(OUT/"bilan.json").write_text(json.dumps({"total":len(rows),"families":summary,"statuses":dict(collections.Counter(r["status"] for r in rows))},indent=2),encoding="utf-8")
print((OUT/"bilan.json").read_text())
