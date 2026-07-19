#include <p5cpp/application/logging.hpp>

#include "../system/utf8_view.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <format>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace p5cpp
{
    namespace
    {
        enum class Level
        {
            Info,
            Debug,
            Warning,
            Error
        };

        struct LevelStyle
        {
            const char* label; // padded to 5 chars, e.g. "WARN "
            const char* icon;  // single codepoint, UTF-8 encoded
            const char* color; // ANSI SGR parameter, e.g. "31" for red
        };

        constexpr LevelStyle styleFor(Level level)
        {
            switch (level) {
                case Level::Info:    return {"INFO ", "ℹ", "36"}; // ℹ cyan
                case Level::Debug:   return {"DEBUG", "◦", "90"}; // ◦ gray
                case Level::Warning: return {"WARN ", "⚠", "33"}; // ⚠ yellow
                case Level::Error:   return {"ERROR", "✖", "31"}; // ✖ red
            }
            return {"", "", ""};
        }

#ifdef _WIN32
        // Classic Windows consoles don't interpret ANSI escape codes unless
        // virtual terminal processing is explicitly enabled for the handle.
        void enableAnsiOnWindows(DWORD handleId)
        {
            const HANDLE handle = GetStdHandle(handleId);
            DWORD mode = 0;
            if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode))
                SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#endif

        // Colors are enabled when writing to an interactive terminal, unless
        // the caller opted out via the NO_COLOR convention (see no-color.org).
        bool computeColorEnabled(FILE* stream)
        {
            if (std::getenv("NO_COLOR") != nullptr)
                return false;

#ifdef _WIN32
            enableAnsiOnWindows(stream == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
#endif
            return isatty(fileno(stream)) != 0;
        }

        bool colorEnabledForStdout()
        {
            static const bool enabled = computeColorEnabled(stdout);
            return enabled;
        }

        bool colorEnabledForStderr()
        {
            static const bool enabled = computeColorEnabled(stderr);
            return enabled;
        }

        std::string colorize(const char* sgr, const std::string& text, bool enabled)
        {
            if (!enabled)
                return text;
            return std::string("\x1b[") + sgr + "m" + text + "\x1b[0m";
        }

        // Number of terminal columns `text` occupies, treating each Unicode
        // codepoint as one column. Good enough for the ASCII/symbol content
        // used here; doesn't account for wide (e.g. CJK) or combining glyphs.
        size_t visibleWidth(const std::string& text)
        {
            return utf8ToUtf32(text).size();
        }

        std::string timestamp()
        {
            using namespace std::chrono;

            const auto now = system_clock::now();
            const auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
            const std::time_t t = system_clock::to_time_t(now);

            std::tm local {};
#ifdef _WIN32
            localtime_s(&local, &t);
#else
            localtime_r(&t, &local);
#endif
            return std::format("{:02}:{:02}:{:02}.{:03}", local.tm_hour, local.tm_min, local.tm_sec, millis.count());
        }

        // Writes `message` to `stream`, prefixed with a timestamp, a
        // level icon/label. Multi-line messages have their continuation
        // lines indented so they line up under the first line's text.
        void writeLog(Level level, const std::string& message, std::ostream& stream, bool color)
        {
            const LevelStyle style = styleFor(level);
            const std::string ts = timestamp();

            const std::string plainPrefix = ts + "  " + style.icon + " " + style.label + "  ";
            const std::string coloredPrefix = ts + "  " + colorize(style.color, std::string(style.icon) + " " + style.label, color) + "  ";
            const std::string indent(visibleWidth(plainPrefix), ' ');

            size_t start = 0;
            bool firstLine = true;
            while (true) {
                const size_t pos = message.find('\n', start);
                const std::string line = message.substr(start, pos == std::string::npos ? std::string::npos : pos - start);

                stream << (firstLine ? coloredPrefix : indent) << line << '\n';

                firstLine = false;
                if (pos == std::string::npos)
                    break;
                start = pos + 1;
            }
        }
    } // namespace

    void info(const std::string& message) { writeLog(Level::Info, message, std::cout, colorEnabledForStdout()); }
    void debug(const std::string& message) { writeLog(Level::Debug, message, std::cout, colorEnabledForStdout()); }
    void warning(const std::string& message) { writeLog(Level::Warning, message, std::cout, colorEnabledForStdout()); }
    void error(const std::string& message) { writeLog(Level::Error, message, std::cerr, colorEnabledForStderr()); }

    void logBox(const std::vector<std::string>& lines)
    {
        if (lines.empty())
            return;

        size_t maxWidth = 0;
        for (const auto& line : lines)
            maxWidth = std::max(maxWidth, visibleWidth(line));

        const size_t contentWidth = maxWidth + 4; // 2 columns of padding on each side
        const bool color = colorEnabledForStdout();

        std::string horizontal;
        for (size_t i = 0; i < contentWidth; ++i)
            horizontal += "─"; // ─

        const std::string bar = colorize("2", "│", color); // │, dimmed

        std::cout << colorize("2", "┌" + horizontal + "┐", color) << '\n'; // ┌...┐
        for (const auto& line : lines) {
            const std::string padding(maxWidth - visibleWidth(line), ' ');
            std::cout << bar << "  " << line << padding << "  " << bar << '\n';
        }
        std::cout << colorize("2", "└" + horizontal + "┘", color) << '\n'; // └...┘
    }
} // namespace p5cpp
