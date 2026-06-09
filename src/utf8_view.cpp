#include "utf8_view.hpp"

namespace p5
{
    // Decodes the next UTF-8 codepoint from sv starting at byte index pos.
    // Advances pos past all consumed bytes.
    static char32_t utf8NextCodepoint(std::string_view text, size_t& i)
    {
        const unsigned char byte = static_cast<unsigned char>(text[i]);

        uint32_t codepoint;
        int extraBytes;

        if (byte < 0x80) { // 0xxxxxxx → 1 Byte (ASCII)
            codepoint = byte;
            extraBytes = 0;
        } else if (byte < 0xE0) { // 110xxxxx → 2 Bytes
            codepoint = byte & 0x1F;
            extraBytes = 1;
        } else if (byte < 0xF0) { // 1110xxxx → 3 Bytes
            codepoint = byte & 0x0F;
            extraBytes = 2;
        } else { // 11110xxx → 4 Bytes
            codepoint = byte & 0x07;
            extraBytes = 3;
        }

        for (int j = 0; j < extraBytes; ++j) {
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[++i]) & 0x3F);
        }

        ++i;
        return static_cast<char32_t>(codepoint);
    }

    std::u32string utf8ToUtf32(std::string_view text)
    {
        std::u32string result;
        size_t i = 0;

        while (i < text.size()) {
            result.push_back(utf8NextCodepoint(text, i));
        }

        return result;
    }
} // namespace p5
