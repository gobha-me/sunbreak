extends SceneTree

func _initialize() -> void:
	call_deferred("_run_contracts")

func _require(cond: bool, msg: String) -> void:
	if not cond:
		push_error("CONTRACT FAILURE: " + msg)
		quit(1)

func _run_contracts() -> void:
	_require(ClassDB.class_exists("SunbreakMatch"), "SunbreakMatch GDExtension class missing")
	var bridge: RefCounted = ClassDB.instantiate("SunbreakMatch")
	_require(bridge != null, "Failed to instantiate SunbreakMatch")

	var sub_vp := SubViewport.new()
	sub_vp.size = Vector2i(1280, 720)
	sub_vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	sub_vp.transparent_bg = false
	root.add_child(sub_vp)

	var main_script: Script = load("res://main.gd")
	_require(main_script != null and main_script.can_instantiate(), "Failed to compile/load res://main.gd")

	var main_scene: PackedScene = load("res://main.tscn")
	_require(main_scene != null, "Failed to load res://main.tscn")
	var inst := main_scene.instantiate()
	_require(inst != null and inst.get("athlete_rigs") != null, "Failed to instantiate main.gd on root node")
	sub_vp.add_child(inst)
	for _i in range(4):
		await process_frame

	_require(inst.athlete_rigs.size() == 4, "Expected 4 on-court AthleteRig2D instances")
	_require(inst.audio_clips.size() == 20, "Expected all 20 Venice BGM/SFX/TTS WAV clips loaded")
	var required_bones := ["Hip", "Torso", "Head", "Arm", "HandSocket", "Thigh", "Shin", "Foot"]
	for i in range(4):
		var rig: Dictionary = inst.athlete_rigs[i]
		var skel: Skeleton2D = rig["skeleton"]
		var poly: Polygon2D = rig["polygon"]
		_require(skel != null, "Missing Skeleton2D on AthleteRig_%d" % i)
		_require(poly != null, "Missing Polygon2D DeformMesh on AthleteRig_%d" % i)
		_require(poly.get_bone_count() == 8, "Expected 8 weighted bones on Polygon2D for AthleteRig_%d" % i)
		for bname in required_bones:
			_require(rig["bones"].has(bname), "Missing Bone2D %s on AthleteRig_%d" % [bname, i])

	# Verify Attract Mode & Roster Selection via GDExtension bridge
	bridge.set_attract_mode(true)
	bridge.set_slot_roster(0, 2)
	var snap_attract: Dictionary = bridge.get_snapshot()
	_require(bool(snap_attract.get("attract_mode", false)), "Expected attract_mode == true on snapshot")
	_require(String(snap_attract["players"][0]["name"]) == "MEI-LIN", "Expected slot 0 roster MEI-LIN")
	bridge.set_attract_mode(false)
	bridge.set_slot_roster(0, 0)

	# Verify 4-Stage Keyframed Jump Chain (`Stand -> Squat -> Stand Arms-Up -> In-Air`) via GDExtension bridge
	bridge.reset_match()
	bridge.trigger_jump(0, 280.0)
	var snap_coil: Dictionary = bridge.get_snapshot()
	var p0_coil: Dictionary = snap_coil["players"][0]
	_require(String(p0_coil["action_state"]) == "jump_coil", "Expected Stage 2 jump_coil (Squat) on jump trigger")

	for _s in range(3):
		bridge.step_frame(0.045, {})
	var snap_launch: Dictionary = bridge.get_snapshot()
	var p0_launch: Dictionary = snap_launch["players"][0]
	_require(String(p0_launch["action_state"]) == "jump_launch", "Expected Stage 3 jump_launch (Stand Arms-Up) after Squat coil")
	_require(String(p0_launch["pose_key"]) == "pose_set", "Expected pose_set during Stage 3 jump_launch")

	for _s in range(3):
		bridge.step_frame(0.045, {})
	var snap_air: Dictionary = bridge.get_snapshot()
	var p0_air: Dictionary = snap_air["players"][0]
	_require(String(p0_air["action_state"]) == "spike", "Expected Stage 4 spike (In-Air Bow-Arch) at apex")
	_require(String(p0_air["pose_key"]) == "pose_spike", "Expected pose_spike at apex")

	# Apply both showcase presets ("sunset_pose" and "neon_deuce") and capture SubViewport frames
	var snap_sunset: Dictionary = inst.apply_preset("sunset_pose")
	_require(absf(float(snap_sunset.get("serve_charge", 0.0)) - 0.84) < 0.01, "Expected 0.84 serve_charge in sunset_pose preset")
	for _i in range(4):
		await process_frame

	var cap_dir := OS.get_environment("SUNBREAK_CAPTURE_DIR")
	if cap_dir != "":
		RenderingServer.force_draw(false)
		var img_sunset := sub_vp.get_texture().get_image()
		if img_sunset != null and not img_sunset.is_empty():
			img_sunset.save_png(cap_dir.path_join("godot_sunbreak_sunset.png"))

	var snap_neon: Dictionary = inst.apply_preset("neon_deuce")
	_require(bool(snap_neon.get("is_deuce", false)), "Expected Deuce state in neon_deuce preset")
	_require(bool(snap_neon.get("on_fire_left", false)), "Expected Team 0 ON FIRE in neon_deuce preset")
	for _i in range(4):
		await process_frame

	if cap_dir != "":
		RenderingServer.force_draw(false)
		var img_neon := sub_vp.get_texture().get_image()
		if img_neon != null and not img_neon.is_empty():
			img_neon.save_png(cap_dir.path_join("godot_sunbreak_neon.png"))

	print("SUNBREAK GODOT 4.7.2 + C++23 NATIVE CONTRACT SUITE: ALL CHECKS PASSED")
	quit(0)
