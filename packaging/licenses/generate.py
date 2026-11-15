#!/usr/bin/env python3
# This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse
"""Builds the third-party notices of EasyMorse.

Reads components.json (written by hand), the license texts of texts/ and the
SBOM that Qt installs next to its libraries (<Qt>/<version>/<platform>/sbom),
then writes:

  packaging/licenses/licenses.json  embedded in the application (About page)
  THIRD-PARTY-NOTICES.txt           installed with the packages

Both are committed: building EasyMorse needs neither Python nor the SBOM. Run
this again when Qt or a bundled library changes version:

  packaging/licenses/generate.py ~/Qt/6.8.3/gcc_64 ~/Qt/6.8.3/android_arm64_v8a
"""

import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
GROUP_QT = "Inside Qt"
LICENSE_ID = re.compile(r"[A-Za-z0-9.+-]+")


def license_ids(expression):
    """SPDX identifiers of an expression, without the operators."""
    ids = []
    for token in LICENSE_ID.findall(expression or ""):
        if token in ("AND", "OR", "WITH") or token in ids:
            continue
        ids.append(token)
    return ids


def text_key(license_id):
    # "or later" uses the text of the version it names
    return license_id[:-len("-or-later")] + "-only" if license_id.endswith("-or-later") else license_id


def comment_field(comment, field):
    match = re.search(rf"^\s*{field}: (.*)$", comment or "", re.MULTILINE)
    return match.group(1).strip() if match else ""


def clean(value):
    value = (value or "").strip()
    if value.startswith("<text>") and value.endswith("</text>"):
        value = value[len("<text>"):-len("</text>")].strip()
    return "" if value in ("NOASSERTION", "unknown") else value


def qt_components(platform_dirs, config):
    """Third-party code that Qt carries, from its SBOM, merged over the platforms."""
    found = {}
    extracted = {}
    for platform_dir in platform_dirs:
        platform = Path(platform_dir).name
        for module in config["modules"]:
            sbom = Path(platform_dir) / "sbom" / f"{module}-{config['version']}.spdx.json"
            if not sbom.exists():
                continue
            data = json.loads(sbom.read_text(encoding="utf-8"))
            for info in data.get("hasExtractedLicensingInfos", []):
                extracted[info["licenseId"]] = info["extractedText"].strip()
            for package in data["packages"]:
                target = package["name"]
                attribution = "_Attribution_" in target
                if not attribution and not target.startswith("Bundled"):
                    continue
                if target.split("_")[0] in config["skip"]:
                    continue
                comment = package.get("comment", "")
                name = comment_field(comment, "Name") or target.replace("Bundled", "", 1)
                version = clean(package.get("versionInfo"))
                license = clean(package.get("licenseConcluded"))
                if not license:
                    continue
                key = (name, version, license)
                entry = found.setdefault(key, {
                    "group": GROUP_QT,
                    "name": name,
                    "version": version,
                    "license": license,
                    "copyright": clean(package.get("copyrightText")),
                    "url": clean(package.get("homepage")) or clean(package.get("downloadLocation")),
                    "usage": comment_field(comment, "Qt usage"),
                    "_platforms": set(),
                    "_modules": set(),
                })
                entry["_platforms"].add(platform)
                entry["_modules"].add(module)
    all_platforms = {Path(p).name for p in platform_dirs}
    components = []
    for entry in sorted(found.values(), key=lambda e: (e["name"].lower(), e["version"])):
        platforms = entry.pop("_platforms")
        modules = entry.pop("_modules")
        if modules == {"qtwayland"} or entry["name"].startswith(("XCB", "xcb")):
            entry["platforms"] = "Linux"
        elif platforms != all_platforms and all(p.startswith("android") for p in platforms):
            entry["platforms"] = "Android"
        components.append(entry)
    return components, extracted


def notice_block(component):
    lines = [component["name"] + (" " + component["version"] if component.get("version") else "")]
    lines.append("-" * len(lines[0]))
    if component.get("platforms"):
        lines.append("Platforms: " + component["platforms"])
    if component.get("license"):
        lines.append("License: " + component["license"])
    if component.get("url"):
        lines.append("Source: " + component["url"])
    if component.get("usage"):
        lines.append("Use: " + component["usage"])
    if component.get("copyright"):
        lines.append("")
        lines.append(component["copyright"])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("qt_platform_dirs", nargs="+",
                        help="Qt installations holding an sbom/ directory, e.g. ~/Qt/6.8.3/gcc_64")
    args = parser.parse_args()

    config = json.loads((HERE / "components.json").read_text(encoding="utf-8"))
    qt, extracted = qt_components(args.qt_platform_dirs, config["qt"])
    if not qt:
        sys.exit("No Qt SBOM found in " + ", ".join(args.qt_platform_dirs))
    components = config["components"] + qt

    texts = {}
    missing = []
    for component in components:
        for license_id in license_ids(component.get("license")):
            key = text_key(license_id)
            if key in texts:
                continue
            path = HERE / "texts" / f"{key}.txt"
            if path.exists():
                texts[key] = path.read_text(encoding="utf-8").strip()
            elif key in extracted:
                texts[key] = extracted[key]
            else:
                missing.append(f"{key} ({component['name']})")
    if missing:
        sys.exit("License texts missing from packaging/licenses/texts: " + ", ".join(missing))

    # The commercial license of Qt is not the one EasyMorse uses: no need to show it
    texts.pop("LicenseRef-Qt-Commercial", None)
    texts = dict(sorted(texts.items(), key=lambda item: item[0].lower()))

    output = {
        "qtVersion": config["qt"]["version"],
        "components": components,
        "texts": texts,
    }
    (HERE / "licenses.json").write_text(json.dumps(output, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    header = f"""EasyMorse - third-party notices
===============================

EasyMorse is free software, distributed under the GNU General Public License
version 3 or later (file LICENSE.GPLv3). Its packages also contain the
software listed below, each under its own license. The full text of every
license named here follows the list.

Qt and the libraries bundled with the official packages are used unmodified.
Their source code can be downloaded from the address given for each of them;
the source code of Qt {config['qt']['version']} is at
https://download.qt.io/archive/qt/6.8/{config['qt']['version']}/submodules/
If one of these addresses no longer works, ask for the source code at
https://github.com/Artemia76/EasyMorse/issues

The list of the code that Qt carries comes from the SBOM installed with Qt
{config['qt']['version']} (Linux and Android builds); on Windows and macOS, Qt uses the
same code or the libraries of the system. This file is generated by
packaging/licenses/generate.py.
"""
    parts = [header]
    group = None
    for component in components:
        if component["group"] != group:
            group = component["group"]
            parts.append("\n" + "=" * 79 + "\n" + group + "\n" + "=" * 79)
        parts.append(notice_block(component))
    parts.append("\n" + "=" * 79 + "\nLicense texts\n" + "=" * 79)
    for key, text in texts.items():
        parts.append("-" * 79 + "\n" + key + "\n" + "-" * 79 + "\n\n" + text)
    (ROOT / "THIRD-PARTY-NOTICES.txt").write_text("\n\n".join(parts) + "\n", encoding="utf-8")
    print(f"{len(components)} components, {len(texts)} license texts")


if __name__ == "__main__":
    main()
