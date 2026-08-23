#!/usr/bin/env python3
from pathlib import Path

# Deterministic patch used only on the lab-expert-engine branch.
path = Path('iamemstab.cpp')
text = path.read_text(encoding='utf-8')
original = text

old = '''    observation.values.insert(QStringLiteral("short_term_trim_pct"), static_cast<int>(data->short_term_fuel_trim) - 100.0);\n    observation.values.insert(QStringLiteral("long_term_trim_raw"), data->long_term_fuel_trim);\n    observation.values.insert(QStringLiteral("ignition_advance_deg"), data->ignition_advance * 0.5 - 24.0);\n'''
new = '''    observation.values.insert(QStringLiteral("short_term_trim_pct"), static_cast<int>(data->short_term_fuel_trim) - 100.0);\n    observation.values.insert(QStringLiteral("long_term_trim_raw"), data->long_term_fuel_trim);\n    observation.values.insert(QStringLiteral("long_term_trim_pct"), static_cast<int>(data->long_term_fuel_trim) - 128.0);\n    observation.values.insert(QStringLiteral("ignition_advance_deg"), data->ignition_advance * 0.5 - 24.0);\n'''
if old not in text and 'long_term_trim_pct' not in text:
    raise SystemExit('fuel trim observation anchor not found')
text = text.replace(old, new, 1)

old = '''    observation.values.insert(QStringLiteral("iac_position"), data->iac_position);\n    observation.values.insert(QStringLiteral("idle_error_raw"), data->idle_error);\n    observation.values.insert(QStringLiteral("closed_loop"), data->closed_loop != 0 ? 1.0 : 0.0);\n'''
new = '''    observation.values.insert(QStringLiteral("iac_position"), data->iac_position);\n    observation.values.insert(QStringLiteral("idle_error_raw"), data->idle_error);\n    const int rawHotIdleError = (static_cast<int>(data->idle_error2) << 8) | static_cast<int>(data->uk10);\n    const int hotIdleCorrection = static_cast<int>(data->idle_hot) - 35;\n    observation.values.insert(QStringLiteral("idle_error_hot_corrected"),\n                              (rawHotIdleError - 32768) + hotIdleCorrection);\n    observation.values.insert(QStringLiteral("uk3_raw"), data->uk3);\n    observation.values.insert(QStringLiteral("closed_loop"), data->closed_loop != 0 ? 1.0 : 0.0);\n'''
if old not in text and 'idle_error_hot_corrected' not in text:
    raise SystemExit('idle observation anchor not found')
text = text.replace(old, new, 1)

old = '''    observation.values.insert(QStringLiteral("throttle_pot_raw"), data->throttle_pot);\n    observation.values.insert(QStringLiteral("fault_mask"),\n                              static_cast<double>(static_cast<quint32>(data->dtc0)\n                              | (static_cast<quint32>(data->dtc1) << 8)\n                              | (static_cast<quint32>(data->dtc2) << 16)));\n'''
new = '''    observation.values.insert(QStringLiteral("throttle_pot_raw"), data->throttle_pot);\n    observation.values.insert(QStringLiteral("lambda_fault_active"),\n                              ((data->dtc2 & 0x04) || (data->dtc2 & 0x08)) ? 1.0 : 0.0);\n    observation.values.insert(QStringLiteral("tps_fault_active"),\n                              ((data->dtc1 & 0x80) || (data->dtc2 & 0x01)) ? 1.0 : 0.0);\n    observation.values.insert(QStringLiteral("fault_mask"),\n                              static_cast<double>(static_cast<quint32>(data->dtc0)\n                              | (static_cast<quint32>(data->dtc1) << 8)\n                              | (static_cast<quint32>(data->dtc2) << 16)));\n'''
if old not in text and 'lambda_fault_active' not in text:
    raise SystemExit('fault observation anchor not found')
text = text.replace(old, new, 1)

if text == original:
    print('IA expert observations already patched')
else:
    path.write_text(text, encoding='utf-8', newline='\n')
    print('IA expert observations patched')
