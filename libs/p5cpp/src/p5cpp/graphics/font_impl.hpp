#pragma once

#include <p5cpp/p5cpp.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb.h>

#include <optional>
#include <unordered_map>
#include <vector>

namespace p5
{
    struct FontImpl
    {
        FontImpl(FT_Face rasterFace, FT_Face hbFace, hb_font_t* hbFont, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t atlasEmPixels);
        FontImpl(const FontImpl&) = delete;
        FontImpl& operator=(const FontImpl&) = delete;
        ~FontImpl();

        std::vector<ShapedGlyph> shape(std::string_view utf8Text, bool ligaturesEnabled) const;
        const GlyphMetrics& getGlyphMetrics(uint32_t glyphIndex);
        std::vector<std::vector<float2>> getGlyphContours(uint32_t glyphIndex);
        Texture getAtlasTexture() const;
        float getUnitsPerEm() const;
        float getAscent() const;
        float getDescent() const;
        float getLineGap() const;

    private:
        const GlyphMetrics& rasterizeGlyph(uint32_t glyphIndex);
        std::optional<rect2f> packIntoAtlas(const std::vector<uint8_t>& cellCoverage, int cellWidth, int cellHeight);
        std::vector<std::vector<float2>> decomposeGlyphOutline(uint32_t glyphIndex);

        FT_Face m_rasterFace;
        FT_Face m_hbFace;
        hb_font_t* m_hbFont;
        Texture m_atlasTexture;
        uint32_t m_atlasEmPixels;
        std::unordered_map<uint32_t, GlyphMetrics> m_glyphCache;
        std::unordered_map<uint32_t, std::vector<std::vector<float2>>> m_outlineCache;

        uint32_t m_shelfX = 0;
        uint32_t m_shelfY = 0;
        uint32_t m_shelfHeight = 0;
    };
} // namespace p5
