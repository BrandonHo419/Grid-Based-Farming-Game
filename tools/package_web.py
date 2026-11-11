"""Copy the browser build and dependency notices into a static demo directory."""

import argparse
from pathlib import Path
import shutil


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--build", type=Path, default=ROOT / "build/web")
    parser.add_argument("--raylib", type=Path, default=ROOT / "build/web/_deps/raylib-src")
    args = parser.parse_args()
    destination = args.destination.resolve()
    if destination == ROOT or destination in ROOT.parents:
        parser.error("choose a separate demo output directory")
    files = [args.build / name for name in ("grid_farm.html", "grid_farm.js", "grid_farm.wasm")]
    external = args.raylib / "src/external"
    if any(not path.is_file() for path in files) or not external.is_dir():
        parser.error("build the browser target first and provide its raylib source directory")
    destination.mkdir(parents=True, exist_ok=True)
    for path in files:
        shutil.copy2(path, destination / path.name)
    shutil.copy2(ROOT / "CREDITS.md", destination / "CREDITS.md")
    shutil.copy2(ROOT / "licenses/raylib.txt", destination / "raylib-license.txt")
    # keep upstream files intact rather than risk dropping an embedded licence
    for path in external.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in (".h", ".c", ".txt", ".md"):
            continue
        content = path.read_text(encoding="utf-8", errors="replace").lower()
        if not any(word in content for word in ("copyright", "license", "licence")):
            continue
        target = destination / "third-party" / path.relative_to(external)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    print(f"Browser files and notices copied to {destination}")


if __name__ == "__main__":
    main()
