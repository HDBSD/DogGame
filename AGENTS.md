# How we write maintained code

Apply this standard to all work in this project, including runtime overrides and scripts.

- Follow the spacing and separate-line braces used in `Setup.ps1`. Formatting is only the starting point.
- Make application code express intent through small APIs for the system it uses. Keep guest addresses, field offsets, button bits, calling conventions, and memory validation inside the relevant adapter.
- Put shared API implementations and headers in `src/api/`. Application sources include their public headers as `api/<name>.h`; keep entry points and application policy in `src/`.
- Use Ghidra/disassembly, debugging, and AI assistance to investigate behavior. Record which evidence actually exists. AI suggestions and inherited comments are not proof of a fresh recovery.
- Document why a workaround exists, how the original behavior was established, the supported revision, and what remains unknown. Include concrete function/instruction references or a reproducible observation when available.
- Name known concepts. Retain address-based names for unknown recovered routines instead of inventing an explanation. Do not turn original instruction sequences into guessed algorithms.
- Preserve guest stack, register, x87, SSE, memory access width, and instruction-order contracts. Host UI callbacks queue actions or consume copied data; guest mutations happen on the title thread. Observation-only diagnostics must state their weaker consistency guarantees.
- Give APIs clear ownership, thread, failure, and side-effect contracts. Keep rendering, input policy, file storage, network orchestration, and guest-layout decoding separate where those responsibilities differ.
- Explain custom behavior at its caller; document recovered layout and provenance beside its adapter and in `src/REVERSE_ENGINEERING.md`.
- Keep generated `src/recomp/` untouched unless regeneration was explicitly requested. Keep original guest-address entry points discoverable by `scripts/generate_game.py` and register new translation units in CMake.
- Check behavior at changed boundaries: stack balance, preserved registers, native call order, input edges, source-string restoration, snapshot ownership, and failure paths. Compilation alone does not establish gameplay correctness.
- Report unresolved reconstruction and validation limits accurately. Moving an instruction-based routine behind an API does not mean it has been recovered into high-level source.
