#pragma once

#include <string>
#include <vector>

namespace p5cpp
{
    void info(const std::string& message);
    void debug(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    // Prints a bordered banner box around `lines`, auto-sized to the widest
    // line. Intended for section headers / summaries (e.g. engine startup)
    // that should stand out from regular log lines above.
    void logBox(const std::vector<std::string>& lines);
} // namespace p5cpp
