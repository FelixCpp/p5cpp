#pragma once

#include <p5cpp/math/rectangle.hpp>

#include <cstddef>
#include <vector>

namespace p5cpp
{
    // A single visually-wrapped line within a TextLayout. Coordinates live in the
    // same space as the (x, y) anchor passed to textLayout()/text().
    struct TextLineLayout
    {
        float width;     // shaped width of this line, in pixels
        float x;          // left edge of this line, after horizontal alignment
        float baselineY;  // y coordinate of this line's baseline
    };

    // Metadata describing how text(text, x, y[, maxWidth]) would lay the text out,
    // without submitting any draw calls. Reflects the current textFont()/textSize()/
    // textAlign()/textWrap()/textLetterSpacing()/textLineSpacing() state at the time
    // textLayout() was called. Useful for positioning, alignment, or drawing your own
    // decorations (backgrounds, underlines, carets) around text.
    struct TextLayout
    {
        std::vector<TextLineLayout> lines;
        float width = 0.0f;      // width of the widest line, in pixels
        float height = 0.0f;     // total height spanned by all lines, in pixels
        float ascender = 0.0f;
        float descender = 0.0f;
        float lineHeight = 0.0f;
        float_rect bounds;        // axis-aligned bounding box, in the same coordinate space as (x, y)

        size_t lineCount() const { return lines.size(); }
    };
} // namespace p5cpp
