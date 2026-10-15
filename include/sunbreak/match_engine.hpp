#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sunbreak {

struct Vec2 {
  double x{0.0};
  double y{0.0};
};

struct Vec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct ZoneUvRadius {
  double u{0.5};
  double v{0.5};
  double r{0.08};
};

enum class PoseId : std::uint8_t {
  Ready = 0,
  Dig = 1,
  Set = 2,
  JumpCoil = 3,
  Spike = 4,
  SpikeFollow = 5,
  Dive = 6,
  Closeup = 7
};

enum class ActionState : std::uint8_t {
  Idle = 0,
  Dig = 1,
  Set = 2,
  JumpCoil = 3,
  JumpLaunch = 4,
  Spike = 5,
  SpikeFollow = 6,
  Dive = 7
};

enum class RallyState : std::uint8_t { Serve = 0, Play = 1, PointPause = 2 };

enum class ServePhase : std::uint8_t { Ready = 0, Toss = 1 };

enum class DefenseTactic : std::uint8_t { BlockNet = 0, PeelDeep = 1 };

enum class PhysicsPreset : std::uint8_t {
  Realistic = 0,
  Doax = 1,
  Overdrive = 2
};

enum class LightingTheme : std::uint8_t { Sunset = 0, Noon = 1, Neon = 2 };

struct AthleteProfile {
  std::string_view id;
  std::string_view name;
  std::string_view title;
  std::string_view signature_move;
  double power{0.80};
  double speed{0.80};
  double jump{0.80};
  double control{0.80};
  double height_scale{1.0};
  double bust_scale{1.08};
  double hip_scale{1.08};
  double spring_freq_hz{3.5};
  double damping_ratio{0.24};
  Vec3 suit_primary{};
  Vec3 suit_trim{};
  Vec3 hair_color{};
  Vec3 skin_base{};
};

struct PoseLandmarks {
  PoseId id{PoseId::Ready};
  std::string_view key;
  double width{384.0};
  double height{512.0};
  double crouch{0.0};
  Vec2 head{};
  Vec2 chest{};
  Vec2 hand_socket{};
  Vec2 hips{};
  Vec2 knees{};
  Vec2 feet{};
  ZoneUvRadius bust_l{};
  ZoneUvRadius bust_r{};
  ZoneUvRadius hip_l{};
  ZoneUvRadius hip_r{};
  ZoneUvRadius hair{};
};

struct SpringNode2D {
  double x{0.0};
  double y{0.0};
  double vx{0.0};
  double vy{0.0};
};

struct SoftBodyState {
  SpringNode2D bust_l{};
  SpringNode2D bust_r{};
  SpringNode2D hip_l{};
  SpringNode2D hip_r{};
  SpringNode2D hair_tip{};
  double wetness{0.18};
  double sand_coat{0.0};
  double stride_phase{0.0};
};

/**
 * Keyframed 2D Bone Pose evaluated by C++23 for Godot's `Skeleton2D` +
 * `Polygon2D` deform mesh. Drives 100%-opaque bone rotation & translation
 * without alpha crossfading.
 */
struct SkeletonPose2D {
  Vec2 root_offset{};
  double hip_deg{0.0};
  double torso_deg{0.0};
  double head_deg{0.0};
  double arm_deg{0.0};
  double forearm_deg{0.0};
  double thigh_deg{0.0};
  double shin_deg{0.0};
  double foot_deg{0.0};
  double squash_x{1.0};
  double stretch_y{1.0};
  double shear_x{0.0};
  Vec3 hand_socket_world{};
  Vec2 hand_socket_screen{};
};

struct PlayerInput {
  double mx{0.0};
  double my{0.0};
  bool btn_a{false};
  bool btn_b{false};
  bool btn_dive{false};
  bool hold_a{false};
  bool hold_b{false};
  bool btn_switch{false};
  bool turbo{false};
};

struct PlayerState {
  std::uint8_t slot{0};
  std::uint8_t team{0};
  std::uint8_t roster_index{0};
  bool is_ai{false};
  bool is_partner_target{false};
  double x{280.0};
  double y{480.0};
  double z{0.0};
  double vx{0.0};
  double vy{0.0};
  double vz{0.0};
  double facing{1.0};
  ActionState action_state{ActionState::Idle};
  PoseId active_pose{PoseId::Ready};
  PoseId prev_pose{PoseId::Ready};
  double pose_blend{1.0};
  double action_timer{0.0};
  double jump_coil_timer{0.0};
  double jump_launch_timer{0.0};
  double jump_cooldown{0.0};
  double pending_jump_impulse{0.0};
  double dive_lock_timer{0.0};
  double active_dig_timer{0.0};
  double turbo{100.0};
  bool turbo_active{false};
  bool on_fire{false};
  SoftBodyState rig{};
  SkeletonPose2D skeleton{};
};

struct BallState {
  double x{280.0};
  double y{480.0};
  double z{42.0};
  double vx{0.0};
  double vy{0.0};
  double vz{0.0};
  double spin_angle{0.0};
  double impact_squash{0.0};
  double topspin{0.0};
  double curve_y{0.0};
  double knuckle{0.0};
  double knuckle_phase{0.0};
  bool solar_spike{false};
  bool on_fire_ball{false};
  std::vector<Vec3> trail{};
};

struct SandCrater {
  double x{0.0};
  double y{0.0};
  double radius{18.0};
  double age{0.0};
};

struct CalloutBanner {
  std::string text{};
  double x{640.0};
  double y{260.0};
  std::string color{"#ffb84d"};
  double age{0.0};
};

inline constexpr double kCourtLeftX = 180.0;
inline constexpr double kCourtRightX = 1100.0;
inline constexpr double kCourtNetX = 640.0;
inline constexpr double kCourtTopY = 310.0;
inline constexpr double kCourtBotY = 650.0;
inline constexpr double kCourtNetHeight = 68.0;
inline constexpr double kCourtGravity = 470.0;

inline constexpr std::array<AthleteProfile, 6> kRoster{{
    {"kasumi",
     "KASUMI",
     "Crimson Cyclone",
     "Solar Overhead Spike",
     0.88,
     0.92,
     0.95,
     0.84,
     1.00,
     1.12,
     1.10,
     3.4,
     0.22,
     {0.96, 0.20, 0.42},
     {1.00, 0.82, 0.28},
     {0.62, 0.22, 0.24},
     {0.98, 0.78, 0.67}},
    {"elena",
     "ELENA",
     "Solar Valkyrie",
     "Valkyrie Sky Block",
     0.94,
     0.82,
     0.90,
     0.88,
     1.05,
     1.18,
     1.14,
     3.1,
     0.20,
     {0.15, 0.72, 0.96},
     {1.00, 0.96, 0.88},
     {0.96, 0.82, 0.46},
     {0.99, 0.82, 0.72}},
    {"meilin",
     "MEI-LIN",
     "Jade Tempest",
     "Mirage Cut-Shot",
     0.78,
     0.98,
     0.88,
     0.96,
     0.96,
     1.04,
     1.08,
     3.8,
     0.25,
     {0.12, 0.84, 0.56},
     {0.98, 0.84, 0.32},
     {0.14, 0.16, 0.24},
     {0.97, 0.80, 0.69}},
    {"ayane",
     "AYANE",
     "Midnight Mirage",
     "Butterfly Spin Serve",
     0.85,
     0.94,
     0.92,
     0.90,
     0.98,
     1.10,
     1.10,
     3.5,
     0.23,
     {0.64, 0.26, 0.92},
     {0.24, 0.92, 0.98},
     {0.48, 0.22, 0.68},
     {0.98, 0.81, 0.74}},
    {"nyx",
     "NYX",
     "Neon Viper",
     "Eclipse Sand Dive",
     0.92,
     0.90,
     0.86,
     0.85,
     1.02,
     1.15,
     1.16,
     3.2,
     0.21,
     {0.18, 0.16, 0.26},
     {1.00, 0.28, 0.62},
     {0.88, 0.22, 0.48},
     {0.84, 0.60, 0.48}},
    {"freya",
     "FREYA",
     "Valkyrie Frost",
     "Northern Lights Set",
     0.96,
     0.80,
     0.88,
     0.92,
     1.06,
     1.20,
     1.15,
     3.0,
     0.19,
     {0.94, 0.96, 1.00},
     {0.20, 0.68, 0.98},
     {0.92, 0.94, 0.98},
     {0.99, 0.84, 0.76}},
}};

[[nodiscard]] auto pose_landmarks(PoseId id) -> const PoseLandmarks&;
[[nodiscard]] auto pose_key_string(PoseId id) -> std::string_view;
[[nodiscard]] auto action_state_string(ActionState st) -> std::string_view;

class SunbreakEngine {
 public:
  SunbreakEngine();

  void reset_match();
  void reset_serve(std::uint8_t serving_team);
  void roll_beach_wind();
  void apply_preset(std::string_view preset_name);

  void start_serve_toss(std::uint8_t slot, bool is_jump_serve);
  void execute_serve_strike(std::uint8_t slot, const PlayerInput& input,
                            double charge_override = -1.0);
  void trigger_keyed_jump(std::uint8_t slot, double jump_velocity);
  auto toggle_partner_tactic(std::uint8_t team) -> DefenseTactic;

  void step(double dt, const std::array<PlayerInput, 4>& raw_inputs);

  [[nodiscard]] auto is_deuce() const -> bool;
  [[nodiscard]] auto match_point_team() const -> int;
  [[nodiscard]] auto hand_socket_world(const PlayerState& p,
                                       PoseId pose_override) const -> Vec3;
  [[nodiscard]] auto evaluate_skeleton_pose(const PlayerState& p) const
      -> SkeletonPose2D;

  [[nodiscard]] auto score() const -> const std::array<int, 2>& {
    return score_;
  }
  [[nodiscard]] auto streak() const -> const std::array<int, 2>& {
    return streak_;
  }
  [[nodiscard]] auto team_on_fire() const -> const std::array<bool, 2>& {
    return team_on_fire_;
  }
  [[nodiscard]] auto solar_meter() const -> const std::array<double, 2>& {
    return solar_meter_;
  }
  [[nodiscard]] auto defense_tactic() const
      -> const std::array<DefenseTactic, 2>& {
    return defense_tactic_;
  }
  [[nodiscard]] auto rally_state() const -> RallyState { return rally_state_; }
  [[nodiscard]] auto serve_phase() const -> ServePhase { return serve_phase_; }
  [[nodiscard]] auto serve_charge() const -> double { return serve_charge_; }
  [[nodiscard]] auto serve_is_jump() const -> bool { return serve_is_jump_; }
  [[nodiscard]] auto serving_team() const -> std::uint8_t {
    return serving_team_;
  }
  [[nodiscard]] auto server_slot() const -> std::uint8_t {
    return server_slot_;
  }
  [[nodiscard]] auto touch_team() const -> std::uint8_t { return touch_team_; }
  [[nodiscard]] auto touch_count() const -> int { return touch_count_; }
  [[nodiscard]] auto wind_knots() const -> int { return wind_knots_; }
  [[nodiscard]] auto wind_x() const -> double { return wind_x_; }
  [[nodiscard]] auto wind_label() const -> const std::string& {
    return wind_label_;
  }
  [[nodiscard]] auto players() const -> const std::array<PlayerState, 4>& {
    return players_;
  }
  [[nodiscard]] auto players_mut() -> std::array<PlayerState, 4>& {
    return players_;
  }
  [[nodiscard]] auto ball() const -> const BallState& { return ball_; }
  [[nodiscard]] auto ball_mut() -> BallState& { return ball_; }
  [[nodiscard]] auto ball_landing() const -> Vec2 { return ball_landing_; }
  [[nodiscard]] auto craters() const -> const std::vector<SandCrater>& {
    return craters_;
  }
  [[nodiscard]] auto callouts() const -> const std::vector<CalloutBanner>& {
    return callouts_;
  }
  [[nodiscard]] auto physics_preset() const -> PhysicsPreset {
    return physics_preset_;
  }
  [[nodiscard]] auto lighting_theme() const -> LightingTheme {
    return lighting_theme_;
  }
  [[nodiscard]] auto pose_cam_active() const -> bool {
    return pose_cam_active_;
  }
  [[nodiscard]] auto cut_in_timer() const -> double { return cut_in_timer_; }
  [[nodiscard]] auto cut_in_label() const -> const std::string& {
    return cut_in_label_;
  }
  [[nodiscard]] auto demo_freeze() const -> bool { return demo_freeze_; }

  void set_score(int left_score, int right_score);
  void set_wind(int knots, double wind_accel_x, std::string label);
  void set_physics_preset(PhysicsPreset preset) { physics_preset_ = preset; }
  void set_lighting_theme(LightingTheme theme) { lighting_theme_ = theme; }
  void set_pose_cam_active(bool active) { pose_cam_active_ = active; }

 private:
  void predict_ball_landing();
  void assign_closest_defender(std::uint8_t team);
  void apply_action_impulse(PlayerState& p, std::string_view action,
                            double intensity);
  void step_soft_body(PlayerState& p, double dt);
  void update_pre_contact_anticipation(PlayerState& p);
  void handle_ball_contact(PlayerState& p, const PlayerInput& input);
  void score_point(std::uint8_t winning_team, std::string_view reason);
  void add_crater(double x, double y, double radius);
  void add_callout(std::string text, double x, double y, std::string color);
  [[nodiscard]] auto compute_ai_input(const PlayerState& p) -> PlayerInput;
  [[nodiscard]] auto select_pose_id(const PlayerState& p) const -> PoseId;

  std::array<int, 2> score_{0, 0};
  std::array<int, 2> streak_{0, 0};
  std::array<bool, 2> team_on_fire_{false, false};
  std::array<double, 2> solar_meter_{0.0, 0.0};
  std::array<DefenseTactic, 2> defense_tactic_{DefenseTactic::BlockNet,
                                               DefenseTactic::PeelDeep};
  std::array<double, 2> tactic_flash_timer_{0.0, 0.0};

  RallyState rally_state_{RallyState::Serve};
  ServePhase serve_phase_{ServePhase::Ready};
  bool serve_is_jump_{false};
  double serve_charge_{0.0};
  double ai_serve_target_{0.82};
  std::uint8_t serving_team_{0};
  std::uint8_t server_slot_{0};
  std::uint8_t touch_team_{0};
  int touch_count_{0};
  int last_touch_slot_{-1};
  double point_pause_timer_{0.0};

  int wind_knots_{9};
  double wind_x_{37.8};
  std::string wind_label_{"9 KT -> E"};
  std::uint32_t rng_state_{0x53554E42U};

  PhysicsPreset physics_preset_{PhysicsPreset::Doax};
  LightingTheme lighting_theme_{LightingTheme::Sunset};
  bool pose_cam_active_{true};
  double cut_in_timer_{0.0};
  std::string cut_in_label_{};
  bool demo_freeze_{false};

  std::array<PlayerState, 4> players_{};
  BallState ball_{};
  Vec2 ball_landing_{640.0, 480.0};
  std::vector<SandCrater> craters_{};
  std::vector<CalloutBanner> callouts_{};
};

} // namespace sunbreak
