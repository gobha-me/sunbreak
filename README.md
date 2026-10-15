# SUNBREAK // V'BALL (Godot 4.7.2 + C++23)

Native **Godot 4.7.2** + **C++23 GDExtension** 2v2 arcade beach volleyball engine featuring:

- **Authoritative C++23 Match Simulation (`libsunbreak_core` + `libsunbreak_godot`)**:
  - Deterministic 3D court & aerodynamic volleyball physics: topspin dive, lateral Meteor curve, `🌀 Knuckle Float` sinusoidal wobble, dynamic Beach Trade-Wind (`🌬️ WIND`), and Net-Cord Trickle impulses.
  - Analytical ball-landing & hand-socket intercept solver (`hand_socket_world`, `predict_ball_landing`).
  - **4-Stage Keyframed Jump Chain (`Stand -> Squat -> Stand Arms-Up Push-Off -> In-Air Bow-Arch -> Whip Follow-Through -> Landing Squat -> Stand`)** with C++23 `SkeletonPose2D` bone angle solver and 2nd-order soft-body spring-damper physics.
  - **NBA Jam / Blitz Arcade Ruleset**:
    - 2-step Serve Power & Sweet-Spot Bar (`0.75..0.90` `★ SWEET ★` zone -> `🌀 KNUCKLE FLOAT` or `⚡ LASER ACE`).
    - 3-point streak `🔥 ON FIRE` state + mandatory defensive Counter-Play extinguish (`🧯 FIRE EXTINGUISHED!`) on active Dig (`J`), Sand Dive (`K`), or Net Roof Block.
    - Off-ball Partner Defensive Tactics (`🧱 BLOCK NET [L]` vs. `🛡️ PEEL DEEP [L]`) and Deuce ("Win by 2") / Match Point detection.
- **Godot 4.7.2 Native Presentation (`godot/`)**:
  - **8-Bone `Skeleton2D` + Deformable `Polygon2D` Athlete Rigs**: Every athlete is driven by an 8-bone `Skeleton2D` (`Hip -> Torso -> Head`, `Torso -> Arm -> HandSocket`, `Hip -> Thigh -> Shin -> Foot`) weighted to a triangulated `Polygon2D` UV mesh at **100% opacity** (zero alpha crossfading, zero ghosting).
  - **2.5D Normal-Mapped Shaders**: `sunbreak_court.gdshader` (normal-mapped beach sand, ocean waves, caustics, sun reflections) and `sunbreak_athlete.gdshader` (RGB bikini/trim/hair/skin palette classifier, 2.5D normal lighting, sweat specular, sand grain cling, and `🔥 ON FIRE` rim glow).

## Build & Verify

```bash
# Format & Lint (clang-format-20 / clang-tidy-20)
./tools/format.sh --check
./tools/lint.sh

# Build C++23 Core, Tests & Godot 4.7.2 GDExtension Bridge (bounded parallelism -j 4)
cmake -S . -B /tmp/sunbreak-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sunbreak-build -j 4
ctest --test-dir /tmp/sunbreak-build --output-on-failure

# Run Godot 4.7.2 Native Contract Suite
/config/.local/bin/godot --headless --audio-driver Dummy --path godot -s res://contract_test.gd
```
