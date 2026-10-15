# AGENTS.md — `sunbreak` (Godot 4.7.2 + C++23)

## Ownership & Architecture
- **C++23 (`include/sunbreak/`, `src/`)** owns the authoritative match simulation, aerodynamic ball physics, 4-stage jump chain, `SkeletonPose2D` bone angle solver, soft-body spring-damper solver, Serve Power Bar, `ON FIRE` counter-play rules, and AI partner tactics.
- **Godot 4.7.2 (`godot/`)** owns native presentation (`Skeleton2D` + `Polygon2D` deform rigs, `sunbreak_court.gdshader`, `sunbreak_athlete.gdshader`, and HUD).

## Build & Quality Gates
- Always cap parallelism at `-j 4` in the 16 GiB pod and stage large C++ builds in `/tmp`.
- Format with `tools/format.sh` (`clang-format-20`) and lint with `tools/lint.sh` (`clang-tidy-20`).
- Validate with both `g++` and `clang++`, plus the headless Godot 4.7.2 contract suite (`godot --headless --audio-driver Dummy --path godot -s res://contract_test.gd`).
