"""Generate DogGame from the user's supported XBE and persistent title overrides."""
import argparse
import subprocess
import sys
import shutil
from pathlib import Path

project = Path(__file__).resolve().parents[1]


def generate(game, runtime, output):
    output = output.resolve()
    if not output.is_relative_to(project):
        raise ValueError("Generated output must remain inside DogGame")
    work = project / "build/pipeline"
    work.mkdir(parents=True, exist_ok=True)
    manual = work / "manual_sources.c"
    # Scan maintained sources, including API implementations, without collecting
    # generated translations back into the manual exclusion list.
    source_root = project / "src"
    maintained_sources = sorted(p for p in source_root.rglob("*.c")
                                if p.relative_to(source_root).parts[0] != "recomp")
    manual.write_text("\n".join(p.read_text(encoding="utf-8")
                                for p in maintained_sources), encoding="utf-8")

    def run(module, *args):
        subprocess.run([sys.executable, "-m", module, *map(str, args)], cwd=runtime, check=True)

    xbe = game.resolve() / "default.xbe"
    run("tools.xbe_parser", xbe, "--json", work / "default_analysis.json")
    run("tools.disasm", xbe, "-o", work / "disasm", "--force",
        "--seed-functions", project / "config/seed_functions.json",
        "--analysis-json", work / "default_analysis.json")
    run("tools.func_id", xbe, "--functions", work / "disasm/functions.json",
        "--strings", work / "disasm/strings.json", "--xrefs", work / "disasm/xrefs.json",
        "--output", work / "func_id")
    run("tools.abi_analysis", xbe, "--disasm-dir", work / "disasm",
        "--func-id-dir", work / "func_id", "--output-dir", work / "abi")
    staging = work / "generated"
    staging.mkdir(exist_ok=True)
    # Remove only files previously produced by this generator, within its own staging directory.
    for p in staging.glob("recomp_*"):
        if p.is_file() and p.suffix in {".c", ".h"}:
            p.unlink()
    run("tools.recomp", xbe, "--all", "--disasm-dir", work / "disasm",
        "--func-id-dir", work / "func_id", "--abi-dir", work / "abi", "--split", "250",
        "--gen-dir", staging, "--output-dir", work / "recomp",
        "--game-name", "DogGame", "--exclude-manual", manual)
    output.mkdir(parents=True, exist_ok=True)
    generated = {p.name: p for p in staging.iterdir() if p.is_file()}
    for old in output.glob("recomp_*"):
        if old.is_file() and old.suffix in {".c", ".h"} and old.name not in generated:
            old.unlink()
    for name, source in generated.items():
        target = output / name
        if not target.exists() or source.read_bytes() != target.read_bytes():
            shutil.copy2(source, target)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, default=project / "build/runtime/xboxrecomp")
    parser.add_argument("--output", type=Path, default=project / "src/recomp/gen")
    args = parser.parse_args()
    generate(args.game_dir, args.runtime.resolve(), args.output)
