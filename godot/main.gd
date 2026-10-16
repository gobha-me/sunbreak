extends Node2D

var match_bridge: RefCounted = null
var court_rect: ColorRect = null
var court_mat: ShaderMaterial = null
var y_sort_layer: Node2D = null
var ball_sprite: Sprite2D = null
var hud_layer: Node2D = null

var pose_albedo: Dictionary = {}
var pose_normal: Dictionary = {}
var ball_tex: ImageTexture = null
var athlete_shader: Shader = null

var athlete_rigs: Array = []
var closeup_panel: ColorRect = null
var closeup_rig: Dictionary = {}
var last_snapshot: Dictionary = {}
var elapsed_time: float = 0.0

# Audio System (Venice Music, SFX, and TTS Announcer / Partner Voice Lines)
var bgm_player: AudioStreamPlayer = null
var sfx_player: AudioStreamPlayer = null
var vo_player: AudioStreamPlayer = null
var audio_clips: Dictionary = {}
var last_audio_seq: int = 0
var audio_muted: bool = false

# UI State: Title Screen, Pause Menu, and Idle Attract Mode
enum UiMode { PLAY = 0, TITLE = 1, PAUSE = 2 }
var ui_mode: int = UiMode.TITLE
var title_cursor: int = 0
var pause_cursor: int = 0
var idle_timer: float = 0.0
var selected_p1_roster: int = 0
var selected_p2_roster: int = 1
var selected_cpu1_roster: int = 4
var selected_cpu2_roster: int = 5

const ROSTER_NAMES := [
	"KASUMI (Crimson Cyclone)",
	"ELENA (Solar Valkyrie)",
	"MEI-LIN (Jade Tempest)",
	"AYANE (Midnight Mirage)",
	"NYX (Neon Viper)",
	"FREYA (Valkyrie Frost)"
]

const THEME_NAMES := ["SUNSET GOLD", "HIGH NOON", "NEON NIGHT"]
const PHYS_NAMES := ["REALISTIC", "DOAX SPRING", "OVERDRIVE"]

const POSE_NAMES := [
	"pose_ready",
	"pose_dig",
	"pose_set",
	"pose_jump_coil",
	"pose_spike",
	"pose_spike_follow",
	"pose_dive",
	"plate_closeup"
]

const REQUIRED_BONES := [
	"Hip",
	"Torso",
	"Head",
	"Arm",
	"HandSocket",
	"Thigh",
	"Shin",
	"Foot"
]

const AUDIO_FILES := [
	"bgm_sunbreak",
	"sfx_dig",
	"sfx_set",
	"sfx_spike",
	"sfx_fire_slam",
	"sfx_dive",
	"sfx_whistle",
	"sfx_block",
	"sfx_sand_bounce",
	"vo_sunbreak_title",
	"vo_heating_up",
	"vo_on_fire",
	"vo_fire_extinguished",
	"vo_monster_block",
	"vo_laser_serve",
	"vo_solar_spike",
	"vo_deuce",
	"vo_match_won",
	"vo_partner_block",
	"vo_partner_peel"
]

class HudCanvas extends Node2D:
	var owner_scene: Node2D = null

	func _draw() -> void:
		if owner_scene != null:
			owner_scene._draw_hud(self)

func _load_png_tex(rel_path: String) -> ImageTexture:
	var abs_path := ProjectSettings.globalize_path(rel_path)
	var img := Image.load_from_file(abs_path)
	if img == null or img.is_empty():
		push_error("Failed to load texture: " + abs_path)
		return null
	return ImageTexture.create_from_image(img)

func _load_wav_stream(rel_path: String, loop_enabled: bool = false) -> AudioStreamWAV:
	var abs_path := ProjectSettings.globalize_path(rel_path)
	if not FileAccess.file_exists(abs_path):
		return null
	var bytes := FileAccess.get_file_as_bytes(abs_path)
	if bytes.size() < 44:
		return null
	var channels := bytes.decode_u16(22)
	var sample_rate := bytes.decode_u32(24)
	var bits := bytes.decode_u16(34)
	var pos := 12
	var data_offset := 44
	var data_size := bytes.size() - 44
	while pos + 8 <= bytes.size():
		var chunk_id := bytes.slice(pos, pos + 4).get_string_from_ascii()
		var chunk_sz := bytes.decode_u32(pos + 4)
		if chunk_id == "data":
			data_offset = pos + 8
			data_size = mini(chunk_sz, bytes.size() - data_offset)
			break
		pos += 8 + chunk_sz + (chunk_sz % 2)
	var wav := AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_16_BITS if bits == 16 else AudioStreamWAV.FORMAT_8_BITS
	wav.mix_rate = int(sample_rate)
	wav.stereo = (channels == 2)
	wav.data = bytes.slice(data_offset, data_offset + data_size)
	if loop_enabled:
		wav.loop_mode = AudioStreamWAV.LOOP_FORWARD
		var bytes_per_sample := maxi(1, (bits / 8) * channels)
		wav.loop_begin = 0
		wav.loop_end = int(data_size / bytes_per_sample)
	return wav

func _play_sfx(key: String) -> void:
	if audio_muted or key == "" or sfx_player == null:
		return
	var stream: AudioStreamWAV = audio_clips.get(key, null)
	if stream != null:
		sfx_player.stream = stream
		sfx_player.play()

func _play_vo(key: String) -> void:
	if audio_muted or key == "" or vo_player == null:
		return
	var stream: AudioStreamWAV = audio_clips.get(key, null)
	if stream != null:
		vo_player.stream = stream
		vo_player.play()

func _build_deform_athlete_rig(rig_name: String, is_closeup: bool) -> Dictionary:
	var root_node := Node2D.new()
	root_node.name = rig_name

	var skel := Skeleton2D.new()
	skel.name = "Skeleton2D"
	root_node.add_child(skel)

	var bone_specs := [
		{"name": "Hip", "parent": "", "pos": Vector2(0.0, -64.0)},
		{"name": "Torso", "parent": "Hip", "pos": Vector2(0.0, -28.0)},
		{"name": "Head", "parent": "Torso", "pos": Vector2(0.0, -28.0)},
		{"name": "Arm", "parent": "Torso", "pos": Vector2(12.0, -10.0)},
		{"name": "HandSocket", "parent": "Arm", "pos": Vector2(18.0, 12.0)},
		{"name": "Thigh", "parent": "Hip", "pos": Vector2(0.0, 14.0)},
		{"name": "Shin", "parent": "Thigh", "pos": Vector2(0.0, 24.0)},
		{"name": "Foot", "parent": "Shin", "pos": Vector2(0.0, 20.0)}
	]

	var bone_nodes: Dictionary = {}
	for spec in bone_specs:
		var b := Bone2D.new()
		b.name = spec["name"]
		b.position = spec["pos"]
		b.set_autocalculate_length_and_angle(false)
		b.set_length(16.0)
		b.rest = Transform2D(0.0, spec["pos"])
		var p_name: String = spec["parent"]
		if p_name == "":
			skel.add_child(b)
		else:
			bone_nodes[p_name].add_child(b)
		bone_nodes[spec["name"]] = b

	var poly := Polygon2D.new()
	poly.name = "DeformMesh"
	var cols := 8
	var rows := 12
	var verts := PackedVector2Array()
	var uvs := PackedVector2Array()
	var polygons := []

	var w_hip := PackedFloat32Array()
	var w_torso := PackedFloat32Array()
	var w_head := PackedFloat32Array()
	var w_arm := PackedFloat32Array()
	var w_hand := PackedFloat32Array()
	var w_thigh := PackedFloat32Array()
	var w_shin := PackedFloat32Array()
	var w_foot := PackedFloat32Array()

	var mesh_w := 280.0 if is_closeup else 118.0
	var mesh_h := 280.0 if is_closeup else 156.0
	var tex_w := 512.0 if is_closeup else 384.0
	var tex_h := 512.0 if is_closeup else 512.0

	for r in range(rows + 1):
		var v_norm := float(r) / float(rows)
		for c in range(cols + 1):
			var u_norm := float(c) / float(cols)
			var px := (u_norm - 0.5) * mesh_w
			var py := (v_norm - 0.93) * mesh_h
			verts.append(Vector2(px, py))
			uvs.append(Vector2(u_norm * tex_w, v_norm * tex_h))

			var head_w := clampf(1.0 - absf(v_norm - 0.18) / 0.18, 0.0, 1.0)
			var torso_w := clampf(1.0 - absf(v_norm - 0.40) / 0.18, 0.0, 1.0)
			var arm_w := clampf((u_norm - 0.52) * 2.2, 0.0, 1.0) * torso_w
			var hand_w := clampf((u_norm - 0.65) * 2.8, 0.0, 1.0) * torso_w
			var hip_w := clampf(1.0 - absf(v_norm - 0.56) / 0.14, 0.0, 1.0)
			var thigh_w := clampf(1.0 - absf(v_norm - 0.70) / 0.14, 0.0, 1.0)
			var shin_w := clampf(1.0 - absf(v_norm - 0.82) / 0.12, 0.0, 1.0)
			var foot_w := clampf((v_norm - 0.84) / 0.16, 0.0, 1.0)

			var sum_w := maxf(0.001, head_w + torso_w + arm_w + hand_w + hip_w + thigh_w + shin_w + foot_w)
			w_head.append(head_w / sum_w)
			w_torso.append(torso_w / sum_w)
			w_arm.append(arm_w / sum_w)
			w_hand.append(hand_w / sum_w)
			w_hip.append(hip_w / sum_w)
			w_thigh.append(thigh_w / sum_w)
			w_shin.append(shin_w / sum_w)
			w_foot.append(foot_w / sum_w)

	for r in range(rows):
		for c in range(cols):
			var i0 := r * (cols + 1) + c
			var i1 := i0 + 1
			var i2 := i0 + (cols + 1)
			var i3 := i2 + 1
			polygons.append(PackedInt32Array([i0, i1, i2]))
			polygons.append(PackedInt32Array([i1, i3, i2]))

	poly.polygon = verts
	poly.uv = uvs
	poly.polygons = polygons

	var mat := ShaderMaterial.new()
	mat.shader = athlete_shader
	poly.material = mat
	root_node.add_child(poly)

	poly.skeleton = poly.get_path_to(skel)
	poly.clear_bones()
	poly.add_bone(skel.get_path_to(bone_nodes["Hip"]), w_hip)
	poly.add_bone(skel.get_path_to(bone_nodes["Torso"]), w_torso)
	poly.add_bone(skel.get_path_to(bone_nodes["Head"]), w_head)
	poly.add_bone(skel.get_path_to(bone_nodes["Arm"]), w_arm)
	poly.add_bone(skel.get_path_to(bone_nodes["HandSocket"]), w_hand)
	poly.add_bone(skel.get_path_to(bone_nodes["Thigh"]), w_thigh)
	poly.add_bone(skel.get_path_to(bone_nodes["Shin"]), w_shin)
	poly.add_bone(skel.get_path_to(bone_nodes["Foot"]), w_foot)

	return {
		"root": root_node,
		"skeleton": skel,
		"bones": bone_nodes,
		"polygon": poly,
		"material": mat,
		"base_verts": verts,
		"cols": cols,
		"rows": rows,
		"is_closeup": is_closeup
	}

func _ready() -> void:
	if ClassDB.class_exists("SunbreakMatch"):
		match_bridge = ClassDB.instantiate("SunbreakMatch")

	athlete_shader = load("res://shaders/sunbreak_athlete.gdshader")
	for pname in POSE_NAMES:
		pose_albedo[pname] = _load_png_tex("res://assets/sprites/%s.png" % pname)
		pose_normal[pname] = _load_png_tex("res://assets/sprites/%s_normal.png" % pname)
	ball_tex = _load_png_tex("res://assets/sprites/ball_vball.png")

	# Load Venice-generated BGM, SFX, and TTS voice clips
	for aname in AUDIO_FILES:
		var is_bgm := (aname == "bgm_sunbreak")
		var clip := _load_wav_stream("res://assets/audio/%s.wav" % aname, is_bgm)
		if clip != null:
			audio_clips[aname] = clip

	bgm_player = AudioStreamPlayer.new()
	bgm_player.name = "BgmPlayer"
	bgm_player.volume_db = -9.5
	add_child(bgm_player)
	if audio_clips.has("bgm_sunbreak"):
		bgm_player.stream = audio_clips["bgm_sunbreak"]
		bgm_player.play()

	sfx_player = AudioStreamPlayer.new()
	sfx_player.name = "SfxPlayer"
	sfx_player.volume_db = -2.0
	add_child(sfx_player)

	vo_player = AudioStreamPlayer.new()
	vo_player.name = "VoPlayer"
	vo_player.volume_db = 0.0
	add_child(vo_player)

	court_rect = ColorRect.new()
	court_rect.name = "BeachCourtRect"
	court_rect.position = Vector2.ZERO
	court_rect.size = Vector2(1280.0, 720.0)
	court_mat = ShaderMaterial.new()
	court_mat.shader = load("res://shaders/sunbreak_court.gdshader")
	court_rect.material = court_mat
	add_child(court_rect)

	y_sort_layer = Node2D.new()
	y_sort_layer.name = "CourtYSortLayer"
	y_sort_layer.y_sort_enabled = true
	add_child(y_sort_layer)

	for i in range(4):
		var rig := _build_deform_athlete_rig("AthleteRig_%d" % i, false)
		y_sort_layer.add_child(rig["root"])
		athlete_rigs.append(rig)

	ball_sprite = Sprite2D.new()
	ball_sprite.name = "VolleyballSprite"
	ball_sprite.texture = ball_tex
	y_sort_layer.add_child(ball_sprite)

	closeup_panel = ColorRect.new()
	closeup_panel.name = "TelemetryStudioPanel"
	closeup_panel.position = Vector2(845.0, 64.0)
	closeup_panel.size = Vector2(410.0, 360.0)
	closeup_panel.color = Color(0.035, 0.06, 0.12, 0.96)
	closeup_panel.clip_contents = true
	add_child(closeup_panel)

	closeup_rig = _build_deform_athlete_rig("CloseupStudioRig", true)
	var cr_root: Node2D = closeup_rig["root"]
	cr_root.position = Vector2(132.0, 338.0)
	closeup_panel.add_child(cr_root)

	hud_layer = HudCanvas.new()
	hud_layer.name = "HudOverlay"
	hud_layer.owner_scene = self
	add_child(hud_layer)

	if match_bridge != null:
		# Start on Title Screen with 4x AI playing live in the background!
		match_bridge.set_attract_mode(true)
		var snap: Dictionary = match_bridge.get_snapshot()
		last_audio_seq = int(snap.get("audio_seq", 0))
		apply_snapshot(snap)
		_play_vo("vo_sunbreak_title")

func apply_preset(preset_name: String) -> Dictionary:
	if match_bridge == null:
		return {}
	ui_mode = UiMode.PLAY
	match_bridge.set_attract_mode(false)
	match_bridge.apply_preset(preset_name)
	var snap: Dictionary = match_bridge.get_snapshot()
	last_audio_seq = int(snap.get("audio_seq", 0))
	apply_snapshot(snap)
	return snap

func _start_match(all_ai: bool) -> void:
	if match_bridge == null:
		return
	match_bridge.set_slot_roster(0, selected_p1_roster)
	match_bridge.set_slot_roster(1, selected_p2_roster)
	match_bridge.set_slot_roster(2, selected_cpu1_roster)
	match_bridge.set_slot_roster(3, selected_cpu2_roster)
	match_bridge.set_attract_mode(all_ai)
	match_bridge.reset_match()
	ui_mode = UiMode.PLAY
	idle_timer = 0.0
	_play_sfx("sfx_whistle")

func _update_rig_from_player(rig: Dictionary, p: Dictionary, theme_idx: int, is_closeup: bool) -> void:
	var pose_key: String = "plate_closeup" if is_closeup else String(p.get("pose_key", "pose_ready"))
	if is_closeup and float(last_snapshot.get("cut_in_timer", 0.0)) > 0.0:
		pose_key = String(p.get("pose_key", "pose_spike_follow"))

	var alb_tex: ImageTexture = pose_albedo.get(pose_key, pose_albedo.get("pose_ready"))
	var nrm_tex: ImageTexture = pose_normal.get(pose_key, pose_normal.get("pose_ready"))
	var poly: Polygon2D = rig["polygon"]
	var mat: ShaderMaterial = rig["material"]
	var bones: Dictionary = rig["bones"]
	var root_node: Node2D = rig["root"]

	poly.texture = alb_tex
	var tw := float(alb_tex.get_width()) if alb_tex != null else 384.0
	var th := float(alb_tex.get_height()) if alb_tex != null else 512.0
	var cols: int = rig["cols"]
	var rows: int = rig["rows"]
	var new_uvs := PackedVector2Array()
	for r in range(rows + 1):
		var vn := float(r) / float(rows)
		for c in range(cols + 1):
			var un := float(c) / float(cols)
			new_uvs.append(Vector2(un * tw, vn * th))
	poly.uv = new_uvs

	var facing := 1.0 if is_closeup else float(p.get("facing", 1.0))
	var sk: Dictionary = p.get("skeleton", {})
	var root_off: Vector2 = sk.get("root_offset", Vector2.ZERO)

	if not is_closeup:
		var px := float(p.get("x", 240.0))
		var py := float(p.get("y", 500.0))
		var pz := float(p.get("z", 0.0))
		var depth_scale := 0.90 + ((py - 300.0) / 360.0) * 0.22
		var h_scale := float(p.get("height_scale", 1.0))
		var sx := float(sk.get("squash_x", 1.0))
		var sy := float(sk.get("stretch_y", 1.0))
		var dive_mult := 0.74 if pose_key == "pose_dive" else 1.0
		var aspect_mult := (tw / th) / (384.0 / 512.0)
		root_node.position = Vector2(px, py - pz * 0.88 + 8.0) + root_off * 0.45
		root_node.scale = Vector2(facing * depth_scale * h_scale * sx * aspect_mult, depth_scale * h_scale * sy * dive_mult)
	else:
		root_node.position = Vector2(132.0, 340.0)
		root_node.scale = Vector2(1.0, 1.0)

	var k_scale := 0.0 if (is_closeup and pose_key == "plate_closeup") else (0.20 if is_closeup else 0.55)
	bones["Hip"].rotation_degrees = float(sk.get("hip_deg", 0.0)) * k_scale
	bones["Torso"].rotation_degrees = float(sk.get("torso_deg", 0.0)) * k_scale
	bones["Head"].rotation_degrees = float(sk.get("head_deg", 0.0)) * k_scale
	bones["Arm"].rotation_degrees = float(sk.get("arm_deg", 0.0)) * k_scale
	bones["HandSocket"].rotation_degrees = float(sk.get("forearm_deg", 0.0)) * k_scale
	bones["Thigh"].rotation_degrees = float(sk.get("thigh_deg", 0.0)) * k_scale
	bones["Shin"].rotation_degrees = float(sk.get("shin_deg", 0.0)) * k_scale
	bones["Foot"].rotation_degrees = float(sk.get("foot_deg", 0.0)) * k_scale

	var uv_gain := 0.0046 if is_closeup else 0.0038
	var dbl: Vector2 = p.get("disp_bust_l", Vector2.ZERO) * uv_gain
	var dbr: Vector2 = p.get("disp_bust_r", Vector2.ZERO) * uv_gain
	var dhl: Vector2 = p.get("disp_hip_l", Vector2.ZERO) * uv_gain * 0.9
	var dhr: Vector2 = p.get("disp_hip_r", Vector2.ZERO) * uv_gain * 0.9
	var dhair: Vector2 = p.get("disp_hair", Vector2.ZERO) * 0.0022

	mat.set_shader_parameter("u_tex_normal", nrm_tex)
	mat.set_shader_parameter("u_flip", facing)
	mat.set_shader_parameter("u_prop_scale", Vector2(float(p.get("bust_scale", 1.1)), float(p.get("hip_scale", 1.1))))
	if is_closeup and pose_key == "plate_closeup":
		mat.set_shader_parameter("u_zone_bust_l", p.get("closeup_zone_bust_l", Vector3(0.575, 0.485, 0.068)))
		mat.set_shader_parameter("u_zone_bust_r", p.get("closeup_zone_bust_r", Vector3(0.675, 0.485, 0.068)))
		mat.set_shader_parameter("u_zone_hip_l", p.get("closeup_zone_hip_l", Vector3(0.430, 0.765, 0.082)))
		mat.set_shader_parameter("u_zone_hip_r", p.get("closeup_zone_hip_r", Vector3(0.515, 0.775, 0.082)))
		mat.set_shader_parameter("u_zone_hair", p.get("closeup_zone_hair", Vector3(0.31, 0.25, 0.16)))
	else:
		mat.set_shader_parameter("u_zone_bust_l", p.get("zone_bust_l", Vector3(0.500, 0.375, 0.052)))
		mat.set_shader_parameter("u_zone_bust_r", p.get("zone_bust_r", Vector3(0.565, 0.375, 0.052)))
		mat.set_shader_parameter("u_zone_hip_l", p.get("zone_hip_l", Vector3(0.380, 0.525, 0.058)))
		mat.set_shader_parameter("u_zone_hip_r", p.get("zone_hip_r", Vector3(0.435, 0.530, 0.058)))
		mat.set_shader_parameter("u_zone_hair", p.get("zone_hair", Vector3(0.34, 0.22, 0.14)))
	mat.set_shader_parameter("u_disp_bust_l", dbl)
	mat.set_shader_parameter("u_disp_bust_r", dbr)
	mat.set_shader_parameter("u_disp_hip_l", dhl)
	mat.set_shader_parameter("u_disp_hip_r", dhr)
	mat.set_shader_parameter("u_disp_hair", dhair)
	mat.set_shader_parameter("u_suit_color", p.get("suit_primary", Vector3(0.96, 0.20, 0.42)))
	mat.set_shader_parameter("u_trim_color", p.get("suit_trim", Vector3(1.0, 0.82, 0.28)))
	mat.set_shader_parameter("u_hair_color", p.get("hair_color", Vector3(0.62, 0.22, 0.24)))
	mat.set_shader_parameter("u_skin_tint", p.get("skin_base", Vector3(0.98, 0.78, 0.67)))
	mat.set_shader_parameter("u_wetness", float(p.get("wetness", 0.25)))
	mat.set_shader_parameter("u_sand_coat", float(p.get("sand_coat", 0.08)))
	mat.set_shader_parameter("u_on_fire", 1.0 if bool(p.get("on_fire", false)) else 0.0)

	if theme_idx == 2:
		mat.set_shader_parameter("u_light_dir", Vector3(0.35, -0.45, 0.82))
		mat.set_shader_parameter("u_sun_color", Vector3(1.0, 0.28, 0.78))
	elif theme_idx == 1:
		mat.set_shader_parameter("u_light_dir", Vector3(-0.18, -0.28, 0.94))
		mat.set_shader_parameter("u_sun_color", Vector3(1.0, 0.98, 0.86))
	else:
		mat.set_shader_parameter("u_light_dir", Vector3(-0.45, -0.52, 0.72))
		mat.set_shader_parameter("u_sun_color", Vector3(1.0, 0.84, 0.42))

func apply_snapshot(snap: Dictionary) -> void:
	if snap.is_empty():
		return
	last_snapshot = snap

	var seq := int(snap.get("audio_seq", 0))
	if seq != last_audio_seq:
		last_audio_seq = seq
		var sfx_key := String(snap.get("last_sfx", ""))
		var vo_key := String(snap.get("last_vo", ""))
		if sfx_key != "":
			_play_sfx(sfx_key)
		if vo_key != "":
			_play_vo(vo_key)

	var shake := float(snap.get("screen_shake", 0.0))
	if shake > 0.01 and y_sort_layer != null:
		var sx := sin(elapsed_time * 95.0) * shake * 6.5
		var sy := cos(elapsed_time * 112.0) * shake * 5.0
		y_sort_layer.position = Vector2(sx, sy)
	elif y_sort_layer != null:
		y_sort_layer.position = Vector2.ZERO

	var theme_idx := int(snap.get("lighting_theme", 0))
	var b: Dictionary = snap.get("ball", {})
	var bx := float(b.get("x", 640.0))
	var by := float(b.get("y", 480.0))
	var bz := float(b.get("z", 42.0))
	var fire_val := 1.8 if bool(b.get("on_fire_ball", false)) else (1.0 if bool(b.get("solar_spike", false)) else 0.0)

	court_mat.set_shader_parameter("u_theme", theme_idx)
	court_mat.set_shader_parameter("u_time", elapsed_time)
	court_mat.set_shader_parameter("u_ball", Vector4(bx, by, bz, fire_val))

	var players: Array = snap.get("players", [])
	for i in range(min(4, players.size())):
		var p: Dictionary = players[i]
		var pv := Vector4(float(p.get("x", 0.0)), float(p.get("y", 0.0)), float(p.get("z", 0.0)), 1.0 if bool(p.get("on_fire", false)) else 0.0)
		court_mat.set_shader_parameter("u_p%d" % i, pv)
		_update_rig_from_player(athlete_rigs[i], p, theme_idx, false)

	if players.size() > 0:
		var show_studio := (ui_mode == UiMode.PLAY) and (bool(snap.get("pose_cam_active", false)) or float(snap.get("cut_in_timer", 0.0)) > 0.0)
		closeup_panel.visible = show_studio
		if show_studio:
			_update_rig_from_player(closeup_rig, players[0], theme_idx, true)

	if ball_sprite != null and ball_tex != null:
		ball_sprite.position = Vector2(bx, by - bz * 0.88)
		var b_diam := 28.0 / float(max(1, ball_tex.get_width()))
		ball_sprite.scale = Vector2(b_diam, b_diam)
		ball_sprite.rotation = float(b.get("spin_angle", 0.0))

	if hud_layer != null:
		hud_layer.queue_redraw()

var prev_j := false
var prev_k := false
var prev_l := false
var prev_sw := false
var prev_p := false
var prev_1 := false
var prev_2 := false
var prev_3 := false
var prev_f1 := false
var prev_f2 := false
var prev_f3 := false
var prev_esc := false
var prev_m := false
var prev_nav_up := false
var prev_nav_down := false
var prev_nav_left := false
var prev_nav_right := false

func _handle_menu_navigation() -> void:
	var nav_up := Input.is_key_pressed(KEY_W) or Input.is_key_pressed(KEY_UP) or Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_UP) or (Input.get_joy_axis(0, JOY_AXIS_LEFT_Y) < -0.45)
	var nav_down := Input.is_key_pressed(KEY_S) or Input.is_key_pressed(KEY_DOWN) or Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_DOWN) or (Input.get_joy_axis(0, JOY_AXIS_LEFT_Y) > 0.45)
	var nav_left := Input.is_key_pressed(KEY_A) or Input.is_key_pressed(KEY_LEFT) or Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_LEFT) or (Input.get_joy_axis(0, JOY_AXIS_LEFT_X) < -0.45)
	var nav_right := Input.is_key_pressed(KEY_D) or Input.is_key_pressed(KEY_RIGHT) or Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_RIGHT) or (Input.get_joy_axis(0, JOY_AXIS_LEFT_X) > 0.45)
	var confirm := (Input.is_key_pressed(KEY_J) or Input.is_key_pressed(KEY_SPACE) or Input.is_key_pressed(KEY_ENTER) or Input.is_joy_button_pressed(0, JOY_BUTTON_A)) and not prev_j

	var step_u := nav_up and not prev_nav_up
	var step_d := nav_down and not prev_nav_down
	var step_l := nav_left and not prev_nav_left
	var step_r := nav_right and not prev_nav_right

	prev_nav_up = nav_up
	prev_nav_down = nav_down
	prev_nav_left = nav_left
	prev_nav_right = nav_right

	if ui_mode == UiMode.TITLE:
		if step_u:
			title_cursor = (title_cursor + 5) % 6
			_play_sfx("sfx_set")
		elif step_d:
			title_cursor = (title_cursor + 1) % 6
			_play_sfx("sfx_set")

		if step_l or step_r:
			var delta_dir := 1 if step_r else -1
			if title_cursor == 2:
				selected_p1_roster = (selected_p1_roster + delta_dir + 6) % 6
				match_bridge.set_slot_roster(0, selected_p1_roster)
				_play_sfx("sfx_dig")
			elif title_cursor == 3:
				selected_p2_roster = (selected_p2_roster + delta_dir + 6) % 6
				match_bridge.set_slot_roster(1, selected_p2_roster)
				_play_sfx("sfx_dig")
			elif title_cursor == 4:
				var cur_t := int(last_snapshot.get("lighting_theme", 0))
				match_bridge.set_lighting_theme((cur_t + delta_dir + 3) % 3)
				_play_sfx("sfx_dig")
			elif title_cursor == 5:
				var cur_p := int(last_snapshot.get("physics_preset", 1))
				match_bridge.set_physics_preset((cur_p + delta_dir + 3) % 3)
				_play_sfx("sfx_dig")

		if confirm:
			if title_cursor == 0:
				_start_match(false)
			elif title_cursor == 1:
				_start_match(true)
			elif title_cursor == 2:
				selected_p1_roster = (selected_p1_roster + 1) % 6
				match_bridge.set_slot_roster(0, selected_p1_roster)
				_play_sfx("sfx_dig")
			elif title_cursor == 3:
				selected_p2_roster = (selected_p2_roster + 1) % 6
				match_bridge.set_slot_roster(1, selected_p2_roster)
				_play_sfx("sfx_dig")
			elif title_cursor == 4:
				var cur_t := int(last_snapshot.get("lighting_theme", 0))
				match_bridge.set_lighting_theme((cur_t + 1) % 3)
				_play_sfx("sfx_dig")
			elif title_cursor == 5:
				var cur_p := int(last_snapshot.get("physics_preset", 1))
				match_bridge.set_physics_preset((cur_p + 1) % 3)
				_play_sfx("sfx_dig")

	elif ui_mode == UiMode.PAUSE:
		if step_u:
			pause_cursor = (pause_cursor + 5) % 6
			_play_sfx("sfx_set")
		elif step_d:
			pause_cursor = (pause_cursor + 1) % 6
			_play_sfx("sfx_set")

		if step_l or step_r:
			var delta_dir := 1 if step_r else -1
			if pause_cursor == 1:
				var cur_ai := bool(last_snapshot.get("attract_mode", false))
				match_bridge.set_attract_mode(not cur_ai)
				_play_sfx("sfx_dig")
			elif pause_cursor == 2:
				var cur_t := int(last_snapshot.get("lighting_theme", 0))
				match_bridge.set_lighting_theme((cur_t + delta_dir + 3) % 3)
				_play_sfx("sfx_dig")
			elif pause_cursor == 3:
				var cur_p := int(last_snapshot.get("physics_preset", 1))
				match_bridge.set_physics_preset((cur_p + delta_dir + 3) % 3)
				_play_sfx("sfx_dig")

		if confirm:
			if pause_cursor == 0:
				ui_mode = UiMode.PLAY
				_play_sfx("sfx_set")
			elif pause_cursor == 1:
				var cur_ai := bool(last_snapshot.get("attract_mode", false))
				match_bridge.set_attract_mode(not cur_ai)
				ui_mode = UiMode.PLAY
				_play_sfx("sfx_whistle")
			elif pause_cursor == 2:
				var cur_t := int(last_snapshot.get("lighting_theme", 0))
				match_bridge.set_lighting_theme((cur_t + 1) % 3)
				_play_sfx("sfx_dig")
			elif pause_cursor == 3:
				var cur_p := int(last_snapshot.get("physics_preset", 1))
				match_bridge.set_physics_preset((cur_p + 1) % 3)
				_play_sfx("sfx_dig")
			elif pause_cursor == 4:
				match_bridge.reset_match()
				ui_mode = UiMode.PLAY
				_play_sfx("sfx_whistle")
			elif pause_cursor == 5:
				match_bridge.set_attract_mode(true)
				match_bridge.reset_match()
				ui_mode = UiMode.TITLE
				_play_vo("vo_sunbreak_title")

func _process(delta: float) -> void:
	elapsed_time += delta
	if match_bridge == null:
		return

	var key_esc := Input.is_key_pressed(KEY_ESCAPE) or Input.is_joy_button_pressed(0, JOY_BUTTON_START)
	var key_m := Input.is_key_pressed(KEY_M)
	var key_p := Input.is_key_pressed(KEY_P)
	var key_1 := Input.is_key_pressed(KEY_1) or Input.is_key_pressed(KEY_KP_1)
	var key_2 := Input.is_key_pressed(KEY_2) or Input.is_key_pressed(KEY_KP_2)
	var key_3 := Input.is_key_pressed(KEY_3) or Input.is_key_pressed(KEY_KP_3)
	var key_f1 := Input.is_key_pressed(KEY_F1) or Input.is_key_pressed(KEY_R)
	var key_f2 := Input.is_key_pressed(KEY_F2)
	var key_f3 := Input.is_key_pressed(KEY_F3)

	if key_m and not prev_m:
		audio_muted = not audio_muted
		if bgm_player != null:
			bgm_player.stream_paused = audio_muted
	prev_m = key_m

	if key_esc and not prev_esc:
		if ui_mode == UiMode.PLAY:
			ui_mode = UiMode.PAUSE
			pause_cursor = 0
			_play_sfx("sfx_set")
		elif ui_mode == UiMode.PAUSE:
			ui_mode = UiMode.PLAY
			_play_sfx("sfx_set")
		elif ui_mode == UiMode.TITLE:
			_start_match(false)
	prev_esc = key_esc

	if key_p and not prev_p:
		var cur := bool(last_snapshot.get("pose_cam_active", false))
		match_bridge.set_pose_cam_active(not cur)
	if key_1 and not prev_1:
		match_bridge.set_lighting_theme(0)
	if key_2 and not prev_2:
		match_bridge.set_lighting_theme(1)
	if key_3 and not prev_3:
		match_bridge.set_lighting_theme(2)
	if key_f1 and not prev_f1:
		match_bridge.reset_match()
	if key_f2 and not prev_f2:
		apply_preset("sunset_pose")
	if key_f3 and not prev_f3:
		apply_preset("neon_deuce")

	prev_p = key_p
	prev_1 = key_1
	prev_2 = key_2
	prev_3 = key_3
	prev_f1 = key_f1
	prev_f2 = key_f2
	prev_f3 = key_f3

	var mx := Input.get_axis("ui_left", "ui_right")
	if Input.is_key_pressed(KEY_A):
		mx -= 1.0
	if Input.is_key_pressed(KEY_D):
		mx += 1.0
	var joy_x := Input.get_joy_axis(0, JOY_AXIS_LEFT_X)
	if absf(joy_x) > 0.18:
		mx += joy_x
	if Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_LEFT):
		mx -= 1.0
	if Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_RIGHT):
		mx += 1.0
	mx = clampf(mx, -1.0, 1.0)

	var my := Input.get_axis("ui_up", "ui_down")
	if Input.is_key_pressed(KEY_W):
		my -= 1.0
	if Input.is_key_pressed(KEY_S):
		my += 1.0
	var joy_y := Input.get_joy_axis(0, JOY_AXIS_LEFT_Y)
	if absf(joy_y) > 0.18:
		my += joy_y
	if Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_UP):
		my -= 1.0
	if Input.is_joy_button_pressed(0, JOY_BUTTON_DPAD_DOWN):
		my += 1.0
	my = clampf(my, -1.0, 1.0)

	var hold_a := Input.is_key_pressed(KEY_J) or Input.is_key_pressed(KEY_SPACE) or Input.is_key_pressed(KEY_Z) or Input.is_key_pressed(KEY_ENTER) or Input.is_joy_button_pressed(0, JOY_BUTTON_A)
	var hold_b := Input.is_key_pressed(KEY_K) or Input.is_key_pressed(KEY_X) or Input.is_joy_button_pressed(0, JOY_BUTTON_X) or Input.is_joy_button_pressed(0, JOY_BUTTON_Y)
	var hold_dive := Input.is_key_pressed(KEY_L) or Input.is_key_pressed(KEY_C) or Input.is_joy_button_pressed(0, JOY_BUTTON_B)
	var hold_sw := Input.is_key_pressed(KEY_SEMICOLON) or Input.is_key_pressed(KEY_TAB) or Input.is_key_pressed(KEY_U) or Input.is_joy_button_pressed(0, JOY_BUTTON_LEFT_SHOULDER)
	var hold_turbo := Input.is_key_pressed(KEY_SHIFT) or Input.is_joy_button_pressed(0, JOY_BUTTON_RIGHT_SHOULDER) or (Input.get_joy_axis(0, JOY_AXIS_TRIGGER_RIGHT) > 0.35)

	if ui_mode != UiMode.PLAY:
		_handle_menu_navigation()
		prev_j = hold_a
		prev_k = hold_b
		prev_l = hold_dive
		prev_sw = hold_sw
		if ui_mode == UiMode.TITLE:
			# Step background 4x AI attract exhibition while on the Title Screen!
			match_bridge.step_frame(delta, {})
		apply_snapshot(match_bridge.get_snapshot())
		return

	var btn_a := hold_a and not prev_j
	var btn_b := hold_b and not prev_k
	var btn_dive := hold_dive and not prev_l
	var btn_switch := hold_sw and not prev_sw

	prev_j = hold_a
	prev_k = hold_b
	prev_l = hold_dive
	prev_sw = hold_sw

	var any_human_input := (absf(mx) > 0.1 or absf(my) > 0.1 or hold_a or hold_b or hold_dive or hold_sw or hold_turbo)
	var is_attract := bool(last_snapshot.get("attract_mode", false))
	if any_human_input:
		idle_timer = 0.0
		# If watching 4-AI Attract Mode and the user presses a button, jump straight into 1P control!
		if is_attract and (btn_a or btn_b or btn_dive):
			match_bridge.set_attract_mode(false)
			_play_sfx("sfx_whistle")
	else:
		idle_timer += delta
		# Auto-enable 4-AI Idle Attract Mode after 25s of inactivity during a match!
		if not is_attract and idle_timer >= 25.0 and not bool(last_snapshot.get("demo_freeze", false)):
			match_bridge.set_attract_mode(true)

	var p0_in := {
		"mx": mx,
		"my": my,
		"btn_a": btn_a,
		"btn_b": btn_b,
		"btn_dive": btn_dive,
		"hold_a": hold_a,
		"hold_b": hold_b,
		"btn_switch": btn_switch,
		"turbo": hold_turbo
	}
	match_bridge.step_frame(delta, p0_in)
	apply_snapshot(match_bridge.get_snapshot())

func _draw_hud(canvas: Node2D) -> void:
	if last_snapshot.is_empty():
		return
	var font := ThemeDB.fallback_font
	var theme_idx := int(last_snapshot.get("lighting_theme", 0))
	var studio_rect := Rect2(845.0, 64.0, 410.0, 360.0)
	var studio_open := closeup_panel != null and closeup_panel.visible

	# 1. Perspective Volleyball Net at X = 640
	var net_x := 640.0
	var net_top_y := 232.0
	var net_bot_y := 656.0
	canvas.draw_line(Vector2(net_x + 22.0, 296.0), Vector2(net_x + 22.0, net_bot_y), Color(0.06, 0.04, 0.12, 0.38), 9.0)
	canvas.draw_rect(Rect2(net_x - 4.0, net_top_y, 8.0, net_bot_y - net_top_y - 36.0), Color(0.10, 0.14, 0.22, 0.45), true)
	var post_col := Color(1.0, 0.21, 0.71) if theme_idx == 2 else Color(1.0, 1.0, 1.0)
	canvas.draw_rect(Rect2(net_x - 5.0, net_top_y - 4.0, 10.0, net_bot_y - net_top_y - 36.0), post_col, true)
	canvas.draw_rect(Rect2(net_x - 8.0, net_top_y - 8.0, 16.0, 12.0), Color(1.0, 0.28, 0.49), true)
	canvas.draw_rect(Rect2(net_x - 8.0, net_bot_y - 12.0, 16.0, 16.0), Color(1.0, 0.28, 0.49), true)

	# 2. Ball Landing Target Reticle on Sand (at playable waist/arm reception height)
	var b: Dictionary = last_snapshot.get("ball", {})
	var landing: Vector2 = b.get("landing", Vector2(640.0, 480.0))
	var ret_col := Color(1.0, 0.33, 0.0) if bool(b.get("on_fire_ball", false)) else Color(0.17, 0.90, 1.0)
	canvas.draw_arc(landing, 18.0, 0.0, TAU, 24, ret_col, 2.2)
	canvas.draw_line(landing + Vector2(-8.0, 0.0), landing + Vector2(8.0, 0.0), ret_col, 2.0)
	canvas.draw_line(landing + Vector2(0.0, -5.0), landing + Vector2(0.0, 5.0), ret_col, 2.0)

	# 3. Top Scoreboard, Solar Overdrive Gauges, & Deuce / Trade-Wind Broadcast Pill
	canvas.draw_rect(Rect2(0.0, 0.0, 1280.0, 54.0), Color(0.03, 0.05, 0.10, 0.92), true)
	var mode_badge := "4-AI SPECTATOR" if bool(last_snapshot.get("attract_mode", false)) else "15-PT SET • WIN BY 2"
	canvas.draw_string(font, Vector2(20.0, 32.0), "SUNBREAK // V'BALL (%s)" % mode_badge, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(1.0, 0.72, 0.30))

	var players: Array = last_snapshot.get("players", [])
	var left_names := "KASUMI & ELENA"
	var right_names := "NYX & FREYA"
	if players.size() >= 4:
		left_names = "%s & %s" % [String(players[0].get("name", "KASUMI")), String(players[1].get("name", "ELENA"))]
		right_names = "%s & %s" % [String(players[2].get("name", "NYX")), String(players[3].get("name", "FREYA"))]

	var s_l := int(last_snapshot.get("score_left", 0))
	var s_r := int(last_snapshot.get("score_right", 0))
	var score_str := "%s   %d  —  %d   %s" % [left_names, s_l, s_r, right_names]
	canvas.draw_string(font, Vector2(455.0, 28.0), score_str, HORIZONTAL_ALIGNMENT_LEFT, -1, 17, Color(0.95, 0.98, 1.0))

	# Solar Overdrive Meters under team names
	var sol_l := clampf(float(last_snapshot.get("solar_left", 0.0)) / 100.0, 0.0, 1.0)
	var sol_r := clampf(float(last_snapshot.get("solar_right", 0.0)) / 100.0, 0.0, 1.0)
	canvas.draw_rect(Rect2(455.0, 36.0, 150.0, 8.0), Color(0.10, 0.14, 0.22, 0.9), true)
	canvas.draw_rect(Rect2(455.0, 36.0, 150.0 * sol_l, 8.0), Color(1.0, 0.38, 0.08) if sol_l >= 1.0 else Color(1.0, 0.75, 0.22), true)
	canvas.draw_string(font, Vector2(392.0, 44.0), "SOLAR", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(1.0, 0.75, 0.22))

	canvas.draw_rect(Rect2(675.0, 36.0, 150.0, 8.0), Color(0.10, 0.14, 0.22, 0.9), true)
	canvas.draw_rect(Rect2(675.0, 36.0, 150.0 * sol_r, 8.0), Color(1.0, 0.38, 0.08) if sol_r >= 1.0 else Color(0.17, 0.90, 1.0), true)
	canvas.draw_string(font, Vector2(832.0, 44.0), "SOLAR", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0.17, 0.90, 1.0))

	var wind_lbl := String(last_snapshot.get("wind_label", "12 KT -> E"))
	var tac_l := "BLOCK NET [;]" if String(last_snapshot.get("tactic_left", "block")) == "block" else "PEEL DEEP [;]"
	var is_deuce := bool(last_snapshot.get("is_deuce", false))
	var pill_str := "WIND: %s  |  PARTNER: %s%s" % [wind_lbl, tac_l, "  |  DEUCE (WIN BY 2)" if is_deuce else ""]
	canvas.draw_rect(Rect2(430.0, 62.0, 420.0, 26.0), Color(0.04, 0.07, 0.14, 0.86), true)
	canvas.draw_rect(Rect2(430.0, 62.0, 420.0, 26.0), Color(1.0, 0.72, 0.30) if is_deuce else Color(0.17, 0.90, 1.0, 0.65), false, 1.6)
	canvas.draw_string(font, Vector2(444.0, 80.0), pill_str, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.92, 0.96, 1.0))

	# 4. Athlete Nameplates, Turbo Stamina Bar, Partner Tactic Pills, and Serve Bar
	for i in range(players.size()):
		var p: Dictionary = players[i]
		var px := float(p.get("x", 240.0))
		var py := float(p.get("y", 500.0))
		var pz := float(p.get("z", 0.0))
		var sy := py - pz * 0.88 - 142.0
		var tag_rect := Rect2(px - 54.0, sy - 16.0, 108.0, 38.0)
		if studio_open and studio_rect.intersects(tag_rect):
			continue
		var pname := String(p.get("name", "ATHLETE"))
		var p_is_ai := bool(p.get("is_ai", i != 0))
		var tag := (("AI • " if p_is_ai else "P1 • ") + pname) if i == 0 else (("PARTNER • " + pname) if i == 1 else ("CPU • " + pname))
		canvas.draw_rect(Rect2(px - 54.0, sy - 16.0, 108.0, 18.0), Color(0.03, 0.05, 0.10, 0.82), true)
		var ncol := Color(1.0, 0.45, 0.15) if bool(p.get("on_fire", false)) else Color(0.92, 0.96, 1.0)
		canvas.draw_string(font, Vector2(px - 48.0, sy - 3.0), tag, HORIZONTAL_ALIGNMENT_LEFT, -1, 11, ncol)

		# Stamina / Turbo bar under P1
		if i == 0:
			var tb := clampf(float(p.get("turbo", 100.0)) / 100.0, 0.0, 1.0)
			canvas.draw_rect(Rect2(px - 54.0, sy + 2.0, 108.0, 4.0), Color(0.08, 0.12, 0.18, 0.85), true)
			canvas.draw_rect(Rect2(px - 54.0, sy + 2.0, 108.0 * tb, 4.0), Color(0.20, 0.92, 0.65), true)

		if i == 1 or i == 2:
			var t_str := ("BLOCK NET [;]" if String(last_snapshot.get("tactic_left", "block")) == "block" else "PEEL DEEP [;]") if i == 1 else "PEEL DEEP"
			var t_col := Color(1.0, 0.72, 0.30) if i == 1 else Color(0.17, 0.90, 1.0)
			canvas.draw_rect(Rect2(px - 44.0, sy + 5.0, 88.0, 15.0), Color(0.03, 0.05, 0.10, 0.86), true)
			canvas.draw_string(font, Vector2(px - 38.0, sy + 16.0), t_str, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, t_col)

	if int(last_snapshot.get("rally_state", 0)) == 0 and int(last_snapshot.get("serve_phase", 0)) == 1:
		var srv_slot := int(last_snapshot.get("server_slot", 0))
		if srv_slot < players.size():
			var sp: Dictionary = players[srv_slot]
			var bx_bar := float(sp.get("x", 240.0)) - 68.0
			var by_bar := float(sp.get("y", 500.0)) - 135.0
			var bw := 16.0
			var bh := 96.0
			var chg := clampf(float(last_snapshot.get("serve_charge", 0.84)), 0.0, 1.0)
			canvas.draw_rect(Rect2(bx_bar - 2.0, by_bar - 2.0, bw + 4.0, bh + 4.0), Color(0.03, 0.05, 0.10, 0.92), true)
			canvas.draw_rect(Rect2(bx_bar, by_bar + bh * (1.0 - chg), bw, bh * chg), Color(1.0, 0.72, 0.30), true)
			var sweet_y := by_bar + bh * (1.0 - 0.90)
			var sweet_h := bh * 0.15
			canvas.draw_rect(Rect2(bx_bar - 2.0, sweet_y, bw + 4.0, sweet_h), Color(0.20, 0.83, 0.60), false, 2.0)
			canvas.draw_string(font, Vector2(bx_bar - 18.0, by_bar - 6.0), "★ SWEET ★", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0.20, 0.83, 0.60))

	# 5. Callout Banners on Court
	var callouts: Array = last_snapshot.get("callouts", [])
	for c in callouts:
		var ctext := String(c.get("text", ""))
		var cx := float(c.get("x", 640.0))
		var cy := float(c.get("y", 260.0))
		var ccol := Color.from_string(String(c.get("color", "#ff5500")), Color(1.0, 0.45, 0.12))
		canvas.draw_string(font, Vector2(cx - 120.0, cy), ctext, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, ccol)

	# Attract Mode Banner when watching All-AI Exhibition in PLAY mode
	if ui_mode == UiMode.PLAY and bool(last_snapshot.get("attract_mode", false)):
		canvas.draw_rect(Rect2(420.0, 96.0, 440.0, 30.0), Color(0.03, 0.05, 0.12, 0.88), true)
		canvas.draw_rect(Rect2(420.0, 96.0, 440.0, 30.0), Color(1.0, 0.28, 0.49, 0.9), false, 1.8)
		canvas.draw_string(font, Vector2(442.0, 116.0), "ATTRACT EXHIBITION (4x AI) • PRESS [J / SPACE / PAD-A] TO PLAY!", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(1.0, 0.82, 0.35))

	# 6. Pose & Skeleton2D Telemetry Studio Overlay
	if closeup_panel != null and closeup_panel.visible and players.size() > 0:
		var p0: Dictionary = players[0]
		var cx := 845.0
		var cy := 64.0
		var cw := 410.0
		var ch := 360.0
		var b_col := Color(1.0, 0.33, 0.0) if float(last_snapshot.get("cut_in_timer", 0.0)) > 0.0 else Color(1.0, 0.28, 0.49)
		canvas.draw_rect(Rect2(cx, cy, cw, ch), b_col, false, 2.2)
		canvas.draw_string(font, Vector2(cx + 14.0, cy + 22.0), "SKELETON2D & SOFT-BODY STUDIO // %s" % String(p0.get("name", "KASUMI")), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, b_col)

		var tx := cx + 236.0
		var ty := cy + 52.0
		var pkey := String(p0.get("pose_key", "pose_spike")).replace("pose_", "").to_upper()
		canvas.draw_string(font, Vector2(tx, ty), "Pose: %s" % pkey, HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.84, 0.90, 0.96))
		ty += 18.0
		canvas.draw_string(font, Vector2(tx, ty), "Rig: 8-BONE SKELETON2D", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.20, 0.83, 0.60))
		ty += 18.0
		canvas.draw_string(font, Vector2(tx, ty), "Opacity: 100% (ZERO GHOST)", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.84, 0.90, 0.96))
		ty += 18.0
		canvas.draw_string(font, Vector2(tx, ty), "Wet: %d%% • Sand: %d%%" % [int(round(float(p0.get("wetness", 0.32)) * 100.0)), int(round(float(p0.get("sand_coat", 0.08)) * 100.0))], HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.84, 0.90, 0.96))
		ty += 18.0

		canvas.draw_rect(Rect2(tx, ty, 156.0, 88.0), Color(0.02, 0.04, 0.09, 0.88), true)
		canvas.draw_rect(Rect2(tx, ty, 156.0, 88.0), Color(1.0, 1.0, 1.0, 0.22), false, 1.2)
		canvas.draw_string(font, Vector2(tx + 8.0, ty + 14.0), "8-BONE + SPRING NODES", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0.65, 0.75, 0.88))

		var mid := Vector2(tx + 78.0, ty + 54.0)
		var dbl: Vector2 = p0.get("disp_bust_l", Vector2.ZERO)
		var dbr: Vector2 = p0.get("disp_bust_r", Vector2.ZERO)
		var dhl: Vector2 = p0.get("disp_hip_l", Vector2.ZERO)
		var dhr: Vector2 = p0.get("disp_hip_r", Vector2.ZERO)
		canvas.draw_circle(mid + Vector2(-18.0, -8.0) + dbl * 1.5, 5.0, Color(1.0, 0.28, 0.49))
		canvas.draw_circle(mid + Vector2(18.0, -8.0) + dbr * 1.5, 5.0, Color(1.0, 0.72, 0.30))
		canvas.draw_circle(mid + Vector2(-14.0, 12.0) + dhl * 1.5, 4.5, Color(0.17, 0.90, 1.0))
		canvas.draw_circle(mid + Vector2(14.0, 12.0) + dhr * 1.5, 4.5, Color(0.20, 0.83, 0.60))

	# 7. Bottom Controls Legend Bar
	canvas.draw_rect(Rect2(50.0, 686.0, 1180.0, 26.0), Color(0.03, 0.06, 0.12, 0.88), true)
	canvas.draw_rect(Rect2(50.0, 686.0, 1180.0, 26.0), Color(0.17, 0.90, 1.0, 0.35), false, 1.0)
	canvas.draw_string(font, Vector2(66.0, 703.0), "[WASD/Stick] Move   [J/Space/Pad-A] Serve/Pass   [K/Pad-X] Jump-Spike (+Back=Dink)   [L/Pad-B] Dive   [;/Tab/LB] Tactic   [ESC/Start] Menu   [M] Mute   [P] Studio", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, Color(0.84, 0.92, 0.98))

	# 8. Title Screen or Pause Menu Modal Overlay
	if ui_mode == UiMode.TITLE or ui_mode == UiMode.PAUSE:
		var m_rect := Rect2(310.0, 125.0, 660.0, 445.0)
		canvas.draw_rect(m_rect, Color(0.02, 0.04, 0.10, 0.92), true)
		canvas.draw_rect(m_rect, Color(1.0, 0.28, 0.52, 0.95), false, 2.6)
		canvas.draw_rect(Rect2(316.0, 131.0, 648.0, 54.0), Color(0.08, 0.12, 0.24, 0.95), true)

		var title_txt := "SUNBREAK // BEACH VOLLEYBALL — MAIN MENU" if ui_mode == UiMode.TITLE else "MATCH PAUSED // TACTICAL & STUDIO OPTIONS"
		canvas.draw_string(font, Vector2(336.0, 165.0), title_txt, HORIZONTAL_ALIGNMENT_LEFT, -1, 19, Color(1.0, 0.76, 0.32))

		var cur_theme: String = String(THEME_NAMES[int(last_snapshot.get("lighting_theme", 0)) % 3])
		var cur_phys: String = String(PHYS_NAMES[int(last_snapshot.get("physics_preset", 1)) % 3])
		var items: Array[String] = []
		var active_cursor := title_cursor if ui_mode == UiMode.TITLE else pause_cursor

		if ui_mode == UiMode.TITLE:
			items = [
				"▶  PLAY 1P + PARTNER vs CPU  (15-PT SET)",
				"▶  WATCH 4x AI ATTRACT EXHIBITION  (IDLE SPECTATOR)",
				"◀  P1 ATHLETE:  %s  ▶" % String(ROSTER_NAMES[selected_p1_roster]),
				"◀  PARTNER AI:  %s  ▶" % String(ROSTER_NAMES[selected_p2_roster]),
				"◀  LIGHTING THEME:  %s  ▶" % cur_theme,
				"◀  SOFT-BODY RIG:  %s  ▶" % cur_phys
			]
		else:
			var ai_mode_str := "ON (4x AI SPECTATOR)" if bool(last_snapshot.get("attract_mode", false)) else "OFF (1P HUMAN CONTROL)"
			items = [
				"▶  RESUME MATCH",
				"◀  IDLE / SPECTATOR MODE:  %s  ▶" % ai_mode_str,
				"◀  LIGHTING THEME:  %s  ▶" % cur_theme,
				"◀  SOFT-BODY RIG:  %s  ▶" % cur_phys,
				"▶  RESTART SET (0 — 0)",
				"▶  RETURN TO TITLE SCREEN"
			]

		for idx in range(items.size()):
			var iy := 215.0 + float(idx) * 46.0
			var is_sel := (idx == active_cursor)
			if is_sel:
				canvas.draw_rect(Rect2(334.0, iy - 22.0, 612.0, 36.0), Color(1.0, 0.28, 0.52, 0.26), true)
				canvas.draw_rect(Rect2(334.0, iy - 22.0, 612.0, 36.0), Color(0.17, 0.90, 1.0, 0.95), false, 1.8)
			var icol := Color(1.0, 0.96, 0.82) if is_sel else Color(0.76, 0.84, 0.94)
			canvas.draw_string(font, Vector2(352.0, iy), items[idx], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, icol)

		canvas.draw_string(font, Vector2(342.0, 518.0), "TIP: Hold [A / Left] during Jump-Spike for a Velvet Drop Dink over blockers!", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.20, 0.88, 0.65))
		canvas.draw_string(font, Vector2(342.0, 544.0), "[W/S or D-Pad] Select   [A/D] Change Option   [J / Space / Enter / Pad-A] Confirm   [ESC / Start] Close", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.68, 0.80, 0.95))
