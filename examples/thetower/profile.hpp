#pragma once

#include <filesystem>

#include "upgrade_levels.hpp"

// everything that survives between rounds
struct Profile
{
    int coins;
    int bestWave;
    UpgradeLevels startLevels;

    // result of the last round, only shown in the menu, not saved
    int lastWave;
    int lastCoins;
};

Profile profile_load(const std::filesystem::path& path);
void profile_save(const Profile& profile, const std::filesystem::path& path);
