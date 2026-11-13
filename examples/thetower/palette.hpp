#pragma once

#include <p5cpp/p5cpp.hpp>

// colors that mean the same on the playfield and in the interface, so gold looks like gold wherever it shows up

inline constexpr p5::color_t accentColor = p5::rgba(60, 180, 220); // the tower, its shots and interface highlights
inline constexpr p5::color_t goldColor = p5::rgba(230, 200, 90);
inline constexpr p5::color_t coinColor = p5::rgba(180, 140, 240);
inline constexpr p5::color_t critColor = p5::rgba(255, 120, 60);
inline constexpr p5::color_t healthColor = p5::rgba(110, 200, 120);
inline constexpr p5::color_t dangerColor = p5::rgba(230, 80, 80);
inline constexpr p5::color_t slowColor = p5::rgba(130, 195, 255); // enemies the tower has slowed down
inline constexpr p5::color_t heatColor = p5::rgba(255, 150, 60);  // the barrel while a rapid fire burst runs

// the playfield behind everything
inline constexpr p5::color_t floorColor = p5::rgba(23, 26, 32);
inline constexpr p5::color_t floorGlowColor = p5::rgba(46, 62, 78);
inline constexpr p5::color_t gridColor = p5::rgba(120, 150, 180);
inline constexpr p5::color_t shadowColor = p5::rgba(0);
