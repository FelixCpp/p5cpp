#pragma once

#include <p5cpp/p5cpp.hpp>

// colors that mean the same on the playfield and in the interface, so gold looks like gold wherever it shows up

inline constexpr p5::color_t accentColor = p5::rgba(60, 180, 220); // the tower, its shots and interface highlights
inline constexpr p5::color_t goldColor = p5::rgba(230, 200, 90);
inline constexpr p5::color_t coinColor = p5::rgba(180, 140, 240);
inline constexpr p5::color_t critColor = p5::rgba(255, 120, 60);
inline constexpr p5::color_t healthColor = p5::rgba(110, 200, 120);
inline constexpr p5::color_t dangerColor = p5::rgba(230, 80, 80);
