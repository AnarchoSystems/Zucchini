#!/usr/bin/env python3
"""Publish CTest JUnit output for a local Zucchini dashboard.

Run once after CTest, including when tests fail:
    python scripts/export_ctest_results.py results.xml path/to/report
"""

import argparse
import json
import os
from pathlib import Path
import tempfile
import xml.etree.ElementTree as ET


def export_results(source: Path, directory: Path) -> None:
    xml = source.read_text(encoding="utf-8-sig")
    root = ET.fromstring(xml)
    if root.tag not in ("testsuite", "testsuites"):
        raise ValueError("Expected CTest JUnit XML")
    directory.mkdir(parents=True, exist_ok=True)
    destination = directory / "ctest-results.js"
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", dir=directory, delete=False
        ) as stream:
            temporary = Path(stream.name)
            stream.write("window.zucchiniCTest(" + json.dumps(xml) + ");\n")
        os.replace(temporary, destination)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("junit", type=Path)
    parser.add_argument("report_directory", type=Path)
    args = parser.parse_args()
    export_results(args.junit, args.report_directory)
