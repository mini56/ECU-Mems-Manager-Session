#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, json, re, sqlite3
from pathlib import Path
import fitz

SHA="c050a3eebe50c5a85bf8a69b7722bd2052079944e09d58578a498984ecf06715"
PAGES=372
PAGE=20
DOC="RCL0193ENG"
ANCHORS=["GENERAL INFORMATION","GENERAL PRECAUTIONS AND FITTING INSTRUCTIONS","SAFETY INSTRUCTIONS","Jacking","Precautions against damage","Brake shoes and pads","Brake hydraulics","Engine coolant caps and plugs","Cleaning components"]
REFS={"LIFTING AND TOWING","ELECTRICAL PRECAUTIONS"}
CHAPTER="GENERAL INFORMATION"
SECTION_GROUP="GENERAL PRECAUTIONS AND FITTING INSTRUCTIONS"
SECTION="SAFETY INSTRUCTIONS"

def sha(b): return hashlib.sha256(b).hexdigest()
def writej(p,x): p.write_text(json.dumps(x,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
def box(r): return [round(float(x),3) for x in r]
def bold(font): return "bold" in font.lower()

def extract(page,out):
    d=page.get_text("dict",sort=False); lines=[]; blocks=[]; visuals=[]
    vd=out/"visuals"; vd.mkdir(parents=True,exist_ok=True)
    for bi,b in enumerate(d["blocks"]):
        if b["type"]==0:
            ids=[]
            for li,line in enumerate(b["lines"]):
                spans=[{"text":s["text"],"font":s["font"],"size":round(float(s["size"]),3),"flags":int(s["flags"]),"color":int(s["color"]),"bbox":box(s["bbox"]),"bold":bold(s["font"])} for s in line["spans"]]
                lid=f"{DOC}_P{PAGE:03d}_L{len(lines):03d}"
                lines.append({"line_id":lid,"native_order":len(lines),"block_index":bi,"line_in_block":li,"text":"".join(s["text"] for s in spans),"bbox":box(line["bbox"]),"spans":spans})
                ids.append(lid)
            blocks.append({"block_index":bi,"type":"text","bbox":box(b["bbox"]),"line_ids":ids})
        elif b["type"]==1:
            raw=bytes(b.get("image",b"")); ext=str(b.get("ext") or "bin").lower()
            vk=f"{DOC}_P{PAGE:03d}_VIS{len(visuals)+1:02d}"; rel=f"visuals/{vk}.{ext}"
            (out/rel).write_bytes(raw)
            v={"visual_key":vk,"block_index":bi,"bbox":box(b["bbox"]),"width":int(b.get("width",0)),"height":int(b.get("height",0)),"relative_path":rel,"sha256":sha(raw),"role":"warning_pictogram","occurrence_preserved":True}
            visuals.append(v); blocks.append({"block_index":bi,"type":"image","bbox":v["bbox"],"visual_key":vk})
        else:
            blocks.append({"block_index":bi,"type":f"unhandled_{b['type']}","bbox":box(b["bbox"])})
    return lines,blocks,visuals

def order(blocks):
    t=[b for b in blocks if b["type"]=="text"]; h=[b for b in t if b["bbox"][1]<60]; f=[b for b in t if b["bbox"][1]>780]
    body=[b for b in t if b not in h and b not in f]; l=[b for b in body if b["bbox"][0]<300]; r=[b for b in body if b["bbox"][0]>=300]
    seq=sorted(h,key=lambda b:(b["bbox"][1],b["bbox"][0]))+sorted(l,key=lambda b:(b["bbox"][1],b["bbox"][0]))+sorted(r,key=lambda b:(b["bbox"][1],b["bbox"][0]))+sorted(f,key=lambda b:(b["bbox"][1],b["bbox"][0]))
    return [b["block_index"] for b in seq]

def structure(lines,blocks,visuals):
    L={x["line_id"]:x for x in lines}; B={b["block_index"]:b for b in blocks}; items=[]; refs=[]; sub=None
    def add(kind,src,bbox,ids,level=None,chapter=CHAPTER,section_group=SECTION_GROUP,section=SECTION,subsection="__current__"):
        nonlocal items, sub
        if subsection=="__current__": subsection=sub
        k=f"{DOC}_P{PAGE:03d}_I{len(items):03d}"
        it={"item_key":k,"order_index":len(items),"item_type":kind,"heading_level":level,"chapter":chapter,"section_group":section_group,"section":section,"subsection":subsection,"source_text":src,"source_line_ids":ids,"bbox":bbox,"applicability_status":"not_explicit_on_page","translation_required_for_display":kind!="printed_page_number"}
        items.append(it); return it
    for bi in order(blocks):
        b=B[bi]; ls=[L[i] for i in b["line_ids"]]
        if not ls: continue
        texts=[x["text"] for x in ls]; first=texts[0].strip(); y=b["bbox"][1]
        allbold=all(s["bold"] for x in ls for s in x["spans"] if s["text"].strip())
        if y>780 and len(ls)==2 and texts[0].strip().isdigit():
            add("printed_page_number",texts[0],ls[0]["bbox"],[ls[0]["line_id"]],chapter=CHAPTER,section_group=SECTION_GROUP,section=None,subsection=None)
            add("running_footer_section",texts[1],ls[1]["bbox"],[ls[1]["line_id"]],chapter=CHAPTER,section_group=texts[1].strip(),section=None,subsection=None)
            continue
        kind="instruction_paragraph"; level=None
        if y<60 and first==CHAPTER:
            kind="running_header"
            src=" ".join(x.strip() for x in texts if x.strip())
            add(kind,src,box(b["bbox"]),[x["line_id"] for x in ls],level,chapter=first,section_group=None,section=None,subsection=None)
            continue
        elif first==SECTION:
            kind="section_title"; level=1; sub=None
        elif len(ls)==1 and allbold:
            kind="subsection_title"; level=2; sub=first
        elif first.startswith("WARNING:") and allbold:
            kind="warning"
        src=" ".join(x.strip() for x in texts if x.strip()); it=add(kind,src,box(b["bbox"]),[x["line_id"] for x in ls],level)
        for x in ls:
            for sp in x["spans"]:
                exact=sp["text"].strip(); key=exact.strip(" .,;:()[]")
                if key in REFS:
                    refs.append({"reference_key":f"{it['item_key']}_REF{len(refs)+1:02d}","item_key":it["item_key"],"source_text":key,"source_text_exact":exact,"type":"rave_section_cross_reference","bbox":sp["bbox"]})
        if kind=="warning":
            cand=[]
            for v in visuals:
                vx0,vy0,vx1,vy1=v["bbox"]; ix0,iy0,ix1,iy1=b["bbox"]
                overlap=max(0,min(vy1,iy1)-max(vy0,iy0))
                if overlap>0 and abs(ix0-vx1)<=40: cand.append((abs(ix0-vx1),v["visual_key"]))
            if cand: it["associated_visual_key"]=sorted(cand)[0][1]
    return items,refs

def dbwrite(path,source_sha,text,lines,items,visuals,refs):
    if path.exists(): path.unlink()
    c=sqlite3.connect(path); c.execute("PRAGMA foreign_keys=ON")
    c.executescript("""
    CREATE TABLE document(document_key TEXT PRIMARY KEY,source_path TEXT,language TEXT,sha256 TEXT,page_count INT,policy TEXT);
    CREATE TABLE page(page_key TEXT PRIMARY KEY,document_key TEXT REFERENCES document,physical_page INT,source_text_exact TEXT,source_text_sha256 TEXT,native_text INT,ocr_used INT);
    CREATE TABLE line(line_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,native_order INT,block_index INT,source_text TEXT,bbox_json TEXT,spans_json TEXT);
    CREATE TABLE content(item_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,order_index INT,item_type TEXT,chapter_title TEXT,section_group_title TEXT,section_title TEXT,subsection_title TEXT,source_text TEXT,source_line_ids_json TEXT,bbox_json TEXT,applicability_status TEXT);
    CREATE TABLE visual(visual_key TEXT PRIMARY KEY,page_key TEXT REFERENCES page,role TEXT,relative_path TEXT,sha256 TEXT,bbox_json TEXT,width INT,height INT);
    CREATE TABLE crossref(reference_key TEXT PRIMARY KEY,item_key TEXT REFERENCES content,source_text TEXT,type TEXT,bbox_json TEXT);
    CREATE INDEX idx_content_section ON content(section_title,subsection_title,order_index);
    CREATE INDEX idx_content_type ON content(item_type);
    """)
    pk=f"{DOC}_P{PAGE:03d}"
    c.execute("INSERT INTO document VALUES(?,?,?,?,?,?)",(DOC,"rave/xn/wmxn990e.pdf","en",source_sha,PAGES,"RAVEMEMS English source only; capture ALL RAVE; localize later in MEMS Manager"))
    c.execute("INSERT INTO page VALUES(?,?,?,?,?,?,?)",(pk,DOC,PAGE,text,sha(text.encode()),1,0))
    for x in lines: c.execute("INSERT INTO line VALUES(?,?,?,?,?,?,?)",(x["line_id"],pk,x["native_order"],x["block_index"],x["text"],json.dumps(x["bbox"]),json.dumps(x["spans"],ensure_ascii=False)))
    for x in items: c.execute("INSERT INTO content VALUES(?,?,?,?,?,?,?,?,?,?,?,?)",(x["item_key"],pk,x["order_index"],x["item_type"],x.get("chapter"),x.get("section_group"),x.get("section"),x.get("subsection"),x["source_text"],json.dumps(x["source_line_ids"]),json.dumps(x["bbox"]),x["applicability_status"]))
    for x in visuals: c.execute("INSERT INTO visual VALUES(?,?,?,?,?,?,?,?)",(x["visual_key"],pk,x["role"],x["relative_path"],x["sha256"],json.dumps(x["bbox"]),x["width"],x["height"]))
    for x in refs: c.execute("INSERT INTO crossref VALUES(?,?,?,?,?)",(x["reference_key"],x["item_key"],x["source_text"],x["type"],json.dumps(x["bbox"])))
    c.commit(); ok=c.execute("PRAGMA integrity_check").fetchone()[0]; c.close()
    if ok.lower()!="ok": raise SystemExit("SQLite integrity_check failed")

def main():
    a=argparse.ArgumentParser(); a.add_argument("--pdf",type=Path,required=True); a.add_argument("--out",type=Path,required=True); a.add_argument("--expected-sha256",default=SHA); a.add_argument("--page",type=int,default=PAGE); z=a.parse_args()
    out=z.out.resolve(); out.mkdir(parents=True,exist_ok=True); raw=z.pdf.read_bytes(); source_sha=sha(raw)
    if source_sha!=z.expected_sha256: raise SystemExit(f"source SHA mismatch {source_sha}")
    d=fitz.open(z.pdf)
    try:
        if len(d)!=PAGES or z.page!=PAGE: raise SystemExit(f"source/page contract mismatch pages={len(d)} page={z.page}")
        evidence="\n".join(d[i].get_text("text",sort=False) for i in range(min(8,len(d))))
        if not re.search(r"RCL\s*0193ENG",evidence,re.I) or not re.search(r"Amendment Number:.*ENG",evidence,re.I): raise SystemExit("English edition evidence missing")
        p=d[PAGE-1]; text=p.get_text("text",sort=False)
        if not text.strip(): raise SystemExit("native text missing: OCR path required")
        for x in ANCHORS:
            if x not in text: raise SystemExit(f"anchor missing: {x}")
        lines,blocks,visuals=extract(p,out); items,refs=structure(lines,blocks,visuals)
        if text.splitlines()!=[x["text"] for x in lines]: raise SystemExit("native line extraction differs from PDF text layer")
        allids=[x["line_id"] for x in lines]; covered=[i for x in items for i in x["source_line_ids"]]
        missing=sorted(set(allids)-set(covered)); dup=sorted({x for x in covered if covered.count(x)>1}); unknown=sorted(set(covered)-set(allids))
        subs=[x["source_text"] for x in items if x["item_type"]=="subsection_title"]; warnings=[x for x in items if x["item_type"]=="warning"]; assoc=sum(bool(x.get("associated_visual_key")) for x in warnings)
        foundrefs={x["source_text"] for x in refs}; unhandled=[b["type"] for b in blocks if b["type"].startswith("unhandled_")]
        if missing or dup or unknown or len(warnings)!=2 or len(visuals)!=2 or assoc!=2 or foundrefs!=REFS or len(subs)!=6 or unhandled:
            raise SystemExit(f"completeness gate failed missing={missing} dup={dup} unknown={unknown} warnings={len(warnings)} visuals={len(visuals)} assoc={assoc} refs={foundrefs} subs={len(subs)} unhandled={unhandled}")
        (out/"native_text_exact.txt").write_text(text,encoding="utf-8")
        L={x["line_id"]:x["text"] for x in lines}; B={b["block_index"]:b for b in blocks}
        readlines=[]
        for bi in order(blocks):
            b=B[bi]
            if b["type"]=="text": readlines += [L[i] for i in b["line_ids"]]
        (out/"reading_order_text.txt").write_text("\n".join(readlines)+"\n",encoding="utf-8")
        writej(out/"native_lines.json",lines); writej(out/"native_blocks.json",blocks); writej(out/"structured_content.json",items); writej(out/"cross_references.json",refs)
        knowledge=[{"knowledge_key":x["item_key"],"domain":"rave_general_information","knowledge_type":x["item_type"],"topic":x.get("subsection") or x.get("section") or x.get("section_group") or x.get("chapter") or "GENERAL INFORMATION","source_key":DOC,"document":"MINI WORKSHOP MANUAL","source_language":"en","source_page":PAGE,"source_chapter":x.get("chapter"),"source_section_group":x.get("section_group"),"source_section":x.get("section"),"source_subsection":x.get("subsection"),"source_text":x["source_text"],"source_line_ids":x["source_line_ids"],"bbox":x["bbox"],"verification_level":"rave_source_exact","applicability":None,"applicability_status":"not_explicit_on_page"} for x in items]
        writej(out/"knowledge_items.json",knowledge)
        bytype={}
        for it in items: bytype.setdefault(it["item_type"],[]).append(it)
        hdr=bytype["running_header"][0]; foot=bytype["running_footer_section"][0]; pno=bytype["printed_page_number"][0]
        hierarchy_ok=(hdr["chapter"]==CHAPTER and hdr["section_group"] is None and hdr["section"] is None and hdr["subsection"] is None and foot["chapter"]==CHAPTER and foot["section_group"]==SECTION_GROUP and foot["section"] is None and foot["subsection"] is None and pno["subsection"] is None and all(it["subsection"] is None for it in items if it["item_type"] in {"running_header","running_footer_section","printed_page_number"}))
        if not hierarchy_ok: raise SystemExit("hierarchy/context inheritance guard failed")
        p.get_pixmap(matrix=fitz.Matrix(1.5,1.5),alpha=False).save(out/f"{DOC}_P{PAGE:03d}_ORIGINAL.png")
        dbp=out/"ravemems_full_text_pilot.sqlite"; dbwrite(dbp,source_sha,text,lines,items,visuals,refs)
        c=sqlite3.connect(dbp); counts={k:c.execute(f"SELECT COUNT(*) FROM {k}").fetchone()[0] for k in ["document","page","line","content","visual","crossref"]}; integrity=c.execute("PRAGMA integrity_check").fetchone()[0]; c.close()
        manifest={"method":"RAVEMEMS_FULL_TEXT_NATIVE_PILOT","document_key":DOC,"source_path":"rave/xn/wmxn990e.pdf","source_language":"en","source_language_policy":"English source only; existing RAVE translations are not re-extracted","processed_languages":["en"],"other_languages_processed":[],"ignored_existing_rave_language_duplicates":True,"english_edition_evidence":"RCL0193ENG / ENG amendment marker","translation_performed_by_github":False,"localization_layer":"MEMS Manager","capture_scope":"ALL_RAVE_CONTENT_NO_ECU_FILTER","all_content_capture_required":True,"source_sha256":source_sha,"page_count":len(d),"physical_page":PAGE,"native_text_layer":True,"ocr_used":False,"native_text_chars":len(text),"native_text_sha256":sha(text.encode()),"native_line_count":len(lines),"native_text_block_count":sum(b["type"]=="text" for b in blocks),"embedded_visual_occurrences":len(visuals),"structured_item_count":len(items),"hierarchy":{"chapter":CHAPTER,"section_group":SECTION_GROUP,"section":SECTION,"context_inheritance_guard":hierarchy_ok},"warning_count":len(warnings),"warning_visual_association_count":assoc,"unhandled_block_types":unhandled,"subsection_titles":subs,"cross_references":sorted(foundrefs),"line_coverage":{"total_source_lines":len(allids),"covered_lines":len(covered),"missing_line_ids":missing,"duplicate_line_ids":dup,"unknown_line_ids":unknown,"coverage_ratio":1.0},"database":{**counts,"integrity":integrity},"original_page_render":f"{DOC}_P{PAGE:03d}_ORIGINAL.png","pass":integrity.lower()=="ok" and len(covered)==len(allids) and not missing and not dup and not unknown}
        writej(out/"manifest.json",manifest)
        if not manifest["pass"]: raise SystemExit("manifest failed")
        print("RAVEMEMS_FULL_TEXT_PILOT_PASS"); print(f"SOURCE_SHA256={source_sha}"); print(f"NATIVE_LINES={len(lines)}"); print(f"STRUCTURED_ITEMS={len(items)}"); print("LINE_COVERAGE=100%"); print("OCR_USED=false"); print("SOURCE_LANGUAGE=en")
    finally: d.close()

if __name__=="__main__": main()
