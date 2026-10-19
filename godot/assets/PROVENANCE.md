# Asset Provenance & License Metadata (`godot/assets/`)

All visual and audio assets bundled in `godot/assets/` were generated specifically for **SUNBREAK // V'BALL** and are distributed under the repository's MIT License (`LICENSE.md`).

## 1. Athlete & Volleyball Sprite Plates (`godot/assets/sprites/`)
- **Albedo Plates (`*.png`)**:
  - `pose_ready.png`, `pose_dig.png`, `pose_set.png`, `pose_jump_coil.png`, `pose_spike.png`, `pose_spike_follow.png`, `pose_dive.png`, `plate_closeup.png`, `ball_vball.png`
  - **Source / Pipeline**: Generated as high-contrast anime-arcade keyframe plates and chroma-cleaned to RGBA with deterministic alpha silhouettes for 8-bone `Skeleton2D` + `Polygon2D` UV mesh deformation and RGB palette recoloring in `sunbreak_athlete.gdshader`.
- **Tangent-Space Normal Maps (`*_normal.png`)**:
  - `pose_ready_normal.png`, `pose_dig_normal.png`, `pose_set_normal.png`, `pose_jump_coil_normal.png`, `pose_spike_normal.png`, `pose_spike_follow_normal.png`, `pose_dive_normal.png`, `plate_closeup_normal.png`, `ball_vball_normal.png`
  - **Source / Pipeline**: Synthesized deterministically from silhouette distance fields and luminance gradients for 2.5D directional sun/neon rim lighting and sweat specular highlights.

## 2. Audio Suite (`godot/assets/audio/`)
All 20 audio clips were generated via the Venice Audio & TTS pipeline and normalized with `ffmpeg` to 44.1 kHz 16-bit PCM `.wav`:

| File | Category | Format | Description |
| :--- | :--- | :--- | :--- |
| `bgm_sunbreak.wav` | Music (`venice music`) | 44.1 kHz 16-bit Stereo PCM | 60s looping upbeat tropical synthwave & slap-bass arcade beach volleyball soundtrack |
| `sfx_dig.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Forearm bump / dig pass impact |
| `sfx_set.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Crisp overhead finger set / soft dink touch |
| `sfx_spike.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Explosive airborne power spike whip & impact |
| `sfx_fire_slam.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Fiery supersonic boom spike (`ON FIRE` / `Solar Overdrive`) |
| `sfx_dive.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Athletic chest-slide dive across beach sand |
| `sfx_block.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Heavy net-front Monster Roof Block impact |
| `sfx_sand_bounce.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Volleyball thudding into packed beach sand |
| `sfx_whistle.wav` | SFX (`venice sfx`) | 44.1 kHz 16-bit Mono PCM | Referee pea whistle for point / match start |
| `vo_sunbreak_title.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Sunbreak! Beach Volleyball!"* |
| `vo_heating_up.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Heating up!"* |
| `vo_on_fire.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"She's on fire! Unstoppable!"* |
| `vo_fire_extinguished.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Shutdown! Fire extinguished!"* |
| `vo_monster_block.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Monster roof block!"* |
| `vo_laser_serve.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Sweet spot! Laser serve!"* |
| `vo_solar_spike.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Solar Overdrive slam!"* |
| `vo_deuce.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Deuce! Win by two!"* |
| `vo_match_won.wav` | TTS (`tts-kokoro`, `am_fenrir`) | 44.1 kHz 16-bit Mono PCM | *"Game, set, and match! Sunbreak champions!"* |
| `vo_partner_block.wav` | TTS (`tts-kokoro`, `af_heart`) | 44.1 kHz 16-bit Mono PCM | *"I've got the net! Block formation!"* |
| `vo_partner_peel.wav` | TTS (`tts-kokoro`, `af_heart`) | 44.1 kHz 16-bit Mono PCM | *"Peeling back! Covering deep court!"* |
