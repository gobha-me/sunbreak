#include "sunbreak/match_engine.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void require_true(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "CONTRACT FAILURE: " << msg << "\n";
    std::exit(1);
  }
}

void test_roster_and_pose_landmarks() {
  require_true(sunbreak::kRoster.size() == 6U, "Expected 6 athletes in roster");
  require_true(sunbreak::kRoster.at(0).name == "KASUMI",
               "Expected slot 0 athlete KASUMI");
  require_true(sunbreak::kRoster.at(1).name == "ELENA",
               "Expected slot 1 athlete ELENA");

  const auto& coil_lm = sunbreak::pose_landmarks(sunbreak::PoseId::JumpCoil);
  require_true(coil_lm.key == "pose_jump_coil", "JumpCoil key mismatch");
  require_true(coil_lm.crouch > 0.30,
               "Expected positive crouch compression in JumpCoil");

  const auto& spike_lm = sunbreak::pose_landmarks(sunbreak::PoseId::Spike);
  require_true(spike_lm.crouch < 0.0,
               "Expected negative upward extension crouch in Spike");
}

void test_four_stage_jump_chain_and_skeleton_bones() {
  sunbreak::SunbreakEngine eng;
  eng.reset_match();

  const auto& p0 = eng.players().at(0);
  require_true(p0.active_pose == sunbreak::PoseId::Ready,
               "Expected Stage 1 Ready pose before jump");

  // Trigger jump -> Stage 2: Squat (`JumpCoil`, 115ms preload)
  eng.trigger_keyed_jump(0, 280.0);
  require_true(p0.action_state == sunbreak::ActionState::JumpCoil,
               "Expected Stage 2 JumpCoil action state immediately on jump");
  require_true(p0.z == 0.0,
               "Athlete must stay grounded during Squat preload coil");

  std::array<sunbreak::PlayerInput, 4> neutral{};
  eng.step(0.06, neutral);
  require_true(p0.active_pose == sunbreak::PoseId::JumpCoil,
               "Expected JumpCoil active pose during Squat preload");
  require_true(std::abs(p0.skeleton.thigh_deg) > 10.0,
               "Expected SkeletonPose2D thigh bend during Squat preload");

  // Step past 115ms -> Stage 3: Stand Arms-Up Push-Off (`JumpLaunch` ->
  // `PoseId::Set`)
  eng.step(0.07, neutral);
  require_true(p0.z > 0.0, "Expected positive airborne Z after Squat coil");
  require_true(p0.action_state == sunbreak::ActionState::JumpLaunch,
               "Expected Stage 3 JumpLaunch action state on push-off");
  require_true(p0.active_pose == sunbreak::PoseId::Set,
               "Expected Set (Stand Arms-Up) pose during Stage 3 push-off");
  require_true(p0.skeleton.arm_deg > 15.0,
               "Expected upward Skeleton2D arm rotation during JumpLaunch");

  // Step past 110ms launch window -> Stage 4: In-Air Bow-Arch (`PoseId::Spike`)
  eng.step(0.12, neutral);
  require_true(p0.action_state == sunbreak::ActionState::Spike,
               "Expected Stage 4 Spike action state at apex");
  require_true(p0.active_pose == sunbreak::PoseId::Spike,
               "Expected Spike pose in mid-air");
}

void test_serve_power_bar_and_sweet_spot() {
  sunbreak::SunbreakEngine eng;
  eng.reset_match();

  // Standing Float Serve in Sweet Spot (0.82) -> Knuckle Float!
  eng.start_serve_toss(0, false);
  require_true(eng.serve_phase() == sunbreak::ServePhase::Toss,
               "Expected ServePhase::Toss after start_serve_toss");

  sunbreak::PlayerInput aim{};
  aim.my = -0.5;
  eng.execute_serve_strike(0, aim, 0.82);
  require_true(eng.rally_state() == sunbreak::RallyState::Play,
               "Expected RallyState::Play after serve strike");
  require_true(eng.ball().knuckle >= 0.99,
               "Expected Sweet-Spot Float Serve to activate Knuckle wobble");

  // Jump Serve in Sweet Spot (0.84) -> Solar Laser Serve!
  eng.reset_serve(0);
  eng.start_serve_toss(0, true);
  eng.execute_serve_strike(0, aim, 0.84);
  require_true(eng.ball().solar_spike,
               "Expected Sweet-Spot Jump Serve to trigger Solar Laser Serve");
  require_true(eng.ball().topspin > 1.0,
               "Expected positive topspin on Sweet-Spot Jump Serve");
}

void test_backcourt_reception_never_jumps() {
  sunbreak::SunbreakEngine eng;
  eng.reset_match();

  // Opponent serves across net onto Team 0 side (`touch_team == 0, touch_count
  // == 0`)
  eng.reset_serve(1);
  eng.start_serve_toss(2, false);
  sunbreak::PlayerInput srv_in{};
  eng.execute_serve_strike(2, srv_in, 0.70);

  // Move ball across net to Team 0 backcourt near P1 (slot 0)
  auto& b = eng.ball_mut();
  b.x = 320.0;
  b.y = 500.0;
  b.z = 48.0;
  b.vx = -220.0;

  // Simulate net crossing so touch_team == 0 and touch_count == 0
  std::array<sunbreak::PlayerInput, 4> inputs{};
  inputs.at(0).btn_a = true; // Press J (Counter-Dig) in backcourt
  eng.step(0.016, inputs);

  const auto& p0 = eng.players().at(0);
  require_true(p0.z == 0.0 && p0.jump_coil_timer == 0.0,
               "Pressing Dig in backcourt on reception must NOT jump");
  require_true(p0.action_state == sunbreak::ActionState::Dig,
               "Pressing Dig in backcourt must enter Dig stance");
}

void test_counter_play_extinguishes_on_fire() {
  sunbreak::SunbreakEngine eng;
  eng.reset_match();

  // Put Team 1 ON FIRE and fire an ON FIRE spike into Team 0's court
  eng.apply_preset("neon_deuce");
  eng.reset_serve(1);
  eng.players_mut().at(2).on_fire = true;
  eng.start_serve_toss(2, true);
  sunbreak::PlayerInput srv_in{};
  eng.execute_serve_strike(2, srv_in, 0.84);
  require_true(eng.ball().on_fire_ball,
               "Expected ON FIRE ball from ON FIRE server");

  // Place ball directly at P0's outstretched forearm socket while P0 performs
  // active Counter-Dig
  auto& p0 = eng.players_mut().at(0);
  p0.x = 300.0;
  p0.y = 480.0;
  p0.z = 0.0;
  const sunbreak::Vec3 hs = eng.hand_socket_world(p0, sunbreak::PoseId::Dig);
  auto& b = eng.ball_mut();
  b.x = hs.x;
  b.y = hs.y;
  b.z = hs.z;
  b.vx = -300.0;

  std::array<sunbreak::PlayerInput, 4> inputs{};
  inputs.at(0).btn_a = true;
  eng.step(0.016, inputs);

  require_true(!eng.ball().on_fire_ball,
               "Counter-Dig must extinguish ON FIRE ball");
  require_true(eng.solar_meter().at(0) >= 40.0,
               "Counter-Play extinguish must award Solar Overdrive meter");
}

void test_partner_tactics_and_deuce_state() {
  sunbreak::SunbreakEngine eng;
  eng.reset_match();
  require_true(eng.defense_tactic().at(0) == sunbreak::DefenseTactic::BlockNet,
               "Default Team 0 tactic must be BlockNet");
  const auto next_t = eng.toggle_partner_tactic(0);
  require_true(next_t == sunbreak::DefenseTactic::PeelDeep,
               "Toggling Team 0 tactic must switch to PeelDeep");

  eng.set_score(14, 14);
  require_true(eng.is_deuce(), "14-14 must be detected as Deuce");
  require_true(eng.match_point_team() == -1,
               "14-14 tied Deuce has no Match Point holder");

  eng.set_score(15, 14);
  require_true(eng.is_deuce(), "15-14 within 1 point is still Deuce state");
  require_true(eng.match_point_team() == 0,
               "15-14 must report Team 0 on Match Point");
}

} // namespace

auto main() -> int {
  test_roster_and_pose_landmarks();
  test_four_stage_jump_chain_and_skeleton_bones();
  test_serve_power_bar_and_sweet_spot();
  test_backcourt_reception_never_jumps();
  test_counter_play_extinguishes_on_fire();
  test_partner_tactics_and_deuce_state();
  std::cout
      << "SUNBREAK C++23 MATCH ENGINE CONTRACT SUITE: ALL CHECKS PASSED\n";
  return 0;
}
