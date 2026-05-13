# Project notes for Claude

## Issue tracking — always on

This project uses the `/issues` skill backed by `./issues.jsonl`. Treat issue
hygiene as a standing requirement, not something to wait to be asked about:

- Before starting any non-trivial work, check `issues.jsonl`. If no matching
  issue exists, create one (status `open`) before you begin.
- Move an issue to `in-progress` the moment work on it starts. Update its
  `updated` field.
- Never mark an issue `done` without explicit user confirmation that they
  have tested the change.
- When new work surfaces during a conversation (porting steps, follow-ups,
  cleanup, bugs found), append it as a new issue immediately rather than
  trusting it to memory.
- When summarizing progress, lead with the issue table (open / in-progress /
  done counts and the relevant rows).

## Toolchain

- Mac-side targets historically built with THINK C (`.proj.bin` files in
  the repo root). A parallel Retro68 toolchain is being stood up — see
  `BUILDING-RETRO68.md` for setup, gotchas, and verification.
- Retro68 lives at `~/github/Retro68`; built toolchain at
  `~/Retro68-build/toolchain/bin` (on PATH via `~/.zshrc`).
- Mini vMac (Plus variant) is the local emulator for smoke-testing 68000-era
  builds. Basilisk II (issue #4) is planned for larger targets and MacTCP.

## History

The Retro68 port (Phases 1–2 of `ROADMAP.md`) was bootstrapped over a
multi-day pair-programming session with Paul in May 2026: toolchain
stand-up, the FujiSerialStub port, and the FujiStubTest runtime harness
that verified 5/5 dispatch slots in Mini vMac. Pick up from `issues.jsonl`
and the roadmap's Phase 3 onward — start with #5 (FujiSerialAsync) unless
the user directs otherwise.
