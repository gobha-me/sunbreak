# Changelog

All notable changes to **SUNBREAK // V'BALL** are documented in this file.

## [0.1.0] - 2026-10-19

### Added
- **Authoritative C++23 Match Engine (`libsunbreak_core`)**:
  - 3D court & aerodynamic volleyball physics with topspin dive, lateral curve, `Knuckle Float` sinusoidal wobble, dynamic Beach Trade-Wind, and Net-Cord Trickle impulses.
  - Analytical `hand_socket_world` and waist-height (`z <= 44`) `predict_ball_landing` intercept solver.
  - 4-stage keyframed jump chain (`Stand -> Squat -> Stand Arms-Up Push-Off -> In-Air Bow-Arch -> Whip Follow-Through -> Landing Squat -> Stand`) with `SkeletonPose2D` bone angles and 2nd-order soft-body spring-damper physics (`Realistic`, `DOAX Spring`, `Overdrive`).
  - 6-athlete roster (*Kasumi, Elena, Mei-Lin, Ayane, Nyx, Freya*) with distinct speed/power/jump/control attributes and custom palette profiles.
  - Arcade ruleset: 15-point win-by-2 sets, 2-step Serve Power & Sweet-Spot Bar (`0.75..0.90`), 3-point `ON FIRE` streaks, mandatory defensive Counter-Play extinguish, Solar Overdrive meter, directional spike aiming, and airborne `Velvet Drop Dink`.
  - Partner AI defensive tactic switching (`BLOCK NET` vs. `PEEL DEEP`) and autonomous 4-AI Attract/Spectator mode (`set_attract_mode`).
- **Godot 4.7.2 Native Presentation & GDExtension Bridge (`libsunbreak_godot`, `godot/`)**:
  - `SunbreakMatch` GDExtension bridge exposing authoritative snapshots, input stepping, presets, roster assignment, attract mode, and audio event sequences.
  - 8-bone `Skeleton2D` + deformable `Polygon2D` athlete rigs rendered at 100% opacity (zero ghosting) with `sunbreak_court.gdshader` and `sunbreak_athlete.gdshader`.
  - Interactive **Title Menu** (1P vs CPU, 4x AI Spectator, P1/Partner Athlete selection, Lighting Theme, Soft-Body Rig), **Pause Menu**, and 25-second auto-idle Attract Mode with instant 1P jump-in.
  - Ergonomic right-hand keyboard cluster (`WASD` + `J/K/L/;`) and full Gamepad (`Stick/D-Pad` + `A/X/B/LB/RT/Start`) support.
  - Hit-stop freeze frames and camera shake impulses on Monster Roof Blocks, Power Spikes, and Solar/On-Fire Slams.
  - Complete 20-clip 44.1 kHz 16-bit PCM WAV audio suite (`bgm_sunbreak.wav`, 8 court/ball SFX clips, and 11 announcer/partner Kokoro TTS voice lines).
