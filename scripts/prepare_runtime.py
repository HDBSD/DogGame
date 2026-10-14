"""Assemble pinned upstream plus DogGame overrides without editing upstream."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

project = Path(__file__).resolve().parents[1]


def prepare(upstream, output):
    upstream, output = upstream.resolve(), output.resolve()
    if output == upstream or output.is_relative_to(upstream) or upstream.is_relative_to(output):
        raise ValueError("Runtime output and upstream must be separate directories")
    if not output.is_relative_to(project / "build"):
        raise ValueError("Runtime output must be inside DogGame/build")
    manifest = json.loads((project / "config/xboxrecomp.lock.json").read_text())
    # Validate before writing anything: a different upstream needs an explicit review.
    for name, expected in manifest["files"].items():
        source = upstream / name
        if not source.is_file() or hashlib.sha256(source.read_bytes().replace(b"\r\n", b"\n")).hexdigest() != expected:
            raise ValueError(f"Upstream differs from pinned revision: {name}")
    desired = {name: upstream / name for name in manifest["files"]}
    overlay = project / "runtime_overrides"
    desired.update({p.relative_to(overlay).as_posix(): p for p in overlay.rglob("*")
                    if p.is_file() and "__pycache__" not in p.parts and p.suffix != ".pyc"})
    state = output / ".sd100f-overlay.json"
    if state.exists():
        for name in json.loads(state.read_text()):
            target = (output / name).resolve()
            if not target.is_relative_to(output):
                raise ValueError("Invalid runtime state path")
            if name not in desired and target.is_file():
                target.unlink()
    for name, source in desired.items():
        target = output / name
        if not target.exists() or target.read_bytes() != source.read_bytes():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
    state.write_text(json.dumps(sorted(desired), indent=2) + "\n")
    print(f"Prepared DogGame runtime: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=project / "build/runtime/xboxrecomp")
    args = parser.parse_args()
    prepare(args.upstream, args.output)
