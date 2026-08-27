#!/usr/bin/env python3
"""Deterministic self-test for the additive RAVE knowledge foundation.

This validator is intentionally independent from the application UI. It checks
only the packaged r20 SQLite contract introduced by research_enrichment_1730.
It accepts the schema-only foundation stage (0 mirrors) and, once migration is
performed, the complete first migration stage (93 mirrors). Any partial state
fails.
"""

from __future__ import annotations

import argparse
import sqlite3
import sys
from pathlib import Path

EXPECTED_USER_VERSION = 20
EXPECTED_RAVE_FACTS = 93
EXPECTED_EXPERT_FACTS = 105
EXPECTED_HISTORICAL_TABLES = 63

FOUNDATION_TABLES = (
    "mems_applicability_scope",
    "mems_scope_constraint",
    "mems_knowledge_item",
    "mems_knowledge_scope",
    "mems_specification",
    "mems_specification_value",
    "mems_procedure",
    "mems_procedure_step",
    "mems_procedure_requirement",
    "mems_knowledge_relation",
    "mems_term_alias",
)

FOUNDATION_INDEXES = (
    "idx_scope_vehicle",
    "idx_scope_powertrain",
    "idx_scope_constraint_lookup",
    "idx_knowledge_class",
    "idx_knowledge_component",
    "idx_knowledge_legacy",
    "idx_knowledge_scope_scope",
    "idx_spec_lookup",
    "idx_procedure_lookup",
    "idx_alias_lookup",
)


def fail(message: str) -> None:
    raise AssertionError(message)


def scalar(db: sqlite3.Connection, sql: str, params: tuple[object, ...] = ()) -> object:
    row = db.execute(sql, params).fetchone()
    if row is None:
        fail(f"query returned no row: {sql}")
    return row[0]


def require_equal(label: str, actual: object, expected: object) -> None:
    if actual != expected:
        fail(f"{label}: expected {expected!r}, got {actual!r}")


def require_schema_guard(db: sqlite3.Connection) -> None:
    """Verify that the declared foundation constraints really reject bad rows."""
    scratch = sqlite3.connect(":memory:")
    try:
        scratch.execute("PRAGMA foreign_keys=ON")
        for table in FOUNDATION_TABLES:
            row = db.execute(
                "SELECT sql FROM sqlite_master WHERE type='table' AND name=?", (table,)
            ).fetchone()
            if row is None or not row[0]:
                fail(f"missing CREATE TABLE SQL for {table}")
            scratch.execute(row[0])

        try:
            scratch.execute(
                "INSERT INTO mems_knowledge_scope(knowledge_key, scope_key, applicability) "
                "VALUES('missing-knowledge', 'missing-scope', 'applies')"
            )
        except sqlite3.IntegrityError:
            pass
        else:
            fail("foreign-key guard did not reject an invalid knowledge/scope link")

        scratch.execute(
            "INSERT INTO mems_knowledge_item(knowledge_key, domain, knowledge_type, topic, verification_level) "
            "VALUES('duplicate-test', 'selftest', 'selftest', 'selftest', 'non_verifie')"
        )
        try:
            scratch.execute(
                "INSERT INTO mems_knowledge_item(knowledge_key, domain, knowledge_type, topic, verification_level) "
                "VALUES('duplicate-test', 'selftest', 'selftest', 'selftest', 'non_verifie')"
            )
        except sqlite3.IntegrityError:
            pass
        else:
            fail("primary-key guard did not reject a duplicate knowledge_key")
    finally:
        scratch.close()


def require_migrated_contract(db: sqlite3.Connection) -> None:
    require_equal(
        "non-NULL legacy_rave_fact_key mirrors",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item "
                "WHERE legacy_rave_fact_key IS NOT NULL AND trim(legacy_rave_fact_key)<>''",
            )
        ),
        EXPECTED_RAVE_FACTS,
    )
    require_equal(
        "unique legacy_rave_fact_key mirrors",
        int(scalar(db, "SELECT COUNT(DISTINCT legacy_rave_fact_key) FROM mems_knowledge_item")),
        EXPECTED_RAVE_FACTS,
    )
    require_equal(
        "orphan legacy_rave_fact_key mirrors",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "LEFT JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "WHERE r.fact_key IS NULL",
            )
        ),
        0,
    )
    require_equal(
        "source trace mismatches",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "WHERE k.source_key IS NOT r.source_key "
                "OR k.document IS NOT r.document "
                "OR k.topic IS NOT r.topic "
                "OR k.source_text IS NOT r.statement "
                "OR k.source_section IS NOT r.source_section "
                "OR k.verification_level IS NOT r.verification_level "
                "OR k.image_ref IS NOT r.image_ref "
                "OR k.notes IS NOT r.notes",
            )
        ),
        0,
    )
    require_equal(
        "knowledge items without scope",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "LEFT JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "WHERE ks.knowledge_key IS NULL",
            )
        ),
        0,
    )
    require_equal(
        "orphan knowledge/scope links",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_scope ks "
                "LEFT JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE s.scope_key IS NULL",
            )
        ),
        0,
    )

    # Proven incompatibilities must be explicit in the normalized scopes.
    require_equal(
        "Japan-only facts without Japan scope",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE r.variant='SPi_Japan_97MY_from_VIN_SAXXNNAXKBD_134455' "
                "AND s.market IS NOT 'Japan'",
            )
        ),
        0,
    )
    require_equal(
        "MPi facts without MPi scope",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE (r.variant LIKE 'MPi_%') AND s.induction IS NOT 'MPi'",
            )
        ),
        0,
    )
    require_equal(
        "SPi facts without SPi scope",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE (r.variant LIKE 'SPi_%' OR r.variant LIKE 'Mini_SPi_%') "
                "AND s.induction IS NOT 'SPi'",
            )
        ),
        0,
    )

    # "market_non_precise" must remain NULL/unknown, never be promoted to ANY.
    require_equal(
        "unspecified-market facts promoted to a market",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE r.variant LIKE '%market_non_precise%' AND s.market IS NOT NULL",
            )
        ),
        0,
    )
    require_equal(
        "Mini_1997_2000 induction over-inference",
        int(
            scalar(
                db,
                "SELECT COUNT(*) FROM mems_knowledge_item k "
                "JOIN mems_rave_fact r ON r.fact_key=k.legacy_rave_fact_key "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE r.variant='Mini_1997_2000' AND s.induction IS NOT NULL",
            )
        ),
        0,
    )

    require_equal(
        "automatic-only SPi Japan fact transmission",
        str(
            scalar(
                db,
                "SELECT s.transmission FROM mems_knowledge_item k "
                "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                "WHERE k.legacy_rave_fact_key='RAVE-COLOR-SPIJ-019'",
            )
        ),
        "automatic",
    )
    for fact_key in ("RAVE-COLOR-SPIJ-023", "RAVE-COLOR-SPIJ-024"):
        require_equal(
            f"manual-only SPi Japan fact transmission {fact_key}",
            str(
                scalar(
                    db,
                    "SELECT s.transmission FROM mems_knowledge_item k "
                    "JOIN mems_knowledge_scope ks ON ks.knowledge_key=k.knowledge_key "
                    "JOIN mems_applicability_scope s ON s.scope_key=ks.scope_key "
                    "WHERE k.legacy_rave_fact_key=?",
                    (fact_key,),
                )
            ),
            "manual",
        )


def validate(database_path: Path) -> str:
    if not database_path.is_file() or database_path.stat().st_size <= 0:
        fail(f"SQLite database missing or empty: {database_path}")

    db = sqlite3.connect(str(database_path))
    try:
        require_equal("PRAGMA integrity_check", str(scalar(db, "PRAGMA integrity_check")).lower(), "ok")
        require_equal("PRAGMA user_version", int(scalar(db, "PRAGMA user_version")), EXPECTED_USER_VERSION)

        user_tables = {
            row[0]
            for row in db.execute(
                "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'"
            )
        }
        missing_tables = sorted(set(FOUNDATION_TABLES) - user_tables)
        if missing_tables:
            fail("missing foundation tables: " + ", ".join(missing_tables))
        require_equal(
            "user table count",
            len(user_tables),
            EXPECTED_HISTORICAL_TABLES + len(FOUNDATION_TABLES),
        )

        indexes = {
            row[0]
            for row in db.execute(
                "SELECT name FROM sqlite_master WHERE type='index' AND name NOT LIKE 'sqlite_%'"
            )
        }
        missing_indexes = sorted(set(FOUNDATION_INDEXES) - indexes)
        if missing_indexes:
            fail("missing foundation indexes: " + ", ".join(missing_indexes))

        require_equal(
            "historical RAVE fact count",
            int(scalar(db, "SELECT COUNT(*) FROM mems_rave_fact")),
            EXPECTED_RAVE_FACTS,
        )
        require_equal(
            "historical expert fact count",
            int(scalar(db, "SELECT COUNT(*) FROM mems_expert_fact_external")),
            EXPECTED_EXPERT_FACTS,
        )

        mirror_count = int(scalar(db, "SELECT COUNT(*) FROM mems_knowledge_item"))
        if mirror_count == 0:
            phase = "foundation"
            non_empty = []
            for table in FOUNDATION_TABLES:
                count = int(scalar(db, f'SELECT COUNT(*) FROM "{table}"'))
                if count != 0:
                    non_empty.append(f"{table}={count}")
            if non_empty:
                fail("schema-only foundation contains unexpected rows: " + ", ".join(non_empty))
        elif mirror_count == EXPECTED_RAVE_FACTS:
            phase = "migrated"
            require_migrated_contract(db)
        else:
            fail(
                "partial knowledge migration: expected 0 or 93 mems_knowledge_item rows, "
                f"got {mirror_count}"
            )

        require_schema_guard(db)
        return phase
    finally:
        db.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path, help="Path to ia_mems_reference_r20.sqlite")
    args = parser.parse_args()

    try:
        phase = validate(args.database.resolve())
    except (AssertionError, sqlite3.Error, OSError) as exc:
        print(f"RAVE_KNOWLEDGE_FOUNDATION_FAIL: {exc}", file=sys.stderr)
        return 1

    print("RAVE_KNOWLEDGE_FOUNDATION_PASS")
    print(f"RAVE_KNOWLEDGE_PHASE={phase}")
    print(f"RAVE_KNOWLEDGE_TABLES={len(FOUNDATION_TABLES)}")
    print(f"RAVE_KNOWLEDGE_INDEXES={len(FOUNDATION_INDEXES)}")
    print(f"RAVE_HISTORICAL_FACTS={EXPECTED_RAVE_FACTS}")
    print(f"EXPERT_HISTORICAL_FACTS={EXPECTED_EXPERT_FACTS}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
