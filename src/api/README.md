# Shared APIs

This folder owns the guest adapters and shared host services. Each public header declares the operations available to application code in `src/`.

Callers include headers as `api/title_guest_menu.h`, for example. API implementations use local header names for other APIs and the configured `src/` include directory for application interfaces.

Guest layouts, original instruction sequences, register/stack handling, and memory access belong here. Application entry points, debug policy, input interpretation, browser flow, and rendering remain in `src/`.

See [the source map](../README.md), [recovery evidence](../REVERSE_ENGINEERING.md), and [the project writing standard](../../AGENTS.md). The generation scan includes this folder and excludes `src/recomp/`.
