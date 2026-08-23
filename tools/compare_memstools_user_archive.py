#!/usr/bin/env python3
"""Compare Andrew's current MEMSTools ZIP with the user's archived 10.29 ZIP baseline.

Produces a provenance/comparison qz64 batch and a human-readable audit. The old
archive itself is not committed; only hashes, counts and conservative technical
findings are preserved.
"""
from __future__ import annotations
import argparse, csv, hashlib, io, json, re, zipfile
from pathlib import Path
from build_memstools_enrichment import parse_ini_sections, sqlq, add_multi, write_qz64, sha256_file

SOURCE_KEY='andrew_revill_memstools_user_archive_10_29'
OUTPUT='research_enrichment_1630.qz64'
FAMILIES={
 '1.3':('RoverMems13','standard'),
 '1.6 NA':('RoverMems16Na','NA'),
 '1.6 Turbo':('RoverMems16Turbo','Turbo'),
 '1.9':('RoverMems19','standard'),
}

def technical_kind(name:str):
    lo=name.lower()
    if name in ('Executables/MemsMapper.RoverMems19.def','Executables/MemsMapper.RoverMems19.dim'):
        return 'definition'
    if 'correlations/' in lo and any(x in lo for x in ('rovermems13','rovermems16na','rovermems16turbo','rovermems19')):
        return 'correlation'
    if 'disassemblies/rover mems 1.3/' in lo or 'disassemblies/rover mems 1.6/' in lo or 'disassemblies/rover mems 1.9/' in lo:
        return 'disassembly'
    if re.search(r'(?i)Firmwares/Rover MEMS 1\.(3|6|9)',name):
        return 'firmware_image'
    return None

def sha(data:bytes): return hashlib.sha256(data).hexdigest()

def semantic_counts(zf:zipfile.ZipFile):
    fwmeta={}
    for label,(suffix,variant) in FAMILIES.items():
        p=f'Executables/Correlations/Firmware.{suffix}.csv'
        header=next(csv.reader(io.StringIO(zf.read(p).decode('utf-8-sig',errors='replace'))))
        fam='1.6' if label.startswith('1.6') else label
        for fw in header[2:]:
            if fw: fwmeta[fw]=(fam,variant)
    relevant=set(fwmeta)
    text=zf.read('Executables/MemsMapper.RoverMems19.def').decode('utf-8-sig',errors='replace')
    sections=parse_ini_sections(text); smap=dict(sections)
    scalar_bind=[]; table_bind=[]; scalar_classes=set(); table_classes=set()
    for sec,props in sections:
        if sec.startswith('Scalar.'):
            rest=sec[7:]
            if '.' not in rest: continue
            fw,addr=rest.split('.',1)
            if fw in relevant and props.get('ScalarClass'):
                scalar_bind.append((fw,addr)); scalar_classes.add(props['ScalarClass'])
        elif sec.startswith('Table.'):
            rest=sec[6:]
            if '.' not in rest: continue
            fw,idx=rest.split('.',1)
            if fw in relevant and props.get('TableClass'):
                table_bind.append((fw,idx)); table_classes.add(props['TableClass'])
    axis=set()
    for c in scalar_classes:
        p=smap.get('ScalarClass.'+c,{})
        if p.get('AxisClass'): axis.add(p['AxisClass'])
    for c in table_classes:
        p=smap.get('TableClass.'+c,{})
        for k in ('XAxis.AxisClass','YAxis.AxisClass','ZAxis.AxisClass'):
            if p.get(k): axis.add(p[k])
    vars_n=0
    for label,(suffix,variant) in FAMILIES.items():
        p=f'Executables/Correlations/Variables.{suffix}.csv'
        rows=list(csv.reader(io.StringIO(zf.read(p).decode('utf-8-sig',errors='replace'))))
        if not rows: continue
        for row in rows[1:]:
            vars_n += sum(1 for x in row[2:] if x)
    return {
      'mems_firmware_catalog':len(relevant),
      'mems_axis_definition':len(axis),
      'mems_scalar_definition':len(scalar_classes),
      'mems_scalar_binding':len(scalar_bind),
      'mems_table_definition':len(table_classes),
      'mems_table_binding':len(table_bind),
      'mems_variable_correlation':vars_n,
    }

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('current_zip',type=Path); ap.add_argument('baseline',type=Path); ap.add_argument('--output-dir',type=Path,default=Path('database/reference')); args=ap.parse_args()
    base=json.loads(args.baseline.read_text(encoding='utf-8'))
    old={x['p']:x for x in base['core_assets']}
    cur_sha=sha256_file(args.current_zip)
    with zipfile.ZipFile(args.current_zip) as zf:
        current={}
        groups={'disassembly':[],'firmware_image':[]}
        for n in zf.namelist():
            if n.endswith('/'): continue
            k=technical_kind(n)
            if not k: continue
            d=zf.read(n); rec={'p':n,'k':k,'s':len(d),'h':sha(d)}; current[n]=rec
            if k in groups: groups[k].append(rec)
        comparisons=[]
        for p,o in old.items():
            c=current.get(p)
            if c is None: status='missing_current'
            elif o['h']==c['h']: status='identical'
            else: status='changed'
            comparisons.append((SOURCE_KEY,p,o['k'],o['s'],None if c is None else c['s'],o['h'],None if c is None else c['h'],status))
        for kind,items in groups.items():
            h=hashlib.sha256()
            for r in sorted(items,key=lambda x:x['p']): h.update(f"{r['p']}\0{r['s']}\0{r['h']}\n".encode())
            oldg=base['group_manifests'][kind]; curh=h.hexdigest(); status='identical' if oldg['count']==len(items) and oldg['sha256']==curh else 'changed'
            comparisons.append((SOURCE_KEY,'@group/'+kind,kind,oldg['count'],len(items),oldg['sha256'],curh,status))
        sc=semantic_counts(zf)
    old_sc=base['semantic_counts']
    counts={s:sum(1 for r in comparisons if r[-1]==s) for s in ('identical','changed','missing_current')}
    kind_stats={}
    for kind in ('correlation','definition','disassembly','firmware_image'):
        rs=[r for r in comparisons if r[2]==kind]
        kind_stats[kind]={s:sum(1 for r in rs if r[-1]==s) for s in ('identical','changed','missing_current')}
    semantic_equal=all(sc.get(k)==old_sc.get(k) for k in sc)
    data_core_exact=kind_stats['correlation']['changed']==0 and kind_stats['correlation']['missing_current']==0 and kind_stats['definition']['changed']==0 and kind_stats['definition']['missing_current']==0

    facts=[
      ('mems13_abem_c8_service5','1.3','ABEMR002','injection_timing','0x00C8 is loaded into the timing parameter and passed to service 5 on the analysed normal path.','source_externe','Strong disassembly evidence; not yet a universal/final live injection mapping.'),
      ('mems13_kbe_c6_service5','1.3','KBE6R003','injection_timing','0x00C6 is loaded into the timing parameter and passed to service 5 on an analysed normal path.','source_externe','Final mapping remains incomplete because other paths/variables must be resolved.'),
      ('mems13_kbe_c100_correction','1.3','KBE6R003','injection_timing_correction','RAM_C100 contributes as a correction centred on 0x8000 before service 5 under the analysed condition.','source_externe','Physical meaning is not proven; do not label as transient, battery or injector compensation.'),
      ('mems13_mge_c6_service5','1.3','MGE7R002','injection_timing','0x00C6 is used on the analysed path around 0xA790 before service 5.','source_externe','Other service-5 paths remain to be resolved before a final mapping.'),
      ('mems13_mge_c0e2_correction','1.3','MGE7R002','injection_timing_correction','RAM_C0E2 contributes as a correction centred on 0x8000 on the analysed path around 0xA790.','source_externe','Physical meaning is not proven.'),
      ('mems13_corr_c8','1.3','ABEMR002/KBE6R003','variable_correlation','Andrew correlation data links 0x00C8 between ABEMR002 and KBE6R003.','source_externe','Correlation alone does not prove identical runtime function.'),
      ('mems13_corr_c6','1.3','KBE6R003/MGE7R002','variable_correlation','Andrew correlation data links 0x00C6 between KBE6R003 and MGE7R002.','source_externe','Correlation alone does not prove a universal injection mapping.'),
      ('mems13_timing_formula','1.3',None,'timing_conversion','The analysed timing service uses a form equivalent to source_duration * 10000 / calibration[0x0C].','source_externe','Final conversion to milliseconds must still be validated per firmware/calibration.'),
      ('mems13_abem_approx_scale','1.3','ABEMR002','timing_conversion','A working estimate from the recorded ABEMR002 calibration is about 0.0015 ms per 0x00C8 unit.','non_verifie','Keep only as a research checkpoint; do not use as a final expert-engine conversion.'),
      ('mems13_no_universal_c6_c8','1.3',None,'safety_rule','Do not treat 0x00C6 or 0x00C8 as universal across MEMS 1.3; firmware-specific path validation remains required.','source_externe','Explicit safety rule from the comparative disassembly work.'),
      ('mems12_no_explicit_correlation_tables','1.2',None,'coverage_gap','The archived 10.29 package has no explicit Firmware/Scalars/Tables/Variables RoverMems12 correlation CSV set.','source_externe','Do not invent MEMS 1.2 correlated addresses.'),
      ('mems1x_d1_optional','1.2',None,'identification','The archived tool contains a Diagnostic frame D1 handler not found condition; D1 absence must not by itself invalidate an ECU session.','source_externe','Consistent with keeping D0 as fallback and D1 optional where unsupported.'),
    ]
    lines=['BEGIN TRANSACTION;',
      'CREATE TABLE IF NOT EXISTS mems_archive_snapshot_external(source_key TEXT PRIMARY KEY,title TEXT,archive_sha256 TEXT,file_count INTEGER,mapper_version TEXT,verification_level TEXT,scope TEXT,compared_current_sha256 TEXT,notes TEXT);',
      'CREATE TABLE IF NOT EXISTS mems_archive_asset_comparison_external(source_key TEXT NOT NULL,relative_path TEXT NOT NULL,file_kind TEXT,old_size INTEGER,current_size INTEGER,old_sha256 TEXT,current_sha256 TEXT,status TEXT NOT NULL,PRIMARY KEY(source_key,relative_path));',
      'CREATE TABLE IF NOT EXISTS mems_expert_fact_external(source_key TEXT NOT NULL,fact_key TEXT NOT NULL,family TEXT,firmware_code TEXT,topic TEXT,statement TEXT,verification_level TEXT,notes TEXT,PRIMARY KEY(source_key,fact_key));',
      f'DELETE FROM mems_archive_snapshot_external WHERE source_key={sqlq(SOURCE_KEY)};',
      f'DELETE FROM mems_archive_asset_comparison_external WHERE source_key={sqlq(SOURCE_KEY)};',
      f'DELETE FROM mems_expert_fact_external WHERE source_key={sqlq(SOURCE_KEY)};']
    snap=(SOURCE_KEY,'User-supplied MEMSTools 10.29 archive',base['archive_sha256'],base['archive_file_count'],base['mapper_version'],'source_externe','MEMS 1.2 / 1.3 / 1.6 / 1.9',cur_sha,'Historical snapshot retained by hash; technical assets compared byte-for-byte with the current Andrew archive. No raw third-party binaries are bundled.')
    lines.append('INSERT INTO mems_archive_snapshot_external VALUES('+','.join(sqlq(v) for v in snap)+');')
    add_multi(lines,'mems_archive_asset_comparison_external',['source_key','relative_path','file_kind','old_size','current_size','old_sha256','current_sha256','status'],comparisons,batch=120)
    add_multi(lines,'mems_expert_fact_external',['source_key','fact_key','family','firmware_code','topic','statement','verification_level','notes'],[(SOURCE_KEY,)+x for x in facts],batch=50)
    lines.append('COMMIT;')
    args.output_dir.mkdir(parents=True,exist_ok=True); write_qz64(args.output_dir/OUTPUT,lines)

    report=args.output_dir/'audits'/'memstools_user_archive_10_29_comparison.md'; report.parent.mkdir(parents=True,exist_ok=True)
    md=['# Comparaison MEMSTools utilisateur 10.29 vs archive Andrew courante','',f'- Ancien ZIP SHA-256 : `{base["archive_sha256"]}`',f'- Ancien ZIP : **{base["archive_file_count"]} fichiers**',f'- Ancien MemsMapper : **{base["mapper_version"]}**',f'- Archive Andrew courante SHA-256 : `{cur_sha}`','', '## Comparaison technique byte-par-byte','',f'- Fichiers cœur comparés individuellement : **{len(old)}**',f'- Assets techniques courants détectés : **{len(current)}**',f'- Identiques : **{counts["identical"]}**',f'- Modifiés : **{counts["changed"]}**',f'- Absents du courant : **{counts["missing_current"]}**','']
    for k,v in kind_stats.items(): md.append(f'- {k}: '+', '.join(f'{s}={n}' for s,n in v.items()))
    md += ['', '## Volumes sémantiques', '', '| Table | 10.29 | Courant | Identique |','|---|---:|---:|:---:|']
    for k in sc: md.append(f'| `{k}` | {old_sc.get(k,"-")} | {sc[k]} | {"oui" if old_sc.get(k)==sc[k] else "non"} |')
    md += ['',f'**Volumes sémantiques identiques : {"OUI" if semantic_equal else "NON"}.**',f'**Corpus corrélations + DEF/DIM byte-identique : {"OUI" if data_core_exact else "NON"}.**','']
    diffs=[r for r in comparisons if r[-1] != 'identical']
    if diffs:
        md += ['## Fichiers techniques différents','','| Statut | Type | Fichier |','|---|---|---|']
        for r in diffs: md.append(f'| {r[-1]} | {r[2]} | `{r[1]}` |')
    else: md += ['## Fichiers techniques différents','','Aucun.']
    md += ['', '## Connaissances historiques conservées', '', '- MEMS 1.3 : chemins service 5 ABEMR002/KBE6R003/MGE7R002 et corrections candidates conservés avec provenance.', '- Les corrélations C8/C6 ne sont pas promues en mapping universel.', '- La conversion ABEM approximative 0.0015 ms/unité reste `non_verifie`.', '- MEMS 1.2 : absence de corpus de corrélation explicite conservée comme lacune réelle.', '', 'Aucune donnée de cette archive historique n’écrase les faits `decoded_by_project` ni les mesures ECU réelles.']
    report.write_text('\n'.join(md)+'\n',encoding='utf-8')
    meta={'source_key':SOURCE_KEY,'old_archive_sha256':base['archive_sha256'],'current_archive_sha256':cur_sha,'core_assets_old':len(old),'technical_assets_current':len(current),'status_counts':counts,'kind_stats':kind_stats,'semantic_counts_old':old_sc,'semantic_counts_current':sc,'semantic_counts_equal':semantic_equal,'data_core_exact':data_core_exact,'expert_facts':len(facts)}
    (args.output_dir/'audits'/'memstools_user_archive_10_29_comparison.json').write_text(json.dumps(meta,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(json.dumps(meta,ensure_ascii=False))

if __name__=='__main__': main()
