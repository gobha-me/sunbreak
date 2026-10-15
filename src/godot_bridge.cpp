#include "sunbreak/match_engine.hpp"

#include <algorithm>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace sunbreak {

namespace {

[[nodiscard]] auto to_gd_str(std::string_view sv) -> godot::String {
  return godot::String::utf8(sv.data(), static_cast<std::int64_t>(sv.size()));
}

[[nodiscard]] auto to_gd_vec2(const Vec2& v) -> godot::Vector2 {
  return godot::Vector2(static_cast<real_t>(v.x), static_cast<real_t>(v.y));
}

[[nodiscard]] auto to_gd_vec3(const Vec3& v) -> godot::Vector3 {
  return godot::Vector3(static_cast<real_t>(v.x), static_cast<real_t>(v.y),
                        static_cast<real_t>(v.z));
}

[[nodiscard]] auto to_gd_zone(const ZoneUvRadius& z) -> godot::Vector3 {
  return godot::Vector3(static_cast<real_t>(z.u), static_cast<real_t>(z.v),
                        static_cast<real_t>(z.r));
}

} // namespace

class SunbreakMatch final : public godot::RefCounted {
  GDCLASS(SunbreakMatch, godot::RefCounted)

 public:
  void reset_match() { engine_.reset_match(); }

  void apply_preset(const godot::String& preset_name) {
    const auto utf8 = preset_name.utf8();
    engine_.apply_preset(std::string_view(utf8.get_data(), utf8.length()));
  }

  void trigger_jump(std::int64_t slot, double jump_vel) {
    engine_.trigger_keyed_jump(
        static_cast<std::uint8_t>(std::clamp<std::int64_t>(slot, 0, 3)),
        jump_vel);
  }

  void start_serve_toss(std::int64_t slot, bool is_jump_serve) {
    engine_.start_serve_toss(
        static_cast<std::uint8_t>(std::clamp<std::int64_t>(slot, 0, 3)),
        is_jump_serve);
  }

  void execute_serve_strike(std::int64_t slot, double aim_y,
                            double charge_override) {
    PlayerInput in{};
    in.my = aim_y;
    engine_.execute_serve_strike(
        static_cast<std::uint8_t>(std::clamp<std::int64_t>(slot, 0, 3)), in,
        charge_override);
  }

  [[nodiscard]] auto toggle_partner_tactic(std::int64_t team) -> godot::String {
    const auto next = engine_.toggle_partner_tactic(
        static_cast<std::uint8_t>(std::clamp<std::int64_t>(team, 0, 1)));
    return next == DefenseTactic::BlockNet ? to_gd_str("block")
                                           : to_gd_str("peel");
  }

  void step_frame(double dt, const godot::Dictionary& p0_input) {
    std::array<PlayerInput, 4> inputs{};
    inputs.at(0).mx = static_cast<double>(p0_input.get("mx", 0.0));
    inputs.at(0).my = static_cast<double>(p0_input.get("my", 0.0));
    inputs.at(0).btn_a = static_cast<bool>(p0_input.get("btn_a", false));
    inputs.at(0).btn_b = static_cast<bool>(p0_input.get("btn_b", false));
    inputs.at(0).hold_a = static_cast<bool>(p0_input.get("hold_a", false));
    inputs.at(0).hold_b = static_cast<bool>(p0_input.get("hold_b", false));
    inputs.at(0).btn_switch =
        static_cast<bool>(p0_input.get("btn_switch", false));
    inputs.at(0).turbo = static_cast<bool>(p0_input.get("turbo", false));
    engine_.step(std::clamp(dt, 0.001, 0.05), inputs);
  }

  [[nodiscard]] auto get_snapshot() const -> godot::Dictionary {
    godot::Dictionary d;
    d["score_left"] = engine_.score().at(0);
    d["score_right"] = engine_.score().at(1);
    d["streak_left"] = engine_.streak().at(0);
    d["streak_right"] = engine_.streak().at(1);
    d["on_fire_left"] = engine_.team_on_fire().at(0);
    d["on_fire_right"] = engine_.team_on_fire().at(1);
    d["solar_left"] = engine_.solar_meter().at(0);
    d["solar_right"] = engine_.solar_meter().at(1);
    d["tactic_left"] = engine_.defense_tactic().at(0) == DefenseTactic::BlockNet
                           ? to_gd_str("block")
                           : to_gd_str("peel");
    d["tactic_right"] =
        engine_.defense_tactic().at(1) == DefenseTactic::BlockNet
            ? to_gd_str("block")
            : to_gd_str("peel");
    d["is_deuce"] = engine_.is_deuce();
    d["match_point_team"] = engine_.match_point_team();
    d["wind_knots"] = engine_.wind_knots();
    d["wind_x"] = engine_.wind_x();
    d["wind_label"] = to_gd_str(engine_.wind_label());
    d["rally_state"] = static_cast<int>(engine_.rally_state());
    d["serve_phase"] = static_cast<int>(engine_.serve_phase());
    d["serve_charge"] = engine_.serve_charge();
    d["serve_is_jump"] = engine_.serve_is_jump();
    d["serving_team"] = static_cast<int>(engine_.serving_team());
    d["server_slot"] = static_cast<int>(engine_.server_slot());
    d["touch_team"] = static_cast<int>(engine_.touch_team());
    d["touch_count"] = engine_.touch_count();
    d["lighting_theme"] = static_cast<int>(engine_.lighting_theme());
    d["physics_preset"] = static_cast<int>(engine_.physics_preset());
    d["pose_cam_active"] = engine_.pose_cam_active();
    d["cut_in_timer"] = engine_.cut_in_timer();
    d["cut_in_label"] = to_gd_str(engine_.cut_in_label());

    const auto& b = engine_.ball();
    godot::Dictionary bd;
    bd["x"] = b.x;
    bd["y"] = b.y;
    bd["z"] = b.z;
    bd["vx"] = b.vx;
    bd["vy"] = b.vy;
    bd["vz"] = b.vz;
    bd["spin_angle"] = b.spin_angle;
    bd["impact_squash"] = b.impact_squash;
    bd["solar_spike"] = b.solar_spike;
    bd["on_fire_ball"] = b.on_fire_ball;
    bd["landing"] = to_gd_vec2(engine_.ball_landing());
    godot::Array b_trail;
    for (const auto& pt : b.trail) {
      b_trail.push_back(to_gd_vec3(pt));
    }
    bd["trail"] = b_trail;
    d["ball"] = bd;

    godot::Array p_arr;
    for (const auto& p : engine_.players()) {
      const auto& prof = kRoster.at(p.roster_index % kRoster.size());
      const auto& lm = pose_landmarks(p.active_pose);
      godot::Dictionary pd;
      pd["slot"] = static_cast<int>(p.slot);
      pd["team"] = static_cast<int>(p.team);
      pd["roster_index"] = static_cast<int>(p.roster_index);
      pd["name"] = to_gd_str(prof.name);
      pd["title"] = to_gd_str(prof.title);
      pd["signature_move"] = to_gd_str(prof.signature_move);
      pd["x"] = p.x;
      pd["y"] = p.y;
      pd["z"] = p.z;
      pd["vx"] = p.vx;
      pd["vy"] = p.vy;
      pd["vz"] = p.vz;
      pd["facing"] = p.facing;
      pd["turbo"] = p.turbo;
      pd["on_fire"] = p.on_fire;
      pd["is_partner_target"] = p.is_partner_target;
      pd["action_state"] = to_gd_str(action_state_string(p.action_state));
      pd["pose_key"] = to_gd_str(pose_key_string(p.active_pose));
      pd["prev_pose_key"] = to_gd_str(pose_key_string(p.prev_pose));
      pd["pose_blend"] = p.pose_blend;
      pd["tex_w"] = lm.width;
      pd["tex_h"] = lm.height;
      pd["height_scale"] = prof.height_scale;
      pd["bust_scale"] = prof.bust_scale;
      pd["hip_scale"] = prof.hip_scale;
      pd["suit_primary"] = to_gd_vec3(prof.suit_primary);
      pd["suit_trim"] = to_gd_vec3(prof.suit_trim);
      pd["hair_color"] = to_gd_vec3(prof.hair_color);
      pd["skin_base"] = to_gd_vec3(prof.skin_base);
      pd["wetness"] = p.rig.wetness;
      pd["sand_coat"] = p.rig.sand_coat;
      pd["disp_bust_l"] = godot::Vector2(static_cast<real_t>(p.rig.bust_l.x),
                                         static_cast<real_t>(p.rig.bust_l.y));
      pd["disp_bust_r"] = godot::Vector2(static_cast<real_t>(p.rig.bust_r.x),
                                         static_cast<real_t>(p.rig.bust_r.y));
      pd["disp_hip_l"] = godot::Vector2(static_cast<real_t>(p.rig.hip_l.x),
                                        static_cast<real_t>(p.rig.hip_l.y));
      pd["disp_hip_r"] = godot::Vector2(static_cast<real_t>(p.rig.hip_r.x),
                                        static_cast<real_t>(p.rig.hip_r.y));
      pd["disp_hair"] = godot::Vector2(static_cast<real_t>(p.rig.hair_tip.x),
                                       static_cast<real_t>(p.rig.hair_tip.y));
      pd["zone_bust_l"] = to_gd_zone(lm.bust_l);
      pd["zone_bust_r"] = to_gd_zone(lm.bust_r);
      pd["zone_hip_l"] = to_gd_zone(lm.hip_l);
      pd["zone_hip_r"] = to_gd_zone(lm.hip_r);
      pd["zone_hair"] = to_gd_zone(lm.hair);

      godot::Dictionary sk;
      sk["root_offset"] = to_gd_vec2(p.skeleton.root_offset);
      sk["hip_deg"] = p.skeleton.hip_deg;
      sk["torso_deg"] = p.skeleton.torso_deg;
      sk["head_deg"] = p.skeleton.head_deg;
      sk["arm_deg"] = p.skeleton.arm_deg;
      sk["forearm_deg"] = p.skeleton.forearm_deg;
      sk["thigh_deg"] = p.skeleton.thigh_deg;
      sk["shin_deg"] = p.skeleton.shin_deg;
      sk["foot_deg"] = p.skeleton.foot_deg;
      sk["squash_x"] = p.skeleton.squash_x;
      sk["stretch_y"] = p.skeleton.stretch_y;
      sk["shear_x"] = p.skeleton.shear_x;
      sk["hand_socket_world"] = to_gd_vec3(p.skeleton.hand_socket_world);
      sk["hand_socket_screen"] = to_gd_vec2(p.skeleton.hand_socket_screen);
      pd["skeleton"] = sk;

      p_arr.push_back(pd);
    }
    d["players"] = p_arr;

    godot::Array callout_arr;
    for (const auto& c : engine_.callouts()) {
      godot::Dictionary cd;
      cd["text"] = to_gd_str(c.text);
      cd["x"] = c.x;
      cd["y"] = c.y;
      cd["color"] = to_gd_str(c.color);
      cd["age"] = c.age;
      callout_arr.push_back(cd);
    }
    d["callouts"] = callout_arr;

    return d;
  }

 protected:
  static void _bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("reset_match"),
                                &SunbreakMatch::reset_match);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_preset", "preset_name"),
                                &SunbreakMatch::apply_preset);
    godot::ClassDB::bind_method(
        godot::D_METHOD("trigger_jump", "slot", "jump_vel"),
        &SunbreakMatch::trigger_jump);
    godot::ClassDB::bind_method(
        godot::D_METHOD("start_serve_toss", "slot", "is_jump_serve"),
        &SunbreakMatch::start_serve_toss);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_serve_strike", "slot",
                                                "aim_y", "charge_override"),
                                &SunbreakMatch::execute_serve_strike);
    godot::ClassDB::bind_method(
        godot::D_METHOD("toggle_partner_tactic", "team"),
        &SunbreakMatch::toggle_partner_tactic);
    godot::ClassDB::bind_method(godot::D_METHOD("step_frame", "dt", "p0_input"),
                                &SunbreakMatch::step_frame);
    godot::ClassDB::bind_method(godot::D_METHOD("get_snapshot"),
                                &SunbreakMatch::get_snapshot);
  }

 private:
  SunbreakEngine engine_{};
};

namespace {

void initialize_sunbreak_module(godot::ModuleInitializationLevel p_level) {
  if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
  godot::ClassDB::register_class<SunbreakMatch>();
}

void uninitialize_sunbreak_module(godot::ModuleInitializationLevel p_level) {
  if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
}

} // namespace

} // namespace sunbreak

extern "C" {
GDExtensionBool GDE_EXPORT
sunbreak_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                      GDExtensionClassLibraryPtr p_library,
                      GDExtensionInitialization* r_initialization) {
  godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library,
                                                 r_initialization);
  init_obj.register_initializer(sunbreak::initialize_sunbreak_module);
  init_obj.register_terminator(sunbreak::uninitialize_sunbreak_module);
  init_obj.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init_obj.init();
}
}
