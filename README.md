# SUNBREAK // V'BALL (Godot 4.7.2 + C++23)

Native **Godot 4.7.2** + **C++23 GDExtension** 2v2 arcade beach volleyball game combining high-energy arcade rules (*NBA Jam* / *Dead or Alive Xtreme*) with a deterministic C++23 match engine, 8-bone `Skeleton2D` deformable athlete rigs, 2.5D normal-mapped shaders, and a complete 20-clip synthwave BGM, SFX, and announcer/partner voice suite.

---

## Key Features

### 1. Authoritative C++23 Match Simulation (`libsunbreak_core` + `libsunbreak_godot`)
- **3D Aerodynamic Volleyball Physics**: Topspin dive, lateral Meteor curve, `🌀 Knuckle Float` sinusoidal wobble, dynamic Beach Trade-Wind (`🌬️ WIND`), and Net-Cord Trickle impulses.
- **Analytical Intercept & Landing Solver**: Real-time `hand_socket_world` and `predict_ball_landing` solver projecting the `+` sand target reticle at playable waist/forearm reception height (`z <= 44`).
- **4-Stage Keyframed Jump Chain**: `Stand -> Squat (JumpCoil) -> Stand Arms-Up Push-Off (JumpLaunch) -> In-Air Bow-Arch (Spike) -> Whip Follow-Through -> Landing Squat -> Stand` driven by a C++23 `SkeletonPose2D` bone angle solver and 2nd-order soft-body spring-damper physics (`Realistic`, `DOAX Spring`, and `Overdrive` presets).
- **6-Athlete Roster**:
  - **Kasumi** (*Crimson Cyclone*) • **Elena** (*Solar Valkyrie*) • **Mei-Lin** (*Jade Tempest*)
  - **Ayane** (*Midnight Mirage*) • **Nyx** (*Neon Viper*) • **Freya** (*Valkyrie Frost*)

### 2. Arcade Ruleset & Tactical Depth
- **15-Point Sets (Win by 2)**: Full Deuce (`14–14+`) and Match Point tracking with automatic celebratory reset.
- **2-Step Serve Power & Sweet-Spot Bar**: Release or tap inside the `0.75..0.90` `★ SWEET ★` window for a `🌀 KNUCKLE FLOAT` standing serve or `⚡ SOLAR LASER SERVE` jump serve.
- **Directional Spike Aiming & Velvet Drop Dink**:
  - Hold **Back** (away from the net) while airborne to feather a soft **`VELVET DROP DINK!`** over net blockers.
  - Hold **Forward** (toward the net) while airborne for +45 px/s cross-court drive velocity.
- **`🔥 ON FIRE` Streaks & Mandatory Defensive Counter-Play**:
  - Scoring 3 consecutive points ignites `🔥 ON FIRE` mode (`ON FIRE SLAM!` + cut-in).
  - An active **Counter-Dig (`J`)**, **Sand Dive (`L`)**, or **Monster Roof Block** against an `ON FIRE` opponent immediately extinguishes their streak (`🧯 FIRE EXTINGUISHED!`) and awards +40% **Solar Overdrive** meter.
- **Partner AI Defensive Tactics (`;` / `Tab` / `LB`)**: Toggle your AI partner live between **`🧱 BLOCK NET`** (net-front roof block coverage) and **`🛡️ PEEL DEEP`** (backcourt perimeter defense).
- **Title Screen, Pause Menu & 4x AI Attract/Spectator Mode**:
  - Select your P1 Athlete, Partner AI, Lighting Theme, and Soft-Body Rig from the Title Menu while 4 AI athletes play live in the background.
  - Choose **Watch 4x AI Attract Exhibition** from the Title/Pause menu—or leave controls idle for 25 seconds during a match—to watch autonomous 4-AI rallies; press **`J` / `Space` / `Gamepad A`** at any time to jump straight back into 1P human control.

### 3. Godot 4.7.2 Native Presentation & Audio (`godot/`)
- **8-Bone `Skeleton2D` + Deformable `Polygon2D` Rigs**: Every athlete is driven by an 8-bone `Skeleton2D` (`Hip -> Torso -> Head`, `Torso -> Arm -> HandSocket`, `Hip -> Thigh -> Shin -> Foot`) weighted to a triangulated `Polygon2D` UV mesh at **100% opacity** (zero alpha crossfading, zero ghosting).
- **2.5D Normal-Mapped Shaders**:
  - `sunbreak_court.gdshader`: Normal-mapped beach sand, ocean waves, caustics, and sun reflections across 3 lighting themes (*Sunset Gold*, *High Noon*, *Neon Night*).
  - `sunbreak_athlete.gdshader`: RGB bikini/trim/hair/skin palette classifier, 2.5D normal lighting, anatomical soft-body UV spring displacement, sweat specular, sand grain cling, and `🔥 ON FIRE` rim glow.
- **20-Clip Audio Suite (`godot/assets/audio/`)**:
  - **BGM**: 60s looping tropical synthwave soundtrack (`bgm_sunbreak.wav`).
  - **SFX**: 8 court/ball impact cues (`sfx_dig`, `sfx_set`, `sfx_spike`, `sfx_fire_slam`, `sfx_dive`, `sfx_block`, `sfx_sand_bounce`, `sfx_whistle`).
  - **Voice (TTS)**: 11 announcer & partner tactical callouts (`vo_sunbreak_title`, `vo_heating_up`, `vo_on_fire`, `vo_fire_extinguished`, `vo_monster_block`, `vo_laser_serve`, `vo_solar_spike`, `vo_deuce`, `vo_match_won`, `vo_partner_block`, `vo_partner_peel`).

---

## Controls (Keyboard & Gamepad)

| Action | Keyboard (Right-Hand Cluster) | Gamepad (Xbox / Standard) |
| :--- | :--- | :--- |
| **Move / Aim Shot** | `WASD` or Arrow Keys | Left Stick or D-Pad |
| **Serve / Pass / Counter-Dig** | `J` / `Space` / `Z` / `Enter` | `A` (South) |
| **Jump-Serve / Jump-Spike / Block** | `K` / `X` | `X` (West) / `Y` (North) |
| **Velvet Drop Dink (Airborne)** | Hold `A` (Back) + `K` | Hold Left Stick Back + `X` |
| **Pancake Sand Dive** | `L` / `C` | `B` (East) |
| **Partner Tactic (`BLOCK` ↔ `PEEL`)** | `;` / `Tab` / `U` | `LB` (Left Shoulder) |
| **Turbo Sprint** | `Shift` | `RB` or `RT` |
| **Pause / Title Menu** | `Esc` | `Start` |
| **Mute / Unmute Audio** | `M` | — |
| **Skeleton2D Studio Cam Toggle** | `P` | — |
| **Lighting Themes (1 / 2 / 3)** | `1` / `2` / `3` | Via Title / Pause Menu |

---

## Build, Verify & Run

```bash
# 1. Format & Lint (clang-format-20 / clang-tidy-20)
./tools/format.sh --check
./tools/lint.sh

# 2. Build C++23 Core, Tests & Godot 4.7.2 GDExtension Bridge (bounded parallelism -j 4)
cmake -S . -B /tmp/sunbreak-build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sunbreak-build -j 4
ctest --test-dir /tmp/sunbreak-build --output-on-failure

# 3. Import GDExtension & Run Headless Godot 4.7.2 Native Contract Suite
godot --headless --audio-driver Dummy --path godot --editor --import --quit --frame-delay 1000
godot --headless --audio-driver Dummy --path godot -s res://contract_test.gd

# 4. Launch Interactive Game
godot --path godot
```
