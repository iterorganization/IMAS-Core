#!/usr/bin/env python3
"""Sanity check for the GitHub-hosted macOS runner images we build on.

Sources:
  * .github/workflows/*.yml  -> the macOS runner labels we use
  * actions/runner-images README -> images currently published, and which are
    marked deprecated/preview, plus retirement callouts
  * actions/runner-images Announcement issues (label "OS: macOS") -> lifecycle
    announcements, with their target dates and affected images, for history

Reports everything in the GitHub step summary. With --apply it rewrites the
labels that have fallen behind to the oldest supported macOS image, plus the
matching MACOSX_DEPLOYMENT_TARGET values, so the workflow can turn that into a
pull request.

Exit status: 1 if the sources could not be parsed (format drift) or no
supported image is left, 0 otherwise.
"""

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

README_URL = "https://raw.githubusercontent.com/actions/runner-images/main/README.md"
ISSUES_URL = (
    "https://api.github.com/repos/actions/runner-images/issues"
    "?labels="
    + urllib.parse.quote("Announcement,OS: macOS")
    + "&state=all&per_page=100"
)
ISSUES_MAX_PAGES = 5

MACOS_LABEL = re.compile(r"macos-(\d+)((?:-[a-z0-9]+)*)$")
WORKFLOW_LABEL = re.compile(r"\b(?:runner|runs-on)\s*:\s*[\"']?(macos-[A-Za-z0-9._-]+)")
README_LABEL = re.compile(r"macos-\d+(?:-[a-z0-9]+)*")
RETIREMENT = re.compile(
    r"All macOS (\d+) images? retired[^.\n]*? on "
    r"([A-Z][a-z]+ \d{1,2}(?:st|nd|rd|th)?,? \d{4})",
    re.IGNORECASE,
)
# Image-level lifecycle only: tool removals ("Node.js will be removed",
# "Deprecation of 4 tools") also tick runner images but are not retirements.
LIFECYCLE = re.compile(r"retir|unsupported|end[- ]of[- ]life|\bEOL\b", re.I)
DEPRECATION = re.compile(r"deprecat", re.I)
IMAGE = re.compile(r"image", re.I)
DATE = r"[A-Z][a-z]+ \d{1,2}(?:st|nd|rd|th)?,? \d{4}"
# Homebrew dependencies are built for the runner's macOS, so the wheels break at
# the delocate step if the target stays behind the runner image we build on.
TARGET_ENV = re.compile(r"(MACOSX_DEPLOYMENT_TARGET\s*[=:]\s*[\"']?)(\d+)(?:\.(\d+))?")


def die(message):
    print("error: " + message, file=sys.stderr)
    sys.exit(1)


def http_get(url, token=None):
    request = urllib.request.Request(url, headers={"User-Agent": "imas-core-ci"})
    if token and url.startswith("https://api.github.com/"):
        request.add_header("Authorization", "Bearer " + token)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.read().decode("utf-8")
    except urllib.error.HTTPError as error:
        raise RuntimeError("%s -> HTTP %s" % (url, error.code))
    except urllib.error.URLError as error:
        raise RuntimeError("%s -> %s" % (url, error.reason))


def workflow_files(root):
    directory = os.path.join(root, ".github", "workflows")
    if not os.path.isdir(directory):
        die("no .github/workflows directory under %r" % root)
    names = sorted(
        name
        for name in os.listdir(directory)
        if name.endswith((".yml", ".yaml"))
    )
    return [os.path.join(directory, name) for name in names]


def used_labels(root):
    """label -> sorted list of "file:line" where it appears."""
    found = {}
    for path in workflow_files(root):
        relative = os.path.relpath(path, root)
        with open(path, encoding="utf-8") as handle:
            for number, line in enumerate(handle, 1):
                if line.lstrip().startswith("#"):
                    continue
                for label in WORKFLOW_LABEL.findall(line):
                    found.setdefault(label, []).append("%s:%d" % (relative, number))
    return found


def parse_readme(text):
    """Return (rows, retired) from the Available Images table and callouts.

    rows: list of dicts with version, labels, deprecated, preview, issue.
    retired: {version(int): "retired on ..."} from the retirement callout.
    """
    rows = []
    for line in text.splitlines():
        if not line.startswith("|"):
            continue
        labels = README_LABEL.findall(line.lower())
        if not labels:
            continue
        lower = line.lower()
        issue = re.search(r"/issues/(\d+)", line)
        rows.append(
            {
                "version": int(MACOS_LABEL.match(labels[0]).group(1)),
                "labels": sorted(set(labels)),
                "deprecated": "deprecated" in lower,
                "preview": "preview" in lower,
                "issue": int(issue.group(1)) if issue else None,
            }
        )
    retired = {}
    for match in RETIREMENT.finditer(text):
        retired[int(match.group(1))] = match.group(2)
    return rows, retired


def section(body, name):
    match = re.search(
        r"^#{2,4}\s*" + re.escape(name) + r"\s*$(.*?)(?=^#{2,4}\s|\Z)",
        body,
        re.MULTILINE | re.DOTALL,
    )
    return match.group(1) if match else ""


def checked_date(block):
    match = re.search(r"GitHub Actions[^\n:]*:\s*(" + DATE + ")", block)
    if match:
        return match.group(1)
    match = re.search(r"GitHub[^\n:]*-[ \t]*(" + DATE + ")", block)
    return match.group(1) if match else None


def fetch_issues(token):
    issues = []
    for page in range(1, ISSUES_MAX_PAGES + 1):
        payload = json.loads(http_get("%s&page=%d" % (ISSUES_URL, page), token))
        if not isinstance(payload, list):
            raise RuntimeError("unexpected issues payload: %r" % (payload,))
        issues.extend(payload)
        if len(payload) < 100:
            return issues, False
    return issues, True


def lifecycle_issues(raw, wanted_versions):
    """Announcements that mention retirement/deprecation of images we use."""
    found = []
    for item in raw:
        if item.get("pull_request"):
            continue
        body = item.get("body") or ""
        title = item.get("title") or ""
        if not (LIFECYCLE.search(title) or (DEPRECATION.search(title) and IMAGE.search(title))):
            continue
        if not re.search(r"-\s*\[[xX]\]\s*GitHub Actions", section(body, "Platforms affected")):
            continue
        affected = sorted(
            {
                int(match.group(1))
                for match in re.finditer(
                    r"-\s*\[[xX]\]\s*macOS\s+(\d+)", section(body, "Runner images affected")
                )
            }
        )
        if not affected or not set(affected) & wanted_versions:
            continue
        found.append(
            {
                "number": item["number"],
                "title": title,
                "state": item.get("state", "?"),
                "affected": affected,
                "date": checked_date(section(body, "Target date")),
                "url": item.get("html_url", ""),
                "updated": (item.get("updated_at") or "")[:10],
            }
        )
    return found


def supported(rows, retired):
    """version -> labels of the rows that are neither deprecated, preview nor retired."""
    result = {}
    for row in rows:
        version = row["version"]
        if version in retired or row["deprecated"] or row["preview"]:
            continue
        result.setdefault(version, set()).update(row["labels"])
    return result


def bump_target(label, version, oldest, labels_by_version):
    """Replacement label for a stale one, or (None, reason)."""
    suffix = MACOS_LABEL.match(label).group(2)
    candidate = "macos-%d%s" % (oldest, suffix)
    if candidate in labels_by_version.get(oldest, set()):
        return candidate, None
    plain = "macos-%d" % oldest
    if plain in labels_by_version.get(oldest, set()):
        return plain, "no %r for %d, keeping the plain label" % (candidate, oldest)
    return None, "no supported label for macOS %d" % oldest


def rewrite(root, replacements):
    """Apply label and deployment-target replacements.

    Returns (changed files, number of deployment-target values rewritten).
    """
    changed = []
    target_count = 0
    label_pattern = re.compile(
        r"\b((?:runner|runs-on)\s*:\s*[\"']?)(macos-[A-Za-z0-9._-]+)"
    )
    version_map = {
        int(MACOS_LABEL.match(old).group(1)): int(MACOS_LABEL.match(new).group(1))
        for old, new in replacements.items()
    }

    def target_replacement(match):
        nonlocal target_count
        major = int(match.group(2))
        if major not in version_map:
            return match.group(0)
        target_count += 1
        return match.group(1) + "%d.%s" % (version_map[major], match.group(3) or "0")

    paths = list(workflow_files(root))
    pyproject = os.path.join(root, "pyproject.toml")
    if os.path.exists(pyproject):
        paths.append(pyproject)

    for path in paths:
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        updated = label_pattern.sub(
            lambda match: match.group(1) + replacements.get(match.group(2), match.group(2)),
            text,
        )
        updated = TARGET_ENV.sub(target_replacement, updated)
        if updated != text:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(updated)
            changed.append(os.path.relpath(path, root))
    return changed, target_count


def render(state):
    lines = []
    lines.append("## Runner images")
    lines.append("")
    lines.append("### macOS runners used in this repository")
    lines.append("")
    if state["used"]:
        lines.append("| Label | macOS | Used at |")
        lines.append("| --- | --- | --- |")
        for label in sorted(state["used"]):
            version = MACOS_LABEL.match(label).group(1)
            lines.append(
                "| `%s` | %s | %s |" % (label, version, "<br>".join(state["used"][label]))
            )
    else:
        lines.append("_No macOS runner labels found in `.github/workflows`._")
    lines.append("")

    lines.append("### macOS runners currently published")
    lines.append("")
    lines.append("| macOS | Labels | Status |")
    lines.append("| --- | --- | --- |")
    for version in sorted(state["rows_by_version"]):
        labels = ", ".join("`%s`" % label for label in sorted(state["rows_by_version"][version]))
        status = "supported"
        if version in state["retired"]:
            status = "retired (%s)" % state["retired"][version]
        elif any(
            row["deprecated"]
            for row in state["rows"]
            if row["version"] == version
        ):
            status = "deprecated"
        elif all(row["preview"] for row in state["rows"] if row["version"] == version):
            status = "preview"
        lines.append("| %d | %s | %s |" % (version, labels, status))
    lines.append("")

    lines.append("### Lifecycle announcements affecting our images")
    lines.append("")
    if state["issues"]:
        lines.append("| Issue | State | Affected macOS | Target date |")
        lines.append("| --- | --- | --- | --- |")
        for issue in state["issues"]:
            lines.append(
                "| [#%d](%s) %s | %s | %s | %s |"
                % (
                    issue["number"],
                    issue["url"],
                    issue["title"].replace("|", "\\|"),
                    issue["state"],
                    ", ".join(str(v) for v in issue["affected"]),
                    issue["date"] or "unknown",
                )
            )
    else:
        lines.append("_No retirement or deprecation announcement found for our images._")
    lines.append("")

    lines.append("### Verdict")
    lines.append("")
    lines.append("Oldest supported macOS image: `%s`." % state["oldest_label"])
    for note in state["notes"]:
        lines.append("")
        lines.append("- " + note)
    if not state["notes"]:
        lines.append("")
        lines.append("- Every macOS label we use is supported.")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--apply", action="store_true", help="rewrite stale labels")
    parser.add_argument("--summary", help="append the markdown report to this file")
    parser.add_argument("--report-out", help="write the markdown report to this file")
    parser.add_argument("--github-output", help="write step outputs to this file")
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")

    used = used_labels(args.repo_root)
    if not used:
        die("no macOS runner labels found; the label regex or the workflows changed")

    try:
        readme = http_get(README_URL, token)
    except RuntimeError as error:
        die("cannot read the runner-images README: %s" % error)
    rows, retired = parse_readme(readme)
    if not rows:
        die("no macOS image rows parsed from the runner-images README (format drift?)")

    supported_versions = supported(rows, retired)
    if not supported_versions:
        die("no supported macOS image found in the runner-images README")

    wanted = {int(MACOS_LABEL.match(label).group(1)) for label in used}
    try:
        raw_issues, truncated = fetch_issues(token)
    except RuntimeError as error:
        die("cannot read the runner-images issues: %s" % error)
    issues = lifecycle_issues(raw_issues, wanted)

    oldest = min(supported_versions)
    labels_by_version = {}
    for row in rows:
        labels_by_version.setdefault(row["version"], set()).update(row["labels"])

    replacements = {}
    notes = []
    for label in sorted(used):
        version = int(MACOS_LABEL.match(label).group(1))
        if version in supported_versions:
            if version != oldest:
                notes.append(
                    "`%s` is supported, but `macos-%d` is the oldest supported image; "
                    "building on it would keep the wheels runnable on macOS %d and %d."
                    % (label, oldest, oldest, version - 1)
                )
            continue
        reason = "retired (%s)" % retired[version] if version in retired else "not published"
        target, why = bump_target(label, version, oldest, labels_by_version)
        if target is None:
            notes.append("`%s` is %s and cannot be bumped: %s." % (label, reason, why))
            continue
        replacements[label] = target
        notes.append(
            "`%s` is %s; bumping to `%s`." % (label, reason, target)
            + (" " + why + "." if why else "")
        )

    if args.apply and replacements:
        changed, target_count = rewrite(args.repo_root, replacements)
    else:
        changed, target_count = [], 0
    if target_count:
        notes.append(
            "Also bumped `MACOSX_DEPLOYMENT_TARGET` in %d place(s) to `%d.0`: the "
            "Homebrew libraries on `macos-%d` are built for macOS %d, and delocate "
            "rejects a wheel with a lower target." % (target_count, oldest, oldest, oldest)
        )

    oldest_label = "macos-%d" % oldest
    state = {
        "used": used,
        "rows": rows,
        "rows_by_version": labels_by_version,
        "retired": retired,
        "issues": issues,
        "oldest_label": oldest_label,
        "notes": notes,
    }
    report = render(state)
    if truncated:
        report += "\n_Issue history truncated at %d pages._\n" % ISSUES_MAX_PAGES

    print(report, end="")
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as handle:
            handle.write(report)
    if args.report_out:
        with open(args.report_out, "w", encoding="utf-8") as handle:
            handle.write(report)

    bump_required = bool(replacements)
    if args.github_output:
        outputs = {
            "oldest_macos": oldest_label,
            "bump_required": "true" if bump_required else "false",
            "branch": "chore/macos-runner-%d" % oldest,
            "report": report,
        }
        with open(args.github_output, "a", encoding="utf-8") as handle:
            for key, value in outputs.items():
                handle.write("%s<<__IMAS_EOF__\n%s\n__IMAS_EOF__\n" % (key, value))

    if changed:
        print("rewritten: " + ", ".join(changed), file=sys.stderr)


if __name__ == "__main__":
    main()