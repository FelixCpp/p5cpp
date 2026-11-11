add_executable(
  thetower
  bullet.cpp
  death_effect.cpp
  enemy.cpp
  enemy_core.cpp
  enemy_damage_feedback.cpp
  enemy_health.cpp
  enemy_movement.cpp
  enemy_size.cpp
  enemy_spawner.cpp
  game_events.cpp
  main.cpp
  melee_enemy.cpp
  meters.cpp
  meters.hpp
  particle_burst.cpp
  tower.cpp
  tower_stats.cpp
  tower_weapon.cpp
  world.cpp
)

target_link_libraries(thetower PRIVATE p5cpp p5cpp_camera p5cpp_animation)
