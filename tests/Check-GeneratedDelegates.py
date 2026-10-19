"""Check maintained FMV observers against the pinned recompiler's manual scanner."""
from pathlib import Path
import importlib.util
import tempfile

root = Path(__file__).resolve().parents[1]
scanner = root / "build/runtime/xboxrecomp/tools/recomp/manual_scan.py"
spec = importlib.util.spec_from_file_location("doggame_manual_scan", scanner)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
sources = sorted(path for path in (root / "src").rglob("*.c")
                 if path.relative_to(root / "src").parts[0] != "recomp")
with tempfile.TemporaryDirectory(dir=root / "build") as work:
    manual = Path(work) / "manual_sources.c"
    manual.write_text("\n".join(path.read_text(encoding="utf-8") for path in sources), encoding="utf-8")
    skipped, wrapped, referenced = module.scan(manual)
    expected = {0x001000B0, 0x00152FEC}
    assert expected <= skipped, "Both guest entry points must remain manually owned"
    assert expected <= wrapped, "Clean generation must retain both original bodies as sub_ADDRESS_gen"
    assert expected <= referenced, "Original entry points must remain discoverable"
print("Passed: both FMV entry points retain translated delegates during clean setup generation.")
