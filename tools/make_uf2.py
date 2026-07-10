#!/usr/bin/env python3
import shutil
import subprocess
from pathlib import Path

targets = [
    ("pico", "build-pico"),
    ("pico2", "build-pico2"),
]

binary_dir = Path("binary")
binary_dir.mkdir(exist_ok=True)

for board, build_dir in targets:
    build = Path(build_dir)
    build.mkdir(exist_ok=True)

    subprocess.run(
        ["cmake", "-B", str(build), f"-DPICO_BOARD={board}"],
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(build)],
        check=True,
    )

    for uf2 in build.rglob("*.uf2"):
        shutil.copy2(uf2, binary_dir / f"{uf2.stem}-{board}.uf2")

print("UF2 files copied to ./binary")