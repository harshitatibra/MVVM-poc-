#!/usr/bin/env python3
"""Create two standalone source ZIPs plus the full repository source ZIP."""
import argparse
from pathlib import Path
import zipfile
from check_layout import ROOT, check_layout


def source_files(folder):
    excluded = {".git", ".local-checks", "build", "install", "log", "dist",
                "__pycache__", ".qt", ".cache"}
    for file in sorted(folder.rglob("*")):
        relative = file.relative_to(folder)
        if not file.is_file() or any(part in excluded or part.startswith("build-")
                                     for part in relative.parts):
            continue
        if file.suffix in {".zip", ".user", ".pyc"} or file.name == "CMakeUserPresets.json":
            continue
        yield file, relative


def package(folder, output, name):
    files = list(source_files(folder))
    archive_path = output / f"{name}.zip"
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for file, relative in files:
            archive.write(file, (Path(name) / relative).as_posix())
    with zipfile.ZipFile(archive_path) as archive:
        for file, relative in files:
            if archive.read((Path(name) / relative).as_posix()) != file.read_bytes():
                raise RuntimeError(f"Archive verification failed: {relative}")
    print(f"PASS: {archive_path.name}: {len(files)} source files; contents verified")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    check_layout()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    package(ROOT / "gcs_app", output, "gcs_app")
    package(ROOT / "robot_ros2", output, "robot_ros2")
    package(ROOT, output, "qt_ros2_poc")
