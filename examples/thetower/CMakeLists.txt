add_executable(
  thetower
  easy_enemy.cpp
  enemy_movement.cpp
  enemy_size.cpp
  game_state.cpp
  main.cpp
  meters.cpp
  meters.hpp
  tower.cpp
)

target_link_libraries(thetower PRIVATE p5cpp p5cpp_camera p5cpp_animation)
