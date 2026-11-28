from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
def read(name):return (root/name).read_text(encoding="utf-8")
names=set(re.findall(r'\{"([A-Z0-9]{8})",',read("mems16na_profiles.inc")))
assert len(names)==123
plans={}
for file in ("mappedinjection_profiles.inc","mappedinjection_profiles_extra.inc"):
    source=read(file)
    for match in re.finditer(r'if\s*\((.*?)\)\s*\{\s*\*profile\s*=\s*InjectionCalculationProfile\(\s*InjectionRamMap\{([^}]+)\},\s*InjectionCalculationKind::(\w+),\s*InjectionCounterPolicy::(\w+),\s*([\d.]+),\s*([\d.]+)\)',source,re.S):
        condition,raw,kind,policy,bscale,tscale=match.groups()
        ids=re.findall(r'id\s*==\s*QStringLiteral\("([A-Z0-9]{8})"\)',condition)
        addr=[int(v.strip().rstrip("uU"),16) for v in raw.split(",")]
        for name in ids:
            plans.setdefault(name,(addr,kind,policy,float(bscale),float(tscale)))
assert names<=set(plans),sorted(names-set(plans))
for n in names:
    (base,source,counter),kind,policy,bscale,tscale=plans[n]
    assert base and source and counter and base%2==source%2==0,n
    assert kind in ("AdditiveC001","IndependentAsync"),n
    assert (kind,policy) in (("AdditiveC001","NonZeroActive"),("IndependentAsync","IncrementEvent")),n
    assert bscale>0 and tscale>0,n
assert 'validatedInjectionProfileForFirmwareNa(firmware,&p)' in read("mappedinjection.cpp")
rapid=read("injectionramtest.cpp")
assert 'plan.counterByte(pair)' in rapid and 'plan.event(counter,havePreviousCounter,previousCounter)' in rapid
assert 'fastReadMode4Word(plan.source' in rapid and 'fastReadMode4Word(plan.base' in rapid
assert 'exactNaReply(response,m_naSession)' in rapid
assert 'isAANMP002Reply' not in rapid
print("SESSION_231_RAPID_INJECTION_123_PROFILE_PLANS_PASS")
