#!/usr/bin/env python3
"""Deterministic self-test for the additive RAVE knowledge foundation.

This validator is intentionally independent from the application UI.  It checks
only the packaged r20 SQLite contract introduced by research_enrichment_1730.
It accepts the schema-only foundation stage (0 mirrors) and, once migration is
performed, the complete first migration stage (93 mirrors).  Any partial state
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
                int(
                    scalar(
                        db,
                        "SELECT COUNT(DISTINCT legacy_rave_fact_key) FROM mems_knowledge_item",
                    )
                ),
                EXPECTED_RAVE_FACTS,
            )
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
