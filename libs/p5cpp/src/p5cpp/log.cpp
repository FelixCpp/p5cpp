#include <p5cpp/p5cpp.hpp>
#include <ctime>

namespace p5::detail
{
    inline static constexpr std::string_view infoColor = "\033[32m";
    inline static constexpr std::string_view warnColor = "\033[33m";
    inline static constexpr std::string_view errorColor = "\033[31m";
    inline static constexpr std::string_view resetColor = "\033[0m";
    inline static constexpr std::string_view traceColor = "\033[36m";

    const char* getCurrentTimestamp()
    {
        static char buffer[256];
        std::time_t now = std::time(nullptr);
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buffer;
    }

    void log(FILE* file, std::string_view message, std::string_view color)
    {
        // p5::detail::logTrace/logInfo/logWarn/logError are public and callable directly with any
        // string_view (the error()/info()/... macros always hand them a nullterminated std::string
        // from std::format(), but a caller going through these functions directly isn't guaranteed
        // to). string_view::data() has no null-termination guarantee, so a substr() or other
        // non-owning slice without a trailing '\0' would make %s's data() read past message's end.
        // "%.*s" bounds the read to message's actual length instead.
        fprintf(file, "%s%s [p5cpp] %.*s%s\n", color.data(), getCurrentTimestamp(), static_cast<int>(message.size()), message.data(), resetColor.data());
        fflush(file);
    }

    void logTrace(std::string_view message) { log(stdout, message, traceColor); }
    void logInfo(std::string_view message) { log(stdout, message, infoColor); }
    void logWarn(std::string_view message) { log(stderr, message, warnColor); }
    void logError(std::string_view message) { log(stderr, message, errorColor); }
} // namespace p5::detail
