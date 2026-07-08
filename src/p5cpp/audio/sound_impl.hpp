#pragma once

#include <p5cpp/audio/sound.hpp>

#include <cstdint>
#include <filesystem>
#include <span>

struct ma_engine;

namespace p5cpp
{
    std::unique_ptr<SoundImpl> loadSoundImpl(ma_engine* engine, const std::filesystem::path& soundFilePath);
    std::unique_ptr<SoundImpl> loadSoundImpl(ma_engine* engine, std::span<const uint8_t> soundData);
} // namespace p5cpp
