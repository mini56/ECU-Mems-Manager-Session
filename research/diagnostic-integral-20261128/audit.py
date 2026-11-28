#!/usr/bin/env python3
"""Full-corpus passive research. No ECU access, no Manager build."""
import csv, hashlib, json, re, struct, collections
from pathlib import Path
ROOT=Path("firmwares-extraits/Firmware Archive")
OUT=Path("research/diagnostic-integral-20261128/results")
OUT.mkdir(parents=True,exist_ok=True)
THEMES={
 "temperature_moteur":["a_clnt_cur_flt","a_clnt_log_flt","a_clnt_backup","a_filt_clnt","m_coolant_temp","i_coolant_temp"],
 "temperature_air":["a_iat_cur_flt","a_iat_log_flt","a_iat_backup","a_filt_iat","m_strat_iat"],
 "map":["a_map_cur_flt","a_map_log_flt","a_map_backup","m_pbu_status","m_backup_map","m_pbu_fault_log"],
 "papillon":["a_thp_cur_flt","a_thp_log_flt","a_thp_backup","m_thr_sw_hard","m_thr_sw_soft","m_thr_sw_slow","m_thr_sw_status"],
 "batterie":["a_bat_cur_flt","a_bat_log_flt","a_bat_backup","i_lv_comp_prv","i_lv_enbl_thr","i_lv_dis_thr","i_lv_setpnt_off","i_lv_step_offs"],
 "lambda":["f_o2s_fitted","f_fbk_dis_count","f_o2s_fault","f_o2s_fault_log","f_o2_htr_safe","f_o2h_timer_state","f_o2_htr_status","f_minpw_dis","f_orfco_dis_fbk","f_rich_lean","f_fbk_switch","iq_oxy_htr_req","iq_oxy_htr_tdir","iq_oxy_htr_rmsk"],
 "pmh":["sync_180_flag","sync_360_flag","sync_720_flag","sync_loss_log","cr_seeing_crank_tth","ign_enable"],
 "bobine":["ign_max_ct","ign_min_ct","ign_coil_sc","ign_still_on_er"],
 "injecteurs":["io_inj_1or2","io_inj_sw_off","iq_drv_aa_trip","iq_drv_ab_trip"],
 "pas_a_pas":["stp_reference","stp_locked","stp_moving","stp_hold_current","stp_cur_position","stp_aim_position","stp_aim_pos","i_req_step_pos","i_req_step_spd"],
 "sorties":["io_tripped_ops","io_tripped_log","io_strat_req","io_port3_copy","io_forced_ops","io_force_req","iq_fuel_pump_req","iq_fuel_pump_tdir","iq_fuel_pump_rmsk","iq_purge_req","iq_purge_tdir","iq_purge_rmsk","iq_slow_fan_req","iq_slow_fan_tdir","iq_slow_fan_rmsk","iq_fast_fan_req","iq_fast_fan_tdir","iq_fast_fan_rmsk","iq_aircon_req","iq_aircon_tdir","iq_aircon_rmsk"]
}
SENSORS={"clnt":"temperature_moteur","iat":"temperature_air","map":"map","thp":"papillon","bat":"batterie"}
def txt(p): return p.read_bytes().decode("latin-1") if p and p.exists() else ""
def hx(n): return f"{n:04X}"
def hits(b,p): return [m.start() for m in re.finditer(re.escape(p),b)]
def parse_xf(s):
    ram={}; cal={}; flags={}; section=""; fb=None; bit=0; fbaddr={}; format3="PETA3" in s[:300] or "VOLATILE STORAGE" in s and "ENDIAN =" in s
    m=re.search(r"(?im)^TUNE AREA START ADDRESS\s*=\s*([0-9a-f]+)",s)
    if not m: m=re.search(r"(?im)Tune Area Start[^\n]*\n;\s*([0-9a-f]+)",s)
    tune=int(m[1],16) if m else None
    for no,l in enumerate(s.splitlines(),1):
        l=l.strip()
        if "TUNE CONSTANT INDEX" in l: section="cal";fb=None;continue
        if "VOLATILE STORAGE" in l: section="ram";fb=None;continue
        if "NON VOLATILE" in l or "NON-VOLATILE" in l or "MAP INDEX" in l or "MAP TABLE" in l: section="";fb=None
        q=re.match(r"^;\*<+\s*flagbyte(\d+)\s*>+",l,re.I)
        r=re.match(r"^(flagbyte\d+)\s*,\s*([0-9A-F]{4})$",l,re.I)
        if q or r:
            fb="flagbyte"+q[1] if q else r[1].lower();bit=0
            if r: fbaddr[fb]=int(r[2],16)
            continue
        if fb is not None and bit<8 and l and not l.startswith(";"):
            if l=="%" or l.upper()=="NULL" or "," in l:
                name=l.split(",")[0].strip()
                if name not in ("%","NULL"): flags[name]=(fb,1<<bit,no,l)
                bit+=1
                continue
            fb=None
        if l.startswith(";"):continue
        r=re.match(r"^(\w+)\s+([0-9A-F]{2});([0-9A-F]{2});([BW]);",l,re.I)
        if r:
            name=r[1];ram[name]=(int(r[2]+r[3],16),r[4],no,l)
            if name.startswith("flagbyte"): fbaddr[name]=ram[name][0]
            continue
        r=re.match(r"^(\w+)\s*,\s*([0-9A-F]{4})\s*,\s*([BW])\s*,",l,re.I)
        if r and section in ("cal","ram"):
            d=cal if section=="cal" else ram
            d[r[1]]=(int(r[2],16),r[3],no,l)
            if section=="ram" and r[1].startswith("flagbyte"):fbaddr[r[1]]=int(r[2],16)
            continue
        r=re.match(r"^(\w+)\s+(\d+)\s+;",l)
        if r and section=="cal": cal[r[1]]=(int(r[2]),"B",no,l)
    return tune,ram,cal,{k:(fbaddr.get(f),mask,no,l) for k,(f,mask,no,l) in flags.items()}
def asm_index(s):
    d={}
    for l in s.splitlines():
        m=re.match(r"^\s*(?:[A-Za-z0-9_]+:)?([0-9A-Fa-f]{4,8})\s+([a-z][a-z0-9.]*)\s+([^;]+)",l)
        if m and m[2] not in ("db","dw","dd","ds","equ"):d[int(m[1],16)]=l.strip()
    return d
def flagtext(f):return f"{hx(f[0])}/{f[1]:02X}" if f and f[0] is not None else ""
def write_csv(path,rows):
    with path.open("w",encoding="utf-8-sig",newline="") as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
inventory=[]; sensor_rows=[]; themes=[]; evidence=[]
for p in sorted(ROOT.rglob("*")):
    if not p.is_file() or p.suffix.lower() not in (".hex",".rom",".bin"):continue
    rel=str(p.relative_to(ROOT));family=str(p.relative_to(ROOT).parent);b=p.read_bytes()
    siblings={q.suffix.lower():q for q in p.parent.iterdir() if q.stem.lower()==p.stem.lower()}
    xf=siblings.get(".xf3",siblings.get(".xf1"));asm=siblings.get(".asm")
    tune,ram,cal,flags=parse_xf(txt(xf))
    older=family in ("Rover MEMS 1.3","Rover MEMS 1.6/NA","Rover MEMS 1.6 TURBO","Rover MEMS 1.9")
    # Exact matching of decoded definitions with bytes and indexed producer.
    ins=asm_index(txt(asm)) if older and asm else {}
    inventory.append(dict(path=rel,family=family,firmware=p.stem,sha256=hashlib.sha256(b).hexdigest(),bytes=len(b),xf=xf.name if xf else "",asm=asm.name if asm else "",flags=len(flags),ram_symbols=len(ram),cal_symbols=len(cal),tune=hx(tune) if tune is not None else ""))
    for key,syms in THEMES.items():
        found={};direct={}
        for name in syms:
            if name in flags:
                a,m,no,line=flags[name];found[name]={"address":flagtext(flags[name]),"line":no,"definition":line}
                if older and a is not None and a<=255:
                    # Immediate OR/AND of byte at a direct address; require ASM instruction match.
                    sites=[]
                    for op,v in ((0x91,m),(0x71,255^m)):
                        for pos in hits(b,bytes([op,v,a])):
                            addr=pos+0x2000
                            if addr in ins and re.search(r"\b(?:orb|andb)\b",ins[addr]):sites.append(hx(addr)+":"+ins[addr])
                    if sites: direct[name]=sites
            elif name in ram or name in cal:
                a,t,no,line=(ram if name in ram else cal)[name];found[name]={"address":hx(a),"space":"RAM" if name in ram else "calibration","type":t,"line":no,"definition":line}
        themes.append(dict(path=rel,theme=key,definitions=len(found),symbols=";".join(found),direct_flag_evidence=len(direct),status="SYMBOLS_AND_DIRECT_CODE_CANDIDATES" if direct else "SYMBOLS_ONLY" if found else "UNRESOLVED_NO_MATCHING_DEFINITION",physical_access="NOT_TESTED"))
        if found:evidence.append({"path":rel,"theme":key,"definitions":found,"direct_instruction_candidates":direct})
    for k,theme in SENSORS.items():
        cf=flags.get("a_"+k+"_cur_flt");lf=flags.get("a_"+k+"_log_flt")
        lo=cal.get("a_"+("temp_ip_min" if k in ("clnt","iat") else k+"_min_raw"))
        hi=cal.get("a_"+("temp_ip_max" if k in ("clnt","iat") else k+"_max_raw"))
        backup=cal.get("a_"+k+"_backup"); dest=ram.get("a_filt_"+k)
        status="UNRESOLVED";rows=[];prod=[];proof=""
        if older and cf and lf and cf[0] is not None and lf[0] is not None and all(x is not None for x in (lo,hi,backup,dest)) and cf[0]<=255 and lf[0]<=255:
            prefix=struct.pack("<HHHBB",lo[0],hi[0],backup[0],cf[1],lf[1])
            rows=[i for i in hits(b,prefix) if i+14<=len(b) and int.from_bytes(b[i+12:i+14],"little")==dest[0]]
            for j in range(0,len(b)-7):
                if b[j]==0x93 and b[j+2:j+4]==bytes([6,cf[0]]) and b[j+4]==0x93 and b[j+5]==b[j+1] and b[j+6:j+8]==bytes([7,lf[0]]):
                    # Both mask loads are indexed into the SAME row. Verify both instructions in ASM.
                    if j+0x2000 in ins and j+4+0x2000 in ins and "orb" in ins[j+0x2000] and "orb" in ins[j+4+0x2000]:prod.append(j)
            # Table pointer link: locate an indexed table-base ADD within the same producer routine.
            links=[]
            for i in rows:
                for j in prod:
                    for q in range(max(0,j-180),j):
                        if q+0x2000 not in ins:continue
                        line=ins[q+0x2000]
                        m=re.search(r"\badd\s+[^,]+,\s*#([0-9a-f]+)h\b",line,re.I)
                        if m and (i+0x2000-int(m[1],16)) in range(0,14*16,14):links.append((i,j,q,int(m[1],16)))
            status="TABLE_AND_INDEXED_PRODUCER_LINKED" if len({(i,j,q,t) for i,j,q,t in links})==1 else "TABLE_AND_PRODUCER_CANDIDATE" if rows and prod else "TABLE_CANDIDATE" if rows else "DEFINITION_ONLY"
            proof=";".join(f"row={hx(i+0x2000)} producer={hx(j+0x2000)} table_ref={hx(q+0x2000)} base={hx(t)}" for i,j,q,t in links)
        elif cf: status="DEFINITION_ONLY"
        # Older 1.3 known two-limit code; verify each independently, never reuse other family addresses.
        if family=="Rover MEMS 1.3" and k in ("clnt","iat"):
            aa,ll,ms=(0xB8,0xBD,(0x20,0x40)) if k=="clnt" else (0xB9,0xBE,(2,4))
            cc=[hits(b,bytes([0x91,m,aa,0x91,m,ll])) for m in ms]
            # These ROM dumps may use another base; store file offsets explicitly.
            if all(len(x)==1 for x in cc):
                status="TWO_LIMIT_FLAGS_BINARY_MATCH";proof=";".join(f"file_offset={hx(x[0])} mask={m:02X}" for m,x in zip(ms,cc))
                cf=(aa,ms[0]|ms[1],0,"two distinct bits");lf=(ll,ms[0]|ms[1],0,"two distinct bits")
        def cv(x):
            if not x or tune is None or not older:return ""
            i=tune-0x2000+x[0]
            return b[i] if 0<=i<len(b) else ""
        sensor_rows.append(dict(path=rel,family=family,firmware=p.stem,sensor=k,current=flagtext(cf),stored=flagtext(lf),lower_adc=cv(lo),upper_adc=cv(hi),substitution_raw=cv(backup),filtered_ram=hx(dest[0]) if dest else "",table_rows=";".join(hx(i+0x2000) for i in rows),producer_sites=";".join(hx(i+0x2000) for i in prod),evidence=proof,status=status,diagnostic_access="NOT_TESTED"))
assert len(inventory)==len({r["path"] for r in inventory})
assert len(sensor_rows)==len(inventory)*5 and len(themes)==len(inventory)*len(THEMES)
write_csv(OUT/"inventaire.csv",inventory);write_csv(OUT/"matrice_capteurs.csv",sensor_rows);write_csv(OUT/"matrice_chantiers.csv",themes)
(OUT/"preuves_symboles.json").write_text(json.dumps(evidence,ensure_ascii=False,indent=1),encoding="utf-8")
bilan={"total":len(inventory),"sensor_rows":len(sensor_rows),"theme_rows":len(themes),"families":dict(collections.Counter(r["family"] for r in inventory)),"by_family_sensor":{},"themes":{},"limitations":"Static pass only. Linked rows prove mask and producer pattern, not diagnostic access or complete strategy. No symbol match is unresolved, never incompatible."}
for family in bilan["families"]:
    bilan["by_family_sensor"][family]={k:dict(collections.Counter(r["status"] for r in sensor_rows if r["family"]==family and r["sensor"]==k)) for k in SENSORS}
for k in THEMES:bilan["themes"][k]=dict(collections.Counter(r["status"] for r in themes if r["theme"]==k))
(OUT/"bilan.json").write_text(json.dumps(bilan,ensure_ascii=False,indent=2),encoding="utf-8")
print("BILAN_RECHERCHE "+json.dumps(bilan,ensure_ascii=False))
