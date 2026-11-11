#include "profile.hpp"

#include <fstream>
#include <string>

#include "upgrades.hpp"

// plain "key value" lines, unknown keys are skipped so old saves keep loading

Profile profile_load(const std::filesystem::path& path)
{
    Profile profile = {
        .coins = 0,
        .bestWave = 0,
        .startLevels = {},
        .lastWave = 0,
        .lastCoins = 0,
    };

    std::ifstream file(path);
    std::string key;
    int value = 0;

    while (file >> key >> value) {
        if (key == "coins") {
            profile.coins = value;
        } else if (key == "bestWave") {
            profile.bestWave = value;
        } else if (const UpgradeDefinition* definition = upgrade_find(key)) {
            profile.startLevels.levels[static_cast<size_t>(definition->id)] = value;
        }
    }

    return profile;
}

void profile_save(const Profile& profile, const std::filesystem::path& path)
{
    std::ofstream file(path);

    file << "coins " << profile.coins << '\n';
    file << "bestWave " << profile.bestWave << '\n';
    for (const UpgradeDefinition& definition : upgrade_definitions()) {
        file << definition.key << ' ' << upgrade_level(profile.startLevels, definition.id) << '\n';
    }
}
