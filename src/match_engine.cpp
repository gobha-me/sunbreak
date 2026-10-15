#include "sunbreak/match_engine.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace sunbreak {

namespace {

constexpr std::array<PoseLandmarks, 8> kPoseTable{{
    {PoseId::Ready,
     "pose_ready",
     384.0,
     512.0,
     0.0,
     {0.52, 0.22},
     {0.53, 0.37},
     {0.66, 0.48},
     {0.48, 0.56},
     {0.52, 0.75},
     {0.50, 0.90},
     {0.50, 0.37, 0.068},
     {0.57, 0.37, 0.068},
     {0.46, 0.56, 0.075},
     {0.53, 0.56, 0.075},
     {0.34, 0.22, 0.14}},
    {PoseId::Dig,
     "pose_dig",
     384.0,
     512.0,
     0.22,
     {0.56, 0.26},
     {0.52, 0.41},
     {0.78, 0.46},
     {0.38, 0.57},
     {0.54, 0.74},
     {0.51, 0.90},
     {0.49, 0.41, 0.068},
     {0.56, 0.41, 0.068},
     {0.36, 0.57, 0.078},
     {0.44, 0.57, 0.078},
     {0.31, 0.24, 0.15}},
    {PoseId::Set,
     "pose_set",
     384.0,
     512.0,
     -0.06,
     {0.50, 0.28},
     {0.50, 0.43},
     {0.62, 0.15},
     {0.47, 0.59},
     {0.51, 0.76},
     {0.49, 0.90},
     {0.47, 0.43, 0.068},
     {0.54, 0.43, 0.068},
     {0.45, 0.59, 0.075},
     {0.52, 0.59, 0.075},
     {0.34, 0.28, 0.14}},
    {PoseId::JumpCoil,
     "pose_jump_coil",
     384.0,
     512.0,
     0.38,
     {0.62, 0.26},
     {0.55, 0.42},
     {0.30, 0.50},
     {0.38, 0.58},
     {0.57, 0.73},
     {0.52, 0.89},
     {0.52, 0.42, 0.068},
     {0.59, 0.42, 0.068},
     {0.35, 0.58, 0.078},
     {0.43, 0.58, 0.078},
     {0.37, 0.22, 0.15}},
    {PoseId::Spike,
     "pose_spike",
     384.0,
     512.0,
     -0.14,
     {0.53, 0.26},
     {0.55, 0.40},
     {0.73, 0.22},
     {0.48, 0.56},
     {0.38, 0.73},
     {0.33, 0.86},
     {0.51, 0.40, 0.068},
     {0.59, 0.40, 0.068},
     {0.45, 0.56, 0.076},
     {0.53, 0.56, 0.076},
     {0.34, 0.22, 0.16}},
    {PoseId::SpikeFollow,
     "pose_spike_follow",
     384.0,
     512.0,
     -0.05,
     {0.63, 0.27},
     {0.57, 0.41},
     {0.81, 0.52},
     {0.43, 0.55},
     {0.48, 0.73},
     {0.44, 0.87},
     {0.54, 0.41, 0.068},
     {0.61, 0.41, 0.068},
     {0.40, 0.55, 0.076},
     {0.48, 0.55, 0.076},
     {0.38, 0.21, 0.16}},
    {PoseId::Dive,
     "pose_dive",
     512.0,
     384.0,
     0.18,
     {0.66, 0.44},
     {0.56, 0.53},
     {0.87, 0.53},
     {0.39, 0.51},
     {0.23, 0.49},
     {0.12, 0.46},
     {0.54, 0.53, 0.065},
     {0.60, 0.53, 0.065},
     {0.37, 0.51, 0.075},
     {0.43, 0.51, 0.075},
     {0.48, 0.36, 0.15}},
    {PoseId::Closeup,
     "plate_closeup",
     512.0,
     512.0,
     0.0,
     {0.51, 0.23},
     {0.56, 0.48},
     {0.67, 0.67},
     {0.45, 0.77},
     {0.47, 0.90},
     {0.47, 0.97},
     {0.51, 0.48, 0.072},
     {0.62, 0.48, 0.072},
     {0.40, 0.77, 0.088},
     {0.52, 0.77, 0.088},
     {0.31, 0.25, 0.16}},
}};

void step_spring(SpringNode2D& node, double ext_ax, double ext_ay, double omega,
                 double zeta, double dt, double max_disp) {
  const double ax =
      ext_ax - (omega * omega) * node.x - (2.0 * zeta * omega) * node.vx;
  const double ay =
      ext_ay - (omega * omega) * node.y - (2.0 * zeta * omega) * node.vy;
  node.vx += ax * dt;
  node.vy += ay * dt;
  node.x += node.vx * dt;
  node.y += node.vy * dt;
  const double mag = std::hypot(node.x, node.y);
  if (mag > max_disp && mag > 1e-6) {
    const double s = max_disp / mag;
    node.x *= s;
    node.y *= s;
    node.vx *= 0.65;
    node.vy *= 0.65;
  }
}

[[nodiscard]] auto smoothstep01(double t) -> double {
  const double c = std::clamp(t, 0.0, 1.0);
  return c * c * (3.0 - 2.0 * c);
}

} // namespace

auto pose_landmarks(PoseId id) -> const PoseLandmarks& {
  const auto idx = static_cast<std::size_t>(id);
  return kPoseTable.at(idx < kPoseTable.size() ? idx : 0U);
}

auto pose_key_string(PoseId id) -> std::string_view {
  return pose_landmarks(id).key;
}

auto action_state_string(ActionState st) -> std::string_view {
  switch (st) {
    case ActionState::Idle: return "idle";
    case ActionState::Dig: return "dig";
    case ActionState::Set: return "set";
    case ActionState::JumpCoil: return "jump_coil";
    case ActionState::JumpLaunch: return "jump_launch";
    case ActionState::Spike: return "spike";
    case ActionState::SpikeFollow: return "spike_follow";
    case ActionState::Dive: return "dive";
  }
  return "idle";
}

SunbreakEngine::SunbreakEngine() {
  reset_match();
}

void SunbreakEngine::reset_match() {
  score_ = {0, 0};
  streak_ = {0, 0};
  team_on_fire_ = {false, false};
  solar_meter_ = {0.0, 0.0};
  defense_tactic_ = {DefenseTactic::BlockNet, DefenseTactic::PeelDeep};
  tactic_flash_timer_ = {0.0, 0.0};
  demo_freeze_ = false;
  cut_in_timer_ = 0.0;
  cut_in_label_.clear();
  craters_.clear();
  callouts_.clear();

  constexpr std::array<std::uint8_t, 4> kDefaultRoster{0, 1, 4, 5};
  for (std::uint8_t i = 0; i < 4; ++i) {
    auto& p = players_.at(i);
    p = PlayerState{};
    p.slot = i;
    p.team = i < 2 ? 0 : 1;
    p.roster_index = kDefaultRoster.at(i);
    p.is_ai = (i != 0);
    p.is_partner_target = (i == 1);
    p.facing = p.team == 0 ? 1.0 : -1.0;
  }

  reset_serve(0);
}

void SunbreakEngine::roll_beach_wind() {
  rng_state_ = rng_state_ * 1664525U + 1013904223U;
  const std::uint32_t idx = (rng_state_ >> 16U) % 6U;
  switch (idx) {
    case 0: set_wind(0, 0.0, "CALM 0 KT"); break;
    case 1: set_wind(7, 29.4, "7 KT -> E"); break;
    case 2: set_wind(9, -37.8, "9 KT <- W"); break;
    case 3: set_wind(12, 50.4, "12 KT -> E"); break;
    case 4: set_wind(14, -58.8, "14 KT <- W"); break;
    default: set_wind(17, 71.4, "GUST 17 KT -> E"); break;
  }
}

void SunbreakEngine::set_score(int left_score, int right_score) {
  score_ = {left_score, right_score};
}

void SunbreakEngine::set_wind(int knots, double wind_accel_x,
                              std::string label) {
  wind_knots_ = knots;
  wind_x_ = wind_accel_x;
  wind_label_ = std::move(label);
}

void SunbreakEngine::reset_serve(std::uint8_t serving_team) {
  demo_freeze_ = false;
  rally_state_ = RallyState::Serve;
  serve_phase_ = ServePhase::Ready;
  serve_is_jump_ = false;
  serve_charge_ = 0.0;
  ai_serve_target_ = 0.80;
  serving_team_ = serving_team;
  touch_team_ = serving_team;
  touch_count_ = 0;
  last_touch_slot_ = -1;
  server_slot_ = serving_team == 0 ? 0 : 2;

  roll_beach_wind();

  constexpr std::array<Vec2, 4> kServeCoords{{
      {240.0, 500.0},
      {470.0, 430.0},
      {1040.0, 460.0},
      {810.0, 520.0},
  }};

  for (std::size_t i = 0; i < 4; ++i) {
    auto& p = players_.at(i);
    p.x = kServeCoords.at(i).x;
    p.y = kServeCoords.at(i).y;
    p.z = 0.0;
    p.vx = 0.0;
    p.vy = 0.0;
    p.vz = 0.0;
    p.action_state = ActionState::Idle;
    p.active_pose = PoseId::Ready;
    p.prev_pose = PoseId::Ready;
    p.pose_blend = 1.0;
    p.action_timer = 0.0;
    p.jump_coil_timer = 0.0;
    p.jump_launch_timer = 0.0;
    p.jump_cooldown = 0.0;
    p.pending_jump_impulse = 0.0;
    p.dive_lock_timer = 0.0;
    p.active_dig_timer = 0.0;
    p.facing = p.team == 0 ? 1.0 : -1.0;
    p.on_fire = team_on_fire_.at(p.team);
    p.is_ai = (i != 0);
    p.is_partner_target = (i == 1);
    p.skeleton = evaluate_skeleton_pose(p);
  }

  const auto& srv = players_.at(server_slot_);
  const Vec3 hs = hand_socket_world(srv, PoseId::Ready);
  ball_ = BallState{};
  ball_.x = hs.x + srv.facing * 10.0;
  ball_.y = hs.y;
  ball_.z = std::max(22.0, hs.z - 6.0);
  ball_landing_ = {kCourtNetX, 480.0};
}

auto SunbreakEngine::is_deuce() const -> bool {
  return score_.at(0) >= 14 && score_.at(1) >= 14 &&
         std::abs(score_.at(0) - score_.at(1)) <= 1;
}

auto SunbreakEngine::match_point_team() const -> int {
  if (score_.at(0) >= 14 && score_.at(0) - score_.at(1) >= 1) {
    return 0;
  }
  if (score_.at(1) >= 14 && score_.at(1) - score_.at(0) >= 1) {
    return 1;
  }
  return -1;
}

auto SunbreakEngine::select_pose_id(const PlayerState& p) const -> PoseId {
  if (p.action_state == ActionState::Dive || p.dive_lock_timer > 0.0) {
    return PoseId::Dive;
  }
  if (p.action_state == ActionState::SpikeFollow) {
    return PoseId::SpikeFollow;
  }
  if (p.action_state == ActionState::JumpCoil) {
    return PoseId::JumpCoil;
  }
  if (p.action_state == ActionState::JumpLaunch) {
    return PoseId::Set;
  }
  if (p.z > 14.0 || p.action_state == ActionState::Spike) {
    return PoseId::Spike;
  }
  if (p.action_state == ActionState::Set) {
    return PoseId::Set;
  }
  if (p.action_state == ActionState::Dig) {
    return PoseId::Dig;
  }
  return PoseId::Ready;
}

auto SunbreakEngine::hand_socket_world(const PlayerState& p,
                                       PoseId pose_override) const -> Vec3 {
  const auto& lm = pose_landmarks(pose_override);
  const auto& prof = kRoster.at(p.roster_index % kRoster.size());
  const double depth_scale = 0.90 + ((p.y - 300.0) / 360.0) * 0.22;
  const double draw_h = 152.0 * depth_scale * prof.height_scale *
                        (pose_override == PoseId::Dive ? 0.74 : 1.0);
  const double draw_w = draw_h * (lm.width / lm.height);
  const double local_dx = (lm.hand_socket.x - 0.5) * draw_w * p.facing;
  const double height_above_feet_px = (0.93 - lm.hand_socket.y) * draw_h;

  return {p.x + local_dx, p.y, p.z + height_above_feet_px / 0.88};
}

auto SunbreakEngine::evaluate_skeleton_pose(const PlayerState& p) const
    -> SkeletonPose2D {
  SkeletonPose2D out{};
  const PoseId curr_id = p.active_pose;
  const PoseId prev_id = p.prev_pose;
  const auto& lm_curr = pose_landmarks(curr_id);
  const auto& lm_prev = pose_landmarks(prev_id);

  const double t = smoothstep01(p.pose_blend);
  const double pulse = std::sin(t * std::numbers::pi);
  const double crouch_mix = std::lerp(lm_prev.crouch, lm_curr.crouch, t);

  // Foot-anchored vertical crouch offset (positive = downward into squat)
  out.root_offset.y = crouch_mix * 24.0;

  // Running stride bob & bone articulation when on sand in Ready stance
  const double speed_xy = std::hypot(p.vx, p.vy);
  if (curr_id == PoseId::Ready && p.z <= 1.0 && speed_xy > 15.0) {
    const double stride_f = std::min(1.0, speed_xy / 235.0);
    out.root_offset.y -=
        std::abs(std::sin(p.rig.stride_phase)) * 6.5 * stride_f;
    out.thigh_deg = std::sin(p.rig.stride_phase) * 18.0 * stride_f * p.facing;
    out.shin_deg = std::max(0.0, std::cos(p.rig.stride_phase)) * 22.0 *
                   stride_f * p.facing;
    out.arm_deg = -std::sin(p.rig.stride_phase) * 15.0 * stride_f * p.facing;
    out.torso_deg = std::sin(p.rig.stride_phase) * 3.5 * stride_f * p.facing;
  }

  // Keyframed 4-Stage Jump & Action Bone Articulation (Stand -> Squat ->
  // Arms-Up -> Bow-Arch -> Whip)
  switch (curr_id) {
    case PoseId::JumpCoil:
      out.hip_deg = 14.0 * p.facing;
      out.torso_deg = 16.0 * p.facing;
      out.thigh_deg = -26.0 * p.facing;
      out.shin_deg = 32.0 * p.facing;
      out.arm_deg = -24.0 * p.facing;
      out.squash_x = 1.0 + 0.10 * (1.0 - t * 0.35);
      out.stretch_y = 1.0 - 0.10 * (1.0 - t * 0.35);
      out.shear_x = 0.06 * pulse * p.facing;
      break;
    case PoseId::Set:
      if (p.action_state == ActionState::JumpLaunch) {
        out.hip_deg = -6.0 * p.facing;
        out.torso_deg = -8.0 * p.facing;
        out.arm_deg = 28.0 * p.facing;
        out.forearm_deg = 18.0 * p.facing;
        out.thigh_deg = 10.0 * p.facing;
        out.shin_deg = -12.0 * p.facing;
        out.squash_x = 1.0 - 0.06 * pulse;
        out.stretch_y = 1.0 + 0.09 * (0.4 + 0.6 * t);
        out.shear_x = 0.05 * pulse * p.facing;
      } else {
        out.torso_deg = -5.0 * p.facing;
        out.arm_deg = 18.0 * p.facing;
        out.squash_x = 1.0 + 0.05 * pulse;
        out.stretch_y = 1.0 - 0.04 * pulse;
      }
      break;
    case PoseId::Spike:
      out.hip_deg = -12.0 * p.facing;
      out.torso_deg =
          std::clamp(-p.vz * 0.04, -14.0, 16.0) * p.facing - 10.0 * p.facing;
      out.arm_deg = -22.0 * (1.0 - t) * p.facing;
      out.thigh_deg = -18.0 * p.facing;
      out.shin_deg = -24.0 * p.facing;
      out.squash_x = 1.0 - 0.06 * pulse;
      out.stretch_y = 1.0 + 0.09 * pulse;
      out.shear_x = -0.07 * pulse * p.facing;
      break;
    case PoseId::SpikeFollow:
      out.hip_deg = 10.0 * p.facing;
      out.torso_deg = 22.0 * p.facing;
      out.arm_deg = 34.0 * p.facing;
      out.forearm_deg = 26.0 * p.facing;
      out.squash_x = 1.0 + 0.07 * pulse;
      out.stretch_y = 1.0 - 0.07 * pulse;
      out.shear_x = 0.12 * pulse * p.facing;
      break;
    case PoseId::Dig:
      out.hip_deg = 10.0 * p.facing;
      out.torso_deg = 14.0 * p.facing;
      out.arm_deg = 12.0 * p.facing;
      out.thigh_deg = -16.0 * p.facing;
      out.shin_deg = 18.0 * p.facing;
      out.squash_x = 1.0 + 0.06 * pulse;
      out.stretch_y = 1.0 - 0.05 * pulse;
      break;
    case PoseId::Dive:
      out.torso_deg = 18.0 * p.facing;
      out.arm_deg = 22.0 * p.facing;
      out.squash_x = 1.08;
      out.stretch_y = 0.94;
      break;
    default: break;
  }

  out.hand_socket_world = hand_socket_world(p, curr_id);
  out.hand_socket_screen = {out.hand_socket_world.x,
                            p.y - out.hand_socket_world.z * 0.88 + 8.0};
  return out;
}

void SunbreakEngine::apply_action_impulse(PlayerState& p,
                                          std::string_view action,
                                          double intensity) {
  auto& r = p.rig;
  if (action == "jump") {
    r.bust_l.vy += 155.0 * intensity;
    r.bust_r.vy += 165.0 * intensity;
    r.hip_l.vy += 120.0 * intensity;
    r.hip_r.vy += 125.0 * intensity;
    r.hair_tip.vy += 180.0 * intensity;
  } else if (action == "land") {
    r.bust_l.vy -= 185.0 * intensity;
    r.bust_r.vy -= 195.0 * intensity;
    r.hip_l.vy -= 150.0 * intensity;
    r.hip_r.vy -= 155.0 * intensity;
    r.hair_tip.vy -= 160.0 * intensity;
  } else if (action == "spike") {
    r.bust_l.vx += 135.0 * intensity * p.facing;
    r.bust_r.vx += 145.0 * intensity * p.facing;
    r.bust_l.vy -= 140.0 * intensity;
    r.bust_r.vy -= 150.0 * intensity;
    r.hair_tip.vx -= 175.0 * intensity * p.facing;
  } else if (action == "dive") {
    r.bust_l.vx += 160.0 * intensity * p.facing;
    r.bust_r.vx += 160.0 * intensity * p.facing;
    r.hip_l.vy -= 130.0 * intensity;
    r.hip_r.vy -= 130.0 * intensity;
    r.sand_coat = std::min(1.0, r.sand_coat + 0.65);
  } else {
    r.bust_l.vy -= 115.0 * intensity;
    r.bust_r.vy -= 125.0 * intensity;
    r.hip_l.vy -= 95.0 * intensity;
    r.hip_r.vy -= 95.0 * intensity;
  }
}

void SunbreakEngine::step_soft_body(PlayerState& p, double dt) {
  const auto& prof = kRoster.at(p.roster_index % kRoster.size());
  double freq_mult = 1.0;
  double damp_mult = 1.0;
  double max_disp = 9.5;
  if (physics_preset_ == PhysicsPreset::Realistic) {
    freq_mult = 1.25;
    damp_mult = 1.55;
    max_disp = 5.5;
  } else if (physics_preset_ == PhysicsPreset::Overdrive) {
    freq_mult = 0.88;
    damp_mult = 0.68;
    max_disp = 13.5;
  }

  const double omega = prof.spring_freq_hz * freq_mult * 2.0 * std::numbers::pi;
  const double zeta = std::clamp(prof.damping_ratio * damp_mult, 0.08, 0.85);
  const double speed_xy = std::hypot(p.vx, p.vy);

  if (speed_xy > 12.0 && p.z <= 1.0) {
    p.rig.stride_phase += dt * (8.5 + speed_xy * 0.028);
  }

  const double stride_osc = (p.z <= 1.0 && speed_xy > 15.0)
                                ? std::sin(p.rig.stride_phase) * 42.0
                                : 0.0;
  const double ext_ax = -p.vx * 0.25;
  const double ext_ay = p.vz * 0.45 + stride_osc;

  step_spring(p.rig.bust_l, ext_ax, ext_ay, omega, zeta, dt, max_disp);
  step_spring(p.rig.bust_r, ext_ax * 1.04, ext_ay * 1.06, omega * 0.97, zeta,
              dt, max_disp);
  step_spring(p.rig.hip_l, ext_ax * 0.85, ext_ay * 0.88, omega * 1.08,
              zeta * 1.1, dt, max_disp * 0.85);
  step_spring(p.rig.hip_r, ext_ax * 0.88, ext_ay * 0.90, omega * 1.05,
              zeta * 1.1, dt, max_disp * 0.85);
  step_spring(p.rig.hair_tip, -p.vx * 0.45 + wind_x_ * 0.6, ext_ay * 0.9,
              omega * 0.82, zeta * 0.9, dt, max_disp * 1.35);

  if (speed_xy > 30.0 || p.z > 5.0) {
    p.rig.wetness = std::min(0.92, p.rig.wetness + dt * 0.014);
  }
  if (p.rig.sand_coat > 0.0 && speed_xy > 40.0) {
    p.rig.sand_coat = std::max(0.0, p.rig.sand_coat - dt * 0.06);
  }
}

void SunbreakEngine::start_serve_toss(std::uint8_t slot, bool is_jump_serve) {
  if (slot >= 4) {
    return;
  }
  auto& p = players_.at(slot);
  serve_phase_ = ServePhase::Toss;
  serve_is_jump_ = is_jump_serve;
  serve_charge_ = 0.0;

  ai_serve_target_ = p.on_fire ? 0.83 : 0.80;
  const Vec3 hs = hand_socket_world(p, PoseId::Set);
  ball_.x = hs.x + p.facing * 8.0;
  ball_.y = hs.y;
  ball_.z = hs.z + 4.0;
  ball_.vx = 0.0;
  ball_.vy = 0.0;
  ball_.vz = is_jump_serve ? 228.0 : 182.0;
  ball_.topspin = 0.0;
  ball_.curve_y = 0.0;
  ball_.knuckle = 0.0;

  p.action_state = ActionState::Set;
  p.action_timer = 0.28;
  apply_action_impulse(p, "set", 0.95);
}

void SunbreakEngine::execute_serve_strike(std::uint8_t slot,
                                          const PlayerInput& input,
                                          double charge_override) {
  if (slot >= 4) {
    return;
  }
  auto& p = players_.at(slot);
  const auto& prof = kRoster.at(p.roster_index % kRoster.size());
  const double charge = std::clamp(
      charge_override >= 0.0 ? charge_override : serve_charge_, 0.0, 1.0);

  rally_state_ = RallyState::Play;
  serve_phase_ = ServePhase::Ready;
  touch_team_ = p.team;
  touch_count_ = 1;
  last_touch_slot_ = static_cast<int>(p.slot);

  const double dir = p.team == 0 ? 1.0 : -1.0;
  const bool is_jump = serve_is_jump_ || p.z > 8.0;
  const bool is_sweet_spot = (charge >= 0.75 && charge <= 0.90);
  const bool is_overhit = (charge > 0.90);
  const bool is_short_drop = (charge < 0.45);

  const PoseId strike_pose = is_jump ? PoseId::SpikeFollow : PoseId::Set;
  const Vec3 hs = hand_socket_world(p, strike_pose);
  ball_.x = hs.x + dir * 10.0;
  ball_.y = hs.y;
  ball_.z = std::max(is_jump ? 54.0 : 34.0, hs.z);
  ball_.impact_squash = is_sweet_spot ? 0.95 : 0.65;
  ball_.topspin = 0.0;
  ball_.curve_y = 0.0;
  ball_.knuckle = 0.0;
  ball_.knuckle_phase = 0.0;
  ball_.solar_spike = false;
  ball_.on_fire_ball = false;

  const double aim_y = input.my * 125.0;

  if (p.on_fire && (is_sweet_spot || is_jump)) {
    ball_.vx = dir * (510.0 + prof.power * 75.0);
    ball_.vy = aim_y * 1.25;
    ball_.vz = 92.0;
    ball_.topspin = 1.65;
    ball_.curve_y = (input.my != 0.0 ? input.my : 0.7) * 145.0;
    ball_.solar_spike = true;
    ball_.on_fire_ball = true;
    cut_in_timer_ = 1.15;
    cut_in_label_ = "ON FIRE METEOR SERVE!";
    add_callout("ON FIRE METEOR SERVE!", p.x, p.y - 105.0, "#ff5500");
  } else if (is_jump) {
    if (is_sweet_spot) {
      ball_.vx = dir * (475.0 + prof.power * 65.0);
      ball_.vy = aim_y;
      ball_.vz = 112.0;
      ball_.topspin = 1.25;
      ball_.solar_spike = true;
      add_callout("SWEET SPOT! LASER SERVE!", p.x, p.y - 95.0, "#ff477e");
    } else if (is_overhit) {
      ball_.vx = dir * (525.0 + prof.power * 70.0);
      ball_.vy = aim_y * 1.3;
      ball_.vz = 175.0;
      ball_.topspin = 0.35;
      add_callout("OVERCHARGED! LONG!", p.x, p.y - 90.0, "#ffb84d");
    } else {
      ball_.vx = dir * (395.0 + charge * 75.0);
      ball_.vy = aim_y;
      ball_.vz = 152.0;
      ball_.topspin = 0.65;
      add_callout("JUMP TOPSPIN SERVE!", p.x, p.y - 88.0, "#ffb84d");
    }
  } else {
    if (is_sweet_spot) {
      ball_.vx = dir * (365.0 + prof.control * 45.0);
      ball_.vy = aim_y * 0.85;
      ball_.vz = 195.0;
      ball_.knuckle = 1.0;
      ball_.knuckle_phase = 0.4;
      add_callout("KNUCKLE FLOAT SERVE!", p.x, p.y - 92.0, "#2ce6ff");
    } else if (is_short_drop) {
      ball_.vx = dir * (275.0 + charge * 90.0);
      ball_.vy = aim_y * 0.9;
      ball_.vz = 188.0;
      ball_.topspin = 0.45;
      add_callout("SHORT DROP SERVE!", p.x, p.y - 88.0, "#34d399");
    } else {
      ball_.vx = dir * (340.0 + charge * 70.0);
      ball_.vy = aim_y;
      ball_.vz = 210.0;
    }
  }

  p.action_state = is_jump ? ActionState::SpikeFollow : ActionState::Set;
  p.action_timer = 0.38;
  apply_action_impulse(p, "spike", is_sweet_spot ? 1.45 : 1.15);
  predict_ball_landing();
  assign_closest_defender(1U - p.team);
}

void SunbreakEngine::trigger_keyed_jump(std::uint8_t slot,
                                        double jump_velocity) {
  if (slot >= 4) {
    return;
  }
  auto& p = players_.at(slot);
  if (p.z > 0.0 || p.jump_coil_timer > 0.0 || p.jump_launch_timer > 0.0 ||
      p.jump_cooldown > 0.0) {
    return;
  }
  // Stage 1 (Stand `pose_ready`) -> Stage 2 (Squat `pose_jump_coil`, 115ms) ->
  // Stage 3 (Stand Arms-Up Push-Off `pose_set`, 110ms) -> Stage 4 (In-Air
  // `pose_spike`)
  p.jump_coil_timer = 0.115;
  p.jump_launch_timer = 0.0;
  p.pending_jump_impulse = jump_velocity * (p.on_fire ? 1.16 : 1.0);
  p.action_state = ActionState::JumpCoil;
  p.action_timer = 0.16;
  apply_action_impulse(p, "dig", 0.95);
  add_crater(p.x, p.y, 18.0);
}

auto SunbreakEngine::toggle_partner_tactic(std::uint8_t team) -> DefenseTactic {
  const auto idx = static_cast<std::size_t>(team % 2U);
  defense_tactic_.at(idx) = (defense_tactic_.at(idx) == DefenseTactic::BlockNet)
                                ? DefenseTactic::PeelDeep
                                : DefenseTactic::BlockNet;
  tactic_flash_timer_.at(idx) = 1.65;
  add_callout(defense_tactic_.at(idx) == DefenseTactic::BlockNet
                  ? "PARTNER: BLOCK NET!"
                  : "PARTNER: PEEL DEEP!",
              team == 0 ? 460.0 : 820.0, 290.0,
              defense_tactic_.at(idx) == DefenseTactic::BlockNet ? "#ffb84d"
                                                                 : "#2ce6ff");
  return defense_tactic_.at(idx);
}

void SunbreakEngine::predict_ball_landing() {
  double sim_x = ball_.x;
  double sim_y = ball_.y;
  double sim_z = ball_.z;
  double sim_vx = ball_.vx;
  double sim_vy = ball_.vy;
  double sim_vz = ball_.vz;
  const double grav_mult = 1.0 + ball_.topspin * 0.34;
  const double wind_mult =
      ball_.knuckle > 0.0 ? 1.65 : (ball_.z > 62.0 ? 1.25 : 0.85);
  constexpr double kDt = 0.016;

  for (int i = 0; i < 240; ++i) {
    sim_vx += wind_x_ * wind_mult * kDt;
    sim_vy += ball_.curve_y * kDt;
    sim_x += sim_vx * kDt;
    sim_y += sim_vy * kDt;
    sim_vz -= kCourtGravity * grav_mult * kDt;
    sim_z += sim_vz * kDt;
    if (sim_z <= 6.0) {
      break;
    }
  }
  ball_landing_ = {
      std::clamp(sim_x, kCourtLeftX - 40.0, kCourtRightX + 40.0),
      std::clamp(sim_y, kCourtTopY - 25.0, kCourtBotY + 25.0),
  };
}

void SunbreakEngine::assign_closest_defender(std::uint8_t team) {
  const std::size_t s0 = team == 0 ? 0U : 2U;
  const std::size_t s1 = team == 0 ? 1U : 3U;
  auto& p0 = players_.at(s0);
  auto& p1 = players_.at(s1);
  const double d0 = std::hypot(ball_landing_.x - p0.x, ball_landing_.y - p0.y);
  const double d1 = std::hypot(ball_landing_.x - p1.x, ball_landing_.y - p1.y);
  p0.is_partner_target = (d0 <= d1);
  p1.is_partner_target = (d1 < d0);
}

auto SunbreakEngine::compute_ai_input(const PlayerState& p) -> PlayerInput {
  PlayerInput out{};
  const std::uint8_t team = p.team;
  const bool is_left = (team == 0);
  const auto& partner = players_.at(team == 0 ? (p.slot == 0 ? 1U : 0U)
                                              : (p.slot == 2 ? 3U : 2U));

  if (rally_state_ == RallyState::Serve) {
    if (p.slot == server_slot_ && serve_phase_ == ServePhase::Ready) {
      out.btn_b = true;
      out.hold_b = true;
    }
    return out;
  }

  if (rally_state_ != RallyState::Play) {
    return out;
  }

  double target_x = 0.0;
  double target_y = 0.0;
  const bool ball_on_my_side = is_left ? (ball_landing_.x < kCourtNetX)
                                       : (ball_landing_.x >= kCourtNetX);

  if (ball_on_my_side) {
    const double d_me =
        std::hypot(ball_landing_.x - p.x, ball_landing_.y - p.y);
    const double d_pa =
        std::hypot(ball_landing_.x - partner.x, ball_landing_.y - partner.y);
    const bool i_must_take =
        (last_touch_slot_ != static_cast<int>(p.slot)) &&
        (last_touch_slot_ == static_cast<int>(partner.slot) ||
         d_me <= d_pa + 18.0);

    if (i_must_take) {
      const PoseId desired_pose =
          touch_count_ == 0 ? PoseId::Dig
                            : (touch_count_ == 1 ? PoseId::Set : PoseId::Spike);
      const Vec3 hs = hand_socket_world(p, desired_pose);
      const double hand_offset_x = hs.x - p.x;
      target_x = ball_landing_.x - hand_offset_x;
      target_y = ball_landing_.y;

      if (std::hypot(target_x - p.x, target_y - p.y) > 55.0) {
        out.turbo = true;
      }

      const double dist_ball = std::hypot(ball_.x - hs.x, ball_.y - hs.y);
      if (touch_count_ == 0 && ball_.z < 52.0 && ball_.vz < 0.0 &&
          dist_ball > 34.0 && dist_ball < 98.0) {
        out.btn_b = true;
      } else if (dist_ball < 62.0 && ball_.z > 32.0 && ball_.z < 138.0 &&
                 touch_count_ == 2 && p.z == 0.0 && p.jump_coil_timer <= 0.0 &&
                 p.jump_launch_timer <= 0.0 && p.jump_cooldown <= 0.0) {
        out.btn_b = true;
      }
    } else {
      target_x = is_left ? 545.0 : 735.0;
      target_y = partner.y < 475.0 ? 535.0 : 410.0;
    }
  } else {
    const DefenseTactic tactic = defense_tactic_.at(team);
    const bool is_front_blocker = (!partner.is_ai) || (p.slot % 2U == 0U);
    if (tactic == DefenseTactic::BlockNet && is_front_blocker) {
      target_x = is_left ? kCourtNetX - 36.0 : kCourtNetX + 36.0;
      target_y = std::clamp(ball_.y, kCourtTopY + 35.0, kCourtBotY - 35.0);
      if (std::hypot(target_x - p.x, target_y - p.y) > 45.0) {
        out.turbo = true;
      }
      const bool ball_approaching_high =
          std::abs(ball_.x - kCourtNetX) < 92.0 &&
          ball_.z > kCourtNetHeight - 4.0 &&
          ((is_left && ball_.vx < -65.0) || (!is_left && ball_.vx > 65.0));
      if (ball_approaching_high && p.z == 0.0 && p.jump_coil_timer <= 0.0 &&
          p.jump_launch_timer <= 0.0 && p.jump_cooldown <= 0.0 &&
          std::abs(p.x - kCourtNetX) < 62.0) {
        out.btn_a = true;
      }
    } else if (tactic == DefenseTactic::PeelDeep) {
      target_x = is_left ? 430.0 : 850.0;
      target_y = partner.y < 475.0 ? 548.0 : 405.0;
    } else {
      target_x = is_left ? 355.0 : 925.0;
      target_y = 480.0 + (ball_.y - 480.0) * 0.45;
    }
  }

  const double dx = target_x - p.x;
  const double dy = target_y - p.y;
  if (std::abs(dx) > 6.0) {
    out.mx = std::clamp(dx / 32.0, -1.0, 1.0);
  }
  if (std::abs(dy) > 6.0) {
    out.my = std::clamp(dy / 32.0, -1.0, 1.0);
  }
  return out;
}

void SunbreakEngine::update_pre_contact_anticipation(PlayerState& p) {
  if (p.z > 0.0 || p.dive_lock_timer > 0.0 || p.jump_coil_timer > 0.0 ||
      p.jump_launch_timer > 0.0 || p.action_timer > 0.12) {
    return;
  }
  const bool ball_on_my_side = (p.team == 0 && ball_.x < kCourtNetX + 40.0) ||
                               (p.team == 1 && ball_.x > kCourtNetX - 40.0);
  if (!ball_on_my_side) {
    return;
  }
  const double dist_xy = std::hypot(ball_.x - p.x, ball_.y - p.y);
  if (dist_xy < 95.0 && ball_.z < 115.0 &&
      last_touch_slot_ != static_cast<int>(p.slot)) {
    p.action_state = (touch_team_ != p.team || touch_count_ == 0)
                         ? ActionState::Dig
                         : ActionState::Set;
    p.action_timer = 0.08;
  }
}

void SunbreakEngine::handle_ball_contact(PlayerState& p,
                                         const PlayerInput& input) {
  const auto& prof = kRoster.at(p.roster_index % kRoster.size());
  const std::uint8_t team = p.team;
  const std::uint8_t opp_team = 1U - team;
  const bool is_left = (team == 0);
  const auto& partner = players_.at(team == 0 ? (p.slot == 0 ? 1U : 0U)
                                              : (p.slot == 2 ? 3U : 2U));

  const bool is_net_block =
      (touch_team_ != team && p.z > 14.0 && std::abs(p.x - kCourtNetX) < 86.0);

  if (touch_team_ != team) {
    touch_team_ = team;
    touch_count_ = is_net_block ? 0 : 1;
  } else {
    touch_count_ += 1;
  }
  last_touch_slot_ = static_cast<int>(p.slot);

  // Mandatory Defensive Counter-Play Rule:
  // An active Dig (`active_dig_timer > 0`), Sand Dive (`dive_lock_timer > 0`),
  // or Net Roof Block against an ON FIRE ball/team ALWAYS extinguishes the
  // opponent's ON FIRE streak!
  const bool is_counter_play =
      (p.dive_lock_timer > 0.0 || p.active_dig_timer > 0.0 || is_net_block);
  if ((ball_.on_fire_ball || team_on_fire_.at(opp_team)) &&
      (touch_count_ <= 1 || is_net_block) && is_counter_play) {
    team_on_fire_.at(opp_team) = false;
    streak_.at(opp_team) = 0;
    ball_.on_fire_ball = false;
    ball_.solar_spike = false;
    for (auto& pl : players_) {
      if (pl.team == opp_team) {
        pl.on_fire = false;
      }
    }
    solar_meter_.at(team) = std::min(100.0, solar_meter_.at(team) + 40.0);
    add_callout(is_net_block ? "FIRE EXTINGUISHED! ROOF BLOCK!"
                             : "FIRE EXTINGUISHED! COUNTER SAVE!",
                p.x, p.y - 95.0, "#2ce6ff");
  }

  ball_.impact_squash = 0.85;
  ball_.knuckle = 0.0;
  ball_.curve_y = 0.0;
  ball_.topspin = 0.0;

  if (is_net_block) {
    const double dir = is_left ? 1.0 : -1.0;
    ball_.vx = dir * (310.0 + prof.power * 60.0);
    ball_.vy = input.my * 95.0;
    ball_.vz = -135.0;
    ball_.topspin = 1.1;
    p.action_state = ActionState::SpikeFollow;
    p.action_timer = 0.34;
    apply_action_impulse(p, "spike", 1.35);
    add_callout("MONSTER ROOF BLOCK!", p.x, p.y - 88.0, "#34d399");
  } else if (touch_count_ == 1) {
    // 1st Touch: Bump / Dig toward partner's setting zone near the net
    const double set_zone_x = is_left ? kCourtNetX - 95.0 : kCourtNetX + 95.0;
    const double set_zone_y =
        std::clamp(partner.y, kCourtTopY + 55.0, kCourtBotY - 55.0);
    ball_.vx = (set_zone_x - ball_.x) * 1.12;
    ball_.vy = (set_zone_y - ball_.y) * 1.12;
    ball_.vz = 268.0;
    if (p.dive_lock_timer <= 0.0) {
      p.action_state = ActionState::Dig;
      p.action_timer = 0.32;
      apply_action_impulse(p, "dig", 1.1);
    }
    solar_meter_.at(team) = std::min(100.0, solar_meter_.at(team) + 14.0);
  } else if (touch_count_ == 2 && p.z < 18.0) {
    // 2nd Touch: High Overhead Alley-Oop Set for partner's spike!
    const double att_x = is_left ? kCourtNetX - 56.0 : kCourtNetX + 56.0;
    const double att_y =
        std::clamp(partner.y, kCourtTopY + 50.0, kCourtBotY - 50.0);
    ball_.vx = (att_x - ball_.x) * 1.05;
    ball_.vy = (att_y - ball_.y) * 1.05;
    ball_.vz = 295.0;
    p.action_state = ActionState::Set;
    p.action_timer = 0.36;
    apply_action_impulse(p, "set", 1.15);
    solar_meter_.at(team) = std::min(100.0, solar_meter_.at(team) + 16.0);
  } else {
    // 3rd Touch or Airborne Attack: Spike / Cut-Shot over the net!
    const double dir = is_left ? 1.0 : -1.0;
    const bool airborne = (p.z > 14.0);
    const bool use_solar =
        airborne && (solar_meter_.at(team) >= 100.0 || p.on_fire);
    if (use_solar) {
      if (!p.on_fire) {
        solar_meter_.at(team) = 0.0;
      }
      ball_.vx = dir * (515.0 + prof.power * 75.0);
      ball_.vy = input.my * 145.0;
      ball_.vz = -165.0;
      ball_.topspin = 1.45;
      ball_.solar_spike = true;
      ball_.on_fire_ball = p.on_fire;
      cut_in_timer_ = 1.1;
      cut_in_label_ = p.on_fire ? "ON FIRE SLAM!" : "SOLAR OVERDRIVE SPIKE!";
      add_callout(p.on_fire ? "BOOMSHAKALAKA! ON FIRE SLAM!"
                            : "SOLAR OVERDRIVE SPIKE!",
                  p.x, p.y - 98.0, "#ff5500");
    } else if (airborne) {
      ball_.vx = dir * (435.0 + prof.power * 55.0);
      ball_.vy = input.my * 120.0;
      ball_.vz = -110.0;
      ball_.topspin = 0.95;
      add_callout("POWER SPIKE!", p.x, p.y - 84.0, "#ffb84d");
    } else {
      ball_.vx = dir * 325.0;
      ball_.vy = input.my * 95.0;
      ball_.vz = 215.0;
    }
    p.action_state = airborne ? ActionState::SpikeFollow : ActionState::Set;
    p.action_timer = 0.38;
    apply_action_impulse(p, "spike", use_solar ? 1.65 : 1.25);
  }

  predict_ball_landing();
}

void SunbreakEngine::score_point(std::uint8_t winning_team,
                                 std::string_view reason) {
  const auto win_idx = static_cast<std::size_t>(winning_team % 2U);
  const auto lose_idx = 1U - win_idx;
  score_.at(win_idx) += 1;
  streak_.at(win_idx) += 1;
  streak_.at(lose_idx) = 0;
  team_on_fire_.at(lose_idx) = false;

  if (streak_.at(win_idx) >= 3) {
    team_on_fire_.at(win_idx) = true;
    solar_meter_.at(win_idx) = 100.0;
    add_callout("HEATING UP! TEAM IS ON FIRE!", kCourtNetX, 240.0, "#ff5500");
  } else if (streak_.at(win_idx) == 2) {
    add_callout("HEATING UP! (2 IN A ROW)", kCourtNetX, 248.0, "#ffb84d");
  } else {
    add_callout(std::string(reason), kCourtNetX, 255.0, "#2ce6ff");
  }

  if (is_deuce()) {
    solar_meter_.at(0) = std::min(100.0, solar_meter_.at(0) + 25.0);
    solar_meter_.at(1) = std::min(100.0, solar_meter_.at(1) + 25.0);
    add_callout("DEUCE! WIN BY 2!", kCourtNetX, 215.0, "#ffb84d");
  }

  for (auto& p : players_) {
    p.on_fire = team_on_fire_.at(p.team);
  }

  serving_team_ = winning_team;
  rally_state_ = RallyState::PointPause;
  point_pause_timer_ = 1.05;
}

void SunbreakEngine::add_crater(double x, double y, double radius) {
  craters_.insert(craters_.begin(), SandCrater{x, y, radius, 0.0});
  if (craters_.size() > 24U) {
    craters_.pop_back();
  }
}

void SunbreakEngine::add_callout(std::string text, double x, double y,
                                 std::string color) {
  callouts_.push_back(
      CalloutBanner{std::move(text), x, y, std::move(color), 0.0});
  if (callouts_.size() > 6U) {
    callouts_.erase(callouts_.begin());
  }
}

void SunbreakEngine::step(double dt,
                          const std::array<PlayerInput, 4>& raw_inputs) {
  if (demo_freeze_) {
    for (auto& p : players_) {
      step_soft_body(p, dt);
      p.skeleton = evaluate_skeleton_pose(p);
    }
    return;
  }

  if (cut_in_timer_ > 0.0) {
    cut_in_timer_ = std::max(0.0, cut_in_timer_ - dt);
  }
  for (double& tf : tactic_flash_timer_) {
    if (tf > 0.0) {
      tf = std::max(0.0, tf - dt);
    }
  }
  for (auto it = craters_.begin(); it != craters_.end();) {
    it->age += dt * 0.085;
    if (it->age >= 1.0) {
      it = craters_.erase(it);
    } else {
      ++it;
    }
  }
  for (auto it = callouts_.begin(); it != callouts_.end();) {
    it->age += dt;
    if (it->age >= 1.65) {
      it = callouts_.erase(it);
    } else {
      ++it;
    }
  }

  if (rally_state_ == RallyState::PointPause) {
    point_pause_timer_ -= dt;
    if (point_pause_timer_ <= 0.0) {
      reset_serve(serving_team_);
    }
    return;
  }

  for (std::size_t i = 0; i < 4; ++i) {
    auto& p = players_.at(i);
    const auto& prof = kRoster.at(p.roster_index % kRoster.size());
    const PlayerInput input = p.is_ai ? compute_ai_input(p) : raw_inputs.at(i);

    if (input.turbo && p.turbo > 8.0) {
      p.turbo_active = true;
      if (!p.on_fire) {
        p.turbo = std::max(0.0, p.turbo - dt * 36.0);
      }
    } else {
      p.turbo_active = false;
      p.turbo = std::min(100.0, p.turbo + dt * (p.on_fire ? 45.0 : 22.0));
    }

    if (!p.is_ai && input.btn_switch) {
      const bool on_def_serve =
          (rally_state_ == RallyState::Serve && serving_team_ != p.team);
      const bool on_def_play =
          (rally_state_ == RallyState::Play && touch_team_ != p.team);
      if (on_def_serve || on_def_play) {
        toggle_partner_tactic(p.team);
      }
    }

    const double target_face_x =
        rally_state_ == RallyState::Serve ? kCourtNetX : ball_.x;
    if (std::abs(target_face_x - p.x) > 12.0) {
      p.facing = target_face_x >= p.x ? 1.0 : -1.0;
    }

    if (rally_state_ == RallyState::Serve) {
      if (p.slot == server_slot_) {
        if (serve_phase_ == ServePhase::Ready) {
          p.vy = input.my * 165.0;
          p.y =
              std::clamp(p.y + p.vy * dt, kCourtTopY + 25.0, kCourtBotY - 25.0);
          const Vec3 hs = hand_socket_world(p, PoseId::Ready);
          ball_.x = hs.x + p.facing * 10.0;
          ball_.y = hs.y;
          ball_.z = std::max(22.0, hs.z - 6.0);

          if (input.btn_a) {
            start_serve_toss(p.slot, false);
          } else if (input.btn_b) {
            start_serve_toss(p.slot, true);
          }
        } else {
          const double charge_rate = p.on_fire ? 1.38 : 1.18;
          serve_charge_ = std::min(1.0, serve_charge_ + dt * charge_rate);
          ball_.vz -= kCourtGravity * 0.72 * dt;
          ball_.z += ball_.vz * dt;

          if (serve_is_jump_ && serve_charge_ >= 0.30 && p.z == 0.0 &&
              p.jump_coil_timer <= 0.0) {
            trigger_keyed_jump(p.slot, 248.0 + prof.jump * 58.0);
          }

          const bool release_trigger =
              serve_charge_ >= 0.24 && (input.btn_a || input.btn_b);
          const bool ai_trigger = p.is_ai && serve_charge_ >= ai_serve_target_;
          const bool auto_max = (serve_charge_ >= 1.0 || ball_.z <= 24.0);
          if (release_trigger || ai_trigger || auto_max) {
            execute_serve_strike(p.slot, input, serve_charge_);
          }
        }
      }
    } else if (rally_state_ == RallyState::Play) {
      if (p.active_dig_timer > 0.0) {
        p.active_dig_timer = std::max(0.0, p.active_dig_timer - dt);
      }
      if (p.jump_cooldown > 0.0) {
        p.jump_cooldown = std::max(0.0, p.jump_cooldown - dt);
      }

      if (p.dive_lock_timer > 0.0) {
        p.dive_lock_timer -= dt;
        p.vx *= 0.92;
        p.vy *= 0.92;
      } else {
        const double turbo_boost =
            p.on_fire ? 1.38 : (p.turbo_active ? 1.32 : 1.0);
        const double max_spd = (235.0 + prof.speed * 80.0) * turbo_boost;
        p.vx = input.mx * max_spd;
        p.vy = input.my * max_spd * 0.88;

        if ((input.btn_a || input.btn_b) && p.z == 0.0 &&
            p.jump_coil_timer <= 0.0 && p.jump_launch_timer <= 0.0) {
          const Vec3 hs_dig = hand_socket_world(p, PoseId::Dive);
          const double dist_ball =
              std::hypot(ball_.x - hs_dig.x, ball_.y - hs_dig.y);
          // Backcourt Reception Guard: touch_count == 0 after net crossing is
          // defensive reception!
          const bool on_defense =
              (touch_team_ != p.team) || (touch_count_ == 0);
          const bool near_net = std::abs(p.x - kCourtNetX) <= 78.0;

          if (on_defense) {
            p.active_dig_timer = 0.36;
          }

          const bool want_dive = input.btn_b && on_defense && ball_.z < 76.0 &&
                                 dist_ball > 20.0 && dist_ball < 145.0 &&
                                 (std::abs(input.mx) > 0.1 ||
                                  std::abs(input.my) > 0.1 || !near_net);
          const bool want_counter_dig =
              (input.btn_a || input.btn_b) && on_defense && !near_net;

          if (want_dive) {
            const double dx = ball_.x - hs_dig.x;
            const double dy = ball_.y - hs_dig.y;
            const double d = std::max(1.0, std::hypot(dx, dy));
            p.vx = (dx / d) * 410.0;
            p.vy = (dy / d) * 340.0;
            p.vz = 90.0;
            p.dive_lock_timer = 0.44;
            p.action_state = ActionState::Dive;
            p.action_timer = 0.44;
            apply_action_impulse(p, "dive", 1.35);
            add_crater(p.x, p.y, 26.0);
          } else if (want_counter_dig) {
            p.action_state = ActionState::Dig;
            p.action_timer = 0.34;
            apply_action_impulse(p, "dig", 1.15);
          } else if (p.jump_cooldown <= 0.0) {
            if (p.is_ai) {
              p.jump_cooldown = 0.72;
            }
            trigger_keyed_jump(p.slot, 248.0 + prof.jump * 62.0);
          }
        }
      }

      p.x += p.vx * dt;
      p.y += p.vy * dt;
      if (p.team == 0) {
        p.x = std::clamp(p.x, kCourtLeftX - 45.0, kCourtNetX - 18.0);
      } else {
        p.x = std::clamp(p.x, kCourtNetX + 18.0, kCourtRightX + 45.0);
      }
      p.y = std::clamp(p.y, kCourtTopY - 30.0, kCourtBotY + 30.0);

      update_pre_contact_anticipation(p);

      const PoseId active_pose = select_pose_id(p);
      const Vec3 hs = hand_socket_world(p, active_pose);
      const double dist_xy = std::hypot(ball_.x - hs.x, ball_.y - hs.y);
      const double dist_z = std::abs(ball_.z - hs.z);
      const double hit_r_xy = p.action_state == ActionState::Dive
                                  ? 54.0
                                  : (p.active_dig_timer > 0.0 ? 48.0 : 42.0);
      const double hit_r_z = p.z > 14.0 ? 44.0 : 38.0;

      if (dist_xy < hit_r_xy && dist_z < hit_r_z &&
          last_touch_slot_ != static_cast<int>(p.slot)) {
        handle_ball_contact(p, input);
      }
    }

    // Step 4-Stage Keyed Jump Chain:
    // Stage 1 (Stand) -> Stage 2 (Squat `JumpCoil`, 115ms) ->
    // Stage 3 (Stand Arms-Up `JumpLaunch`, 110ms) -> Stage 4 (In-Air `Spike`)
    if (p.jump_coil_timer > 0.0) {
      p.jump_coil_timer -= dt;
      if (p.jump_coil_timer <= 0.0) {
        p.vz = p.pending_jump_impulse > 0.0 ? p.pending_jump_impulse : 260.0;
        p.pending_jump_impulse = 0.0;
        p.jump_launch_timer = 0.11;
        p.action_state = ActionState::JumpLaunch;
        p.action_timer = 0.62;
        apply_action_impulse(p, "jump", 1.35);
      }
    }
    if (p.jump_launch_timer > 0.0) {
      p.jump_launch_timer -= dt;
      if (p.jump_launch_timer <= 0.0 && p.z > 0.0 &&
          p.action_state == ActionState::JumpLaunch) {
        p.action_state = ActionState::Spike;
      }
    }

    if (p.z > 0.0 || p.vz != 0.0) {
      const bool near_apex = (p.z > 32.0 && std::abs(p.vz) < 85.0);
      const double eff_grav = near_apex ? kCourtGravity * 0.48 : kCourtGravity;
      p.vz -= eff_grav * dt;
      p.z += p.vz * dt;
      if (p.z > 0.0 && p.z < 22.0 && p.vz < -155.0 &&
          p.action_state == ActionState::Spike) {
        p.action_state = ActionState::JumpLaunch;
      }
      if (p.z <= 0.0) {
        p.z = 0.0;
        p.vz = 0.0;
        p.jump_launch_timer = 0.0;
        p.jump_cooldown = p.is_ai ? 0.55 : 0.18;
        p.action_state = ActionState::JumpCoil;
        p.action_timer = 0.135;
        apply_action_impulse(p, "land", 1.25);
        add_crater(p.x, p.y, 20.0);
      }
    }

    if (p.action_timer > 0.0) {
      p.action_timer -= dt;
      if (p.action_timer <= 0.0 && p.z == 0.0 && p.jump_coil_timer <= 0.0 &&
          p.jump_launch_timer <= 0.0) {
        p.action_state = ActionState::Idle;
      }
    }

    const PoseId next_pose = select_pose_id(p);
    if (next_pose != p.active_pose) {
      p.prev_pose = p.active_pose;
      p.active_pose = next_pose;
      p.pose_blend = 0.0;
    } else if (p.pose_blend < 1.0) {
      p.pose_blend = std::min(1.0, p.pose_blend + dt * 6.8);
    }

    step_soft_body(p, dt);
    p.skeleton = evaluate_skeleton_pose(p);
  }

  // Step Volleyball Physics
  if (ball_.impact_squash > 0.0) {
    ball_.impact_squash = std::max(0.0, ball_.impact_squash - dt * 6.5);
  }

  if (rally_state_ == RallyState::Play) {
    const double prev_x = ball_.x;
    const double grav_mult = 1.0 + ball_.topspin * 0.34;
    const double wind_mult =
        ball_.knuckle > 0.0 ? 1.65 : (ball_.z > 62.0 ? 1.25 : 0.85);

    ball_.vx += wind_x_ * wind_mult * dt;
    ball_.vy += ball_.curve_y * dt;
    if (ball_.knuckle > 0.0) {
      ball_.knuckle_phase += 14.5 * dt;
      ball_.vy += std::cos(ball_.knuckle_phase) * 265.0 * ball_.knuckle * dt;
      predict_ball_landing();
    }

    ball_.x += ball_.vx * dt;
    ball_.y += ball_.vy * dt;
    ball_.vz -= kCourtGravity * grav_mult * dt;
    ball_.z += ball_.vz * dt;
    const double spin_spd =
        ball_.knuckle > 0.0
            ? 1.4
            : (ball_.vx * 0.035 + (ball_.topspin > 0.0 ? 13.5 : 7.5));
    ball_.spin_angle += spin_spd * dt;

    ball_.trail.push_back({ball_.x, ball_.y, ball_.z});
    if (ball_.trail.size() > 10U) {
      ball_.trail.erase(ball_.trail.begin());
    }

    if ((prev_x < kCourtNetX && ball_.x >= kCourtNetX) ||
        (prev_x > kCourtNetX && ball_.x <= kCourtNetX)) {
      if (ball_.z < kCourtNetHeight) {
        ball_.vx *= 0.45;
        ball_.vz = std::max(65.0, std::abs(ball_.vz) * 0.4);
        ball_.z = kCourtNetHeight + 2.0;
        ball_.topspin = 0.0;
        ball_.curve_y = 0.0;
        ball_.knuckle = 0.0;
        ball_.impact_squash = 0.7;
        add_callout("NET CORD TRICKLE!", kCourtNetX, ball_.y - 75.0, "#ffb84d");
        predict_ball_landing();
      } else {
        const std::uint8_t new_side = ball_.vx > 0.0 ? 1U : 0U;
        if (touch_team_ != new_side) {
          touch_team_ = new_side;
          touch_count_ = 0;
          last_touch_slot_ = -1;
          assign_closest_defender(new_side);
        }
      }
    }

    if (ball_.z <= 6.0) {
      ball_.z = 6.0;
      ball_.impact_squash = 1.0;
      add_crater(ball_.x, ball_.y, ball_.on_fire_ball ? 34.0 : 24.0);
      const bool in_y =
          (ball_.y >= kCourtTopY - 8.0 && ball_.y <= kCourtBotY + 8.0);
      const bool in_x =
          (ball_.x >= kCourtLeftX - 8.0 && ball_.x <= kCourtRightX + 8.0);
      const std::uint8_t landed_side = ball_.x < kCourtNetX ? 0U : 1U;
      if (in_x && in_y) {
        score_point(1U - landed_side, "IN BOUNDS");
      } else {
        const std::uint8_t hitter_team =
            last_touch_slot_ >= 0
                ? players_.at(static_cast<std::size_t>(last_touch_slot_)).team
                : static_cast<std::uint8_t>(1U - landed_side);
        score_point(1U - hitter_team, "OUT OF BOUNDS");
      }
    }
  }
}

void SunbreakEngine::apply_preset(std::string_view preset_name) {
  reset_match();
  if (preset_name == "sunset_pose") {
    lighting_theme_ = LightingTheme::Sunset;
    physics_preset_ = PhysicsPreset::Doax;
    pose_cam_active_ = true;
    rally_state_ = RallyState::Serve;
    serve_phase_ = ServePhase::Toss;
    serve_is_jump_ = true;
    serve_charge_ = 0.84;
    set_wind(12, 50.4, "12 KT -> E");
    defense_tactic_ = {DefenseTactic::BlockNet, DefenseTactic::PeelDeep};
    tactic_flash_timer_ = {4.0, 4.0};
    demo_freeze_ = true;

    players_.at(0).z = 38.0;
    players_.at(0).action_state = ActionState::Spike;
    players_.at(0).prev_pose = PoseId::Set;
    players_.at(0).active_pose = PoseId::Spike;
    players_.at(0).pose_blend = 0.78;
    apply_action_impulse(players_.at(0), "jump", 1.45);

    players_.at(1).x = 598.0;
    players_.at(1).y = 435.0;
    players_.at(1).action_state = ActionState::JumpCoil;
    players_.at(1).active_pose = PoseId::JumpCoil;

    players_.at(2).x = 745.0;
    players_.at(2).y = 485.0;
    players_.at(2).action_state = ActionState::Dig;
    players_.at(2).active_pose = PoseId::Dig;

    players_.at(3).x = 885.0;
    players_.at(3).y = 585.0;

    for (auto& p : players_) {
      p.skeleton = evaluate_skeleton_pose(p);
    }
    const Vec3 hs0 = hand_socket_world(players_.at(0), PoseId::Spike);
    ball_.x = hs0.x + 8.0;
    ball_.y = hs0.y;
    ball_.z = hs0.z + 6.0;
  } else if (preset_name == "neon_deuce") {
    lighting_theme_ = LightingTheme::Neon;
    physics_preset_ = PhysicsPreset::Overdrive;
    pose_cam_active_ = true;
    rally_state_ = RallyState::Play;
    demo_freeze_ = true;
    score_ = {14, 14};
    set_wind(14, -58.8, "14 KT <- W");
    defense_tactic_ = {DefenseTactic::BlockNet, DefenseTactic::PeelDeep};
    tactic_flash_timer_ = {4.0, 4.0};
    team_on_fire_ = {true, false};
    streak_ = {3, 0};
    solar_meter_ = {35.0, 25.0};
    players_.at(0).on_fire = true;
    players_.at(1).on_fire = true;
    cut_in_timer_ = 4.0;
    cut_in_label_ = "ON FIRE SLAM!";

    players_.at(0).x = 560.0;
    players_.at(0).y = 445.0;
    players_.at(0).z = 52.0;
    players_.at(0).vz = 90.0;
    players_.at(0).action_state = ActionState::SpikeFollow;
    players_.at(0).prev_pose = PoseId::Spike;
    players_.at(0).active_pose = PoseId::SpikeFollow;
    players_.at(0).pose_blend = 0.85;
    apply_action_impulse(players_.at(0), "spike", 1.7);

    players_.at(1).x = 420.0;
    players_.at(1).y = 535.0;
    players_.at(1).action_state = ActionState::Set;
    players_.at(1).active_pose = PoseId::Set;

    players_.at(2).x = 740.0;
    players_.at(2).y = 485.0;
    players_.at(2).action_state = ActionState::Dive;
    players_.at(2).active_pose = PoseId::Dive;
    players_.at(2).dive_lock_timer = 4.0;

    players_.at(3).x = 895.0;
    players_.at(3).y = 585.0;
    players_.at(3).action_state = ActionState::JumpCoil;
    players_.at(3).active_pose = PoseId::JumpCoil;

    for (auto& p : players_) {
      p.skeleton = evaluate_skeleton_pose(p);
    }

    const Vec3 hs_spike =
        hand_socket_world(players_.at(0), PoseId::SpikeFollow);
    ball_.x = hs_spike.x + 28.0;
    ball_.y = hs_spike.y + 8.0;
    ball_.z = hs_spike.z - 10.0;
    ball_.vx = 520.0;
    ball_.vy = 45.0;
    ball_.vz = -180.0;
    ball_.solar_spike = true;
    ball_.on_fire_ball = true;
    ball_.trail = {
        {hs_spike.x - 18.0, hs_spike.y - 6.0, hs_spike.z + 24.0},
        {hs_spike.x - 4.0, hs_spike.y, hs_spike.z + 12.0},
        {hs_spike.x + 12.0, hs_spike.y + 4.0, hs_spike.z},
    };
    add_callout("KASUMI IS ON FIRE! BOOMSHAKALAKA!", 415.0, 228.0, "#ff5500");
    add_callout("FIRE EXTINGUISHED! DIVING SAVE!", 645.0, 266.0, "#2ce6ff");
  }
}

} // namespace sunbreak
