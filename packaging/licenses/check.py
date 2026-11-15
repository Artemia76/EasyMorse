#!/usr/bin/env python3
# This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse
"""Checks that the third-party notices name the versions the build really uses.

Run by the CI with the Qt version it installs:

  packaging/licenses/check.py 6.8.3

When it fails, update packaging/licenses/components.json and run generate.py.
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent


def read(path):
    return (ROOT / path).read_text(encoding="utf-8", errors="replace")


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    notices = json.loads(read("packaging/licenses/licenses.json"))
    versions = {c["name"]: c.get("version", "") for c in notices["components"]}
    expected = {
        "Qt": sys.argv[1],
        "libjuice": re.search(r"GIT_TAG\s+([0-9a-f]{40})", read("cmake/LibJuice.cmake")).group(1),
        "OpenSSL": re.search(r"^VERSION=(\S+)", read("packaging/android/make-openssl.sh"), re.MULTILINE).group(1),
        "miniaudio": ".".join(re.search(rf"#define MA_VERSION_{part}\s+(\d+)", read("3rdparty/miniaudio/miniaudio.h")).group(1)
                              for part in ("MAJOR", "MINOR", "REVISION")),
        "Monocypher": re.search(r"Monocypher (\S+)", read("3rdparty/monocypher/CMakeLists.txt")).group(1),
    }
    errors = [f"{name}: the notices say '{versions.get(name)}', the build uses {version}"
              for name, version in expected.items() if version not in versions.get(name, "")]
    if notices.get("qtVersion") != sys.argv[1]:
        errors.append(f"the Qt code listed comes from the SBOM of Qt {notices.get('qtVersion')}, not {sys.argv[1]}")
    if "Qt " + sys.argv[1] not in read("THIRD-PARTY-NOTICES.txt"):
        errors.append("THIRD-PARTY-NOTICES.txt is older than licenses.json: run generate.py")
    if errors:
        sys.exit("Third-party notices out of date (see packaging/licenses/generate.py):\n  " + "\n  ".join(errors))
    print("Third-party notices match the versions used")


if __name__ == "__main__":
    main()
