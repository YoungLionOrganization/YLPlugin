# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Publish only assets from a successful same-repository main-branch CI run."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import tarfile
import tempfile


def gh(*arguments, missing=False):
    result = subprocess.run(["gh", *arguments], text=True, capture_output=True)
    if missing and result.returncode and "HTTP 404" in result.stderr:
        return None
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout


def api(path, missing=False):
    result = gh("api", path, missing=missing)
    return None if result is None else json.loads(result)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


assert os.environ.get("GITHUB_ACTIONS") == "true", "use the authorized GitHub publishing workflow"
repository = os.environ["GITHUB_REPOSITORY"]
run_id, tag = os.environ["RELEASE_RUN_ID"], os.environ["RELEASE_TAG"]
assert repository.lower() == "younglionorganization/ylplugin"
assert re.fullmatch(r"[0-9]+", run_id) and re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag)
base = f"repos/{repository}"
run = api(f"{base}/actions/runs/{run_id}")
assert run["status"] == "completed" and run["conclusion"] == "success"
assert run["head_branch"] == "main" and run["path"] == ".github/workflows/ci.yml"
assert run["head_repository"]["full_name"].lower() == repository.lower()
jobs = api(f"{base}/actions/runs/{run_id}/jobs?per_page=100")["jobs"]
assert jobs and all(job["conclusion"] == "success" for job in jobs)
assert {"fuzz", "sanitizers", "package"}.issubset({job["name"] for job in jobs})

root = Path("dist")
manifest = json.loads((root / "release-manifest.json").read_text())
version, commit = manifest["version"], manifest["commit"]
assert tag == "v" + version and commit == run["head_sha"]
assert re.fullmatch(r"[0-9a-f]{40}", commit)
assert manifest["workflow_run"] == f"https://github.com/{repository}/actions/runs/{run_id}"
expected = {"SHA256SUMS", "release-manifest.json", "demo.ylplugin",
            f"ylplugin-{version}-source.tar.gz", f"ylplugin-{version}-py3-none-any.whl"}
assert {path.name for path in root.iterdir()} == expected
checksums = {}
for line in (root / "SHA256SUMS").read_text().splitlines():
    checksum, name = line.split("  ", 1)
    assert name in expected - {"SHA256SUMS"} and name not in checksums
    assert re.fullmatch(r"[0-9a-f]{64}", checksum) and digest(root / name) == checksum
    checksums[name] = checksum
assert set(checksums) == expected - {"SHA256SUMS"}
assert set(manifest["sha256"]) == expected - {"SHA256SUMS", "release-manifest.json"}
for name, checksum in manifest["sha256"].items():
    assert digest(root / name) == checksum

with tarfile.open(root / f"ylplugin-{version}-source.tar.gz") as source:
    notes_member = f"YLPlugin-{version}/docs/releases/{tag}.md"
    if notes_member in source.getnames():
        notes = source.extractfile(notes_member).read().decode()
    else:
        # Release notes may be finalized on main after immutable CI artifacts.
        # Read only the version-specific file from this trusted workflow checkout.
        notes = Path(f"docs/releases/{tag}.md").read_text()
        assert notes.startswith(f"# YLPlugin {tag} ")
notes += f"\n## Verified release build\n\nCommit: `{commit}`\n\n"
notes += f"[All {len(jobs)} CI jobs passed]({manifest['workflow_run']}). Linux x86_64/ARM64, Windows, macOS, sanitizer, fuzz and packaging jobs succeeded. Native execution tests are intentionally skipped on Windows/macOS. See the workflow for precise results and the release manifest for asset hashes.\n"

existing_tag = api(f"{base}/git/ref/tags/{tag}", missing=True)
if existing_tag:
    target = existing_tag["object"]
    while target["type"] == "tag":
        target = api(f"{base}/git/tags/{target['sha']}")["object"]
    assert target["type"] == "commit" and target["sha"] == commit
release = api(f"{base}/releases/tags/{tag}", missing=True)
if release and not release["draft"]:
    assert release["target_commitish"] == commit
    assert {asset["name"] for asset in release["assets"]} == expected
    for asset in release["assets"]:
        assert asset["digest"] == "sha256:" + digest(root / asset["name"])
    print("Already published and verified:", release["html_url"])
    raise SystemExit(0)
if release:
    assert release["draft"] and release["target_commitish"] == commit
with tempfile.TemporaryDirectory() as temporary:
    notes_path = Path(temporary) / "notes.md"
    notes_path.write_text(notes)
    if not release:
        gh("release", "create", tag, "--repo", repository, "--target", commit,
           "--title", f"YLPlugin {tag} — experimental", "--notes-file", str(notes_path), "--draft")
    gh("release", "upload", tag, "--repo", repository, "--clobber", *[str(root / name) for name in sorted(expected)])
    draft = api(f"{base}/releases/tags/{tag}", missing=True)
    if draft is None:
        # Draft tags are not created until publication; resolve the caller's draft by listing.
        drafts = [item for item in api(f"{base}/releases") if item["tag_name"] == tag]
        assert len(drafts) == 1
        draft = drafts[0]
    assert draft["draft"] and draft["target_commitish"] == commit
    assert {asset["name"] for asset in draft["assets"]} == expected
    for asset in draft["assets"]:
        assert asset["digest"] == "sha256:" + digest(root / asset["name"])
    gh("release", "edit", tag, "--repo", repository, "--draft=false", "--latest")
    published = api(f"{base}/releases/tags/{tag}")
    assert not published["draft"] and not published["prerelease"] and published["published_at"]
    print("Published:", published["html_url"])
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as stream:
            stream.write(f"Published [{tag}]({published['html_url']}) from `{commit}` with five verified assets.\n")
