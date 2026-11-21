#!/usr/bin/env python3
"""Publish a verified SESSION Windows Setup to the public MAJ site.

Invoked as a mandatory LAST step of the Windows build job, on the SAME runner.
Only the Setup + SHA256 are public. No development notes, sources or research.
All operations are idempotent, and missing or mismatched public files are fatal.
"""
import argparse
import base64
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

PUBLIC = "mini56/ECU-Mems-Manager-Session-MAJ"
API = f"repos/{PUBLIC}"
TAG = "mems-session-build-{}"


def run_gh(*args, input_text=None, missing_ok=False):
    env = os.environ.copy()
    if not env.get("GH_TOKEN"):
        raise RuntimeError("SECRET_MANQUANT: SESSION_MAJ_WRITE_TOKEN dans le depot prive")
    result = subprocess.run(
        ["gh", *args], input=input_text, text=True, encoding="utf-8",
        capture_output=True, env=env, check=False
    )
    if result.returncode:
        if missing_ok and "HTTP 404" in result.stderr:
            return None
        raise RuntimeError("ECHEC_GITHUB_MAJ: " + result.stderr[-1500:])
    return result.stdout


def api_get(path, *, missing_ok=False):
    out = run_gh("api", path, missing_ok=missing_ok)
    return json.loads(out) if out is not None else None


def api_send(method, path, data):
    return json.loads(run_gh(
        "api", "-X", method, path, "--input", "-",
        input_text=json.dumps(data, ensure_ascii=False)))


def digest(path):
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(8 * 1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def public_notes(number):
    return (
        "### Français\n\n"
        f"**ECU MEMS Manager SESSION {number}**\n\n"
        "Mise à jour du programme pour Windows.\n"
        "Améliorations et corrections du logiciel.\n\n"
        "Téléchargements : installateur Windows (Setup) et vérification SHA-256.\n\n"
        "---\n\n"
        "### English\n\n"
        f"**ECU MEMS Manager SESSION {number}**\n\n"
        "Windows application update.\n"
        "Software improvements and fixes.\n\n"
        "Downloads: Windows installer (Setup) and SHA-256 verification file.\n"
    )


def release_of(tag):
    # GitHub REST /releases/tags/{tag} returns 404 for existing drafts.
    # Authenticated listing includes both published and draft releases.
    release = api_get(f"{API}/releases/tags/{tag}", missing_ok=True)
    if release is not None:
        return release
    for page in range(1, 11):
        releases = api_get(f"{API}/releases?per_page=100&page={page}")
        for item in releases:
            if item.get("tag_name") == tag:
                return item
        if len(releases) < 100:
            break
    return None


def ensure_public_release(tag, files, number):
    notes = public_notes(number)
    note_file = files[0].parent / f"public-notes-{number}.md"
    note_file.write_text(notes, encoding="utf-8")
    release = release_of(tag)
    if release is None:
        # gh stages both assets before publishing. It tracks the draft by ID.
        run_gh("release", "create", tag, *(str(file) for file in files),
               "--repo", PUBLIC, "--latest", "--title",
               f"ECU MEMS Manager SESSION {number} - Setup Windows",
               "--notes-file", str(note_file))
        for attempt in range(10):
            release = release_of(tag)
            if release is not None:
                break
            time.sleep(3)
        if release is None:
            raise RuntimeError("MAJ_RELEASE_CREATE_NOT_VISIBLE")

    existing = {a["name"]: a for a in release.get("assets", [])}
    for file in files:
        local_sha = digest(file)
        local_size = file.stat().st_size
        remote = existing.get(file.name)
        if remote:
            if (remote.get("size") != local_size or
                    remote.get("digest") != "sha256:" + local_sha):
                raise RuntimeError("MAJ_PUBLIC_ASSET_CONFLICT: " + file.name)
            print("MAJ_ASSET_ALREADY_VERIFIED", file.name)
            continue
        run_gh("release", "upload", tag, str(file), "--repo", PUBLIC)
        print("MAJ_ASSET_UPLOADED", file.name)
    for attempt in range(5):
        release = release_of(tag)
        remote = {a["name"]: a for a in release.get("assets", [])}
        if all(x.name in remote and
               remote[x.name].get("size") == x.stat().st_size and
               remote[x.name].get("digest") == "sha256:" + digest(x)
               for x in files):
            break
        time.sleep(3)
    else:
        raise RuntimeError("MAJ_ASSET_DIGEST_OR_SIZE_NOT_VERIFIED")
    # A release is made public only after both assets match the local files.
    args = ["release", "edit", tag, "--repo", PUBLIC, "--latest",
            "--notes-file", str(note_file)]
    if release.get("draft"):
        args.append("--draft=false")
    run_gh(*args)
    public = release_of(tag)
    if not public or public.get("draft") or public.get("prerelease"):
        raise RuntimeError("MAJ_RELEASE_NOT_PUBLIC")
    print("MAJ_RELEASE_PUBLIC_AND_SHA256_OK", tag)


def decoded_manifest():
    result = api_get(f"{API}/contents/manifest.json?ref=main")
    if not result or not result.get("sha") or result.get("encoding") != "base64":
        raise RuntimeError("MAJ_MANIFEST_CANNOT_BE_READ")
    body = base64.b64decode(result["content"]).decode("utf-8")
    return json.loads(body), result["sha"]


def manifest_number(program):
    url = program.get("release_url", "")
    match = re.search(r"/releases/download/mems-session-build-(\d+)/", url)
    if not match:
        raise RuntimeError("MAJ_MANIFEST_RELEASE_URL_INVALID")
    return int(match.group(1))


def verify_manifest(number, expected_url, expected_sha):
    current, _ = decoded_manifest()
    program = current.get("program", {})
    n = manifest_number(program)
    if n < number:
        raise RuntimeError("MAJ_MANIFEST_NOT_UPDATED")
    if n == number and (program.get("release_url") != expected_url or
                        program.get("sha256") != expected_sha or
                        program.get("version") !=
                        f"1.{number // 100}.{number % 100}" or
                        program.get("available") is not True):
        raise RuntimeError("MAJ_MANIFEST_CONTENT_MISMATCH")
    return n


def update_manifest(number, tag, installer_sha):
    file_name = f"ECU-Mems-Manager-Session-{number}-Setup.exe"
    url = f"https://github.com/{PUBLIC}/releases/download/{tag}/{file_name}"
    version = f"1.{number // 100}.{number % 100}"
    for _ in range(4):
        manifest, blob_sha = decoded_manifest()
        program = manifest.setdefault("program", {})
        current = manifest_number(program)
        if current > number:
            print("MAJ_MANIFEST_NEWER_RELEASE_PRESERVED", current)
            break
        if current == number and (program.get("sha256") != installer_sha or
                                  program.get("release_url") != url):
            raise RuntimeError("MAJ_MANIFEST_SAME_VERSION_CONFLICT")
        if current == number and program.get("version") == version and \
                program.get("available") is True:
            break
        program.update({
            "available": True,
            "version": version,
            "release_url": url,
            "sha256": installer_sha
        })
        encoded = base64.b64encode(
            (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
        ).decode("ascii")
        try:
            api_send("PUT", f"{API}/contents/manifest.json", {
                "message": f"Publier SESSION {number} - Setup Windows et SHA256",
                "content": encoded,
                "sha": blob_sha,
                "branch": "main"
            })
            break
        except RuntimeError as exc:
            # Retry only optimistic concurrent updates of manifest.json.
            if "HTTP 409" not in str(exc):
                raise
            time.sleep(2)
    else:
        raise RuntimeError("MAJ_MANIFEST_CONCURRENT_UPDATE_FAILED")
    for _ in range(5):
        try:
            n = verify_manifest(number, url, installer_sha)
            print("MAJ_MANIFEST_VERIFIED", n)
            return
        except RuntimeError as exc:
            if str(exc) not in ("MAJ_MANIFEST_NOT_UPDATED",):
                raise
            time.sleep(2)
    raise RuntimeError("MAJ_MANIFEST_NOT_VISIBLE")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--number", type=int, required=True)
    parser.add_argument("--setup", type=Path, required=True)
    parser.add_argument("--sha-file", type=Path, required=True)
    args = parser.parse_args()
    number = args.number
    if number < 100 or str(number) != os.environ.get("MEMS_BUILD_NUMBER"):
        raise RuntimeError("MAJ_WRONG_BUILD_NUMBER")
    installer_name = f"ECU-Mems-Manager-Session-{number}-Setup.exe"
    hash_name = f"SESSION-{number}-Setup.sha256"
    if (args.setup.name != installer_name or args.sha_file.name != hash_name or
            not args.setup.is_file() or not args.sha_file.is_file() or
            args.setup.stat().st_size < 100 * 1024 * 1024):
        raise RuntimeError("MAJ_WINDOWS_SETUP_OR_SHA_FILE_MISSING")
    with args.setup.open("rb") as stream:
        if stream.read(2) != b"MZ":
            raise RuntimeError("MAJ_WINDOWS_SETUP_INVALID_MZ")
    checksum = digest(args.setup)
    lines = args.sha_file.read_text(encoding="ascii").strip().splitlines()
    if len(lines) != 1 or lines[0].strip() != f"{checksum}  {installer_name}":
        raise RuntimeError("MAJ_LOCAL_SHA256_FILE_MISMATCH")
    if not os.environ.get("GH_TOKEN"):
        raise RuntimeError("MAJ_WRITE_TOKEN_NOT_CONFIGURED")
    tag = TAG.format(number)
    print("MAJ_PUBLICATION_REQUIRED", tag)
    ensure_public_release(tag, [args.setup, args.sha_file], number)
    update_manifest(number, tag, checksum)
    # Final readback: no green CI while public Release and manifest disagree.
    release = release_of(tag)
    assets = {a["name"]: a for a in release.get("assets", [])}
    if (release.get("draft") or
            any(assets.get(f.name, {}).get("digest") !=
                "sha256:" + digest(f) for f in (args.setup, args.sha_file))):
        raise RuntimeError("MAJ_PUBLICATION_FINAL_VERIFICATION_FAILED")
    print("SESSION_MAJ_PUBLICATION_VERIFIED", number)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print("::error::ECHEC_PUBLICATION_SITE_MAJ:", str(exc), file=sys.stderr)
        sys.exit(1)
