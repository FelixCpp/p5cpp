#pragma once

#include <p5cpp/graphics/color.hpp>
#include <p5cpp/graphics/pixels.hpp>
#include <p5cpp/math/value2.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace p5cpp
{
    // Controls how the sx/sy/sWidth/sHeight source-rect arguments of image() (and the
    // u/v arguments of vertex()) are interpreted: normalized expects the standard 0..1
    // UV range, image expects pixel coordinates/extents of the source texture. Both use
    // a top-left origin, matching the rest of p5cpp's pixel coordinate conventions (see
    // Pixels) regardless of the texture's actual bottom-to-top GL storage order.
    enum class TextureMode {
        normalized,
        image,
    };

    // Forward-declared so saveTexture(path, const Framebuffer&) below can be declared
    // without including framebuffer.hpp, which itself includes this header.
    class Framebuffer;

    struct TextureId
    {
        uint32_t value;

        constexpr bool operator==(const TextureId& other) const = default;
        constexpr bool operator!=(const TextureId& other) const = default;
    };

    // rgba8: regular 4-byte-per-pixel color texture (loadTexture()).
    // r8: single-channel 1-byte-per-pixel texture, used internally for the font glyph
    // atlas (see detail::makeGlyphAtlasTexture()) - not expected to be created by sketch
    // authors directly.
    enum class TextureFormat {
        rgba8,
        r8,
    };

    class Texture;

    namespace detail
    {
        struct TextureResource;

        // Zero-initialized single-channel (r8) atlas texture for Font's glyph atlas -
        // internal use only, not part of the sketch-author API.
        Texture makeGlyphAtlasTexture(uint32_t width, uint32_t height);
    }

    // See Texture::upload() — `data` must already be in bottom-to-top row order.
    // Returns an invalid Texture on failure.
    Texture loadTexture(uint32_t width, uint32_t height, const color_t* data);

    // A GPU texture handle. Copies are cheap and alias the same underlying GL texture
    // (shared_ptr-backed); the GL texture is deleted automatically once the last copy
    // is destroyed. Default-constructed instances are "invalid" (getTextureId().value
    // == 0, getSize() == {0, 0}) - see isValid().
    class Texture
    {
    public:
        Texture();

        bool isValid() const;
        TextureId getTextureId() const;
        uint2 getSize() const;
        TextureFormat getFormat() const;

        // Expects the same raw GL row order (bottom-to-top) as Framebuffer::writePixels()
        // and the data loadTexture(path)/loadTexture(span<uint8_t>) produce — row 0 of
        // `data` becomes v=0 (bottom) of the texture. Callers holding top-left-origin
        // pixel data (e.g. row 0 == y == 0) must flip it before uploading, or sampling
        // will be vertically mirrored relative to images loaded via loadTexture() or
        // textures read back from a Framebuffer.
        // Pure convenience for the common rgba8 case: equivalent to
        // updateRegion(0, 0, size.x, size.y, <data reinterpreted as raw bytes>).
        void upload(std::span<const color_t> data);

        // Uploads a sub-region of raw bytes (glTexSubImage2D). Byte layout depends on
        // getFormat(): rgba8 expects 4 bytes/pixel, r8 expects 1 byte/pixel. Same
        // bottom-to-top row order as upload().
        void updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::span<const uint8_t> rawBytes);

        // Reads the texture's current GPU content back as a top-left-origin Pixels
        // buffer, mirroring Processing's PImage.loadPixels()/.pixels[]. rgba8 only -
        // r8 (the font glyph atlas format) returns an empty Pixels, since its
        // GL_TEXTURE_SWIZZLE_RGBA doesn't apply to glGetTexImage the way it does to
        // shader sampling, so a raw readback wouldn't match what's visually rendered.
        Pixels loadPixels() const;

        // Uploads a top-left-origin Pixels buffer (Processing's PImage.updatePixels()).
        // pixels' size is expected to match getSize() - same trust-the-caller contract
        // as upload(). rgba8 only, see loadPixels().
        void updatePixels(const Pixels& pixels);

    private:
        explicit Texture(std::shared_ptr<detail::TextureResource> resource, uint2 size, TextureFormat format);

        friend Texture loadTexture(uint32_t width, uint32_t height, const color_t* data);
        friend Texture detail::makeGlyphAtlasTexture(uint32_t width, uint32_t height);

        std::shared_ptr<detail::TextureResource> m_resource;
        uint2 m_size;
        TextureFormat m_format;
    };
} // namespace p5cpp

namespace p5cpp
{
    // Decodes a PNG/JPG/BMP/... file into a GPU texture. Returns an invalid Texture
    // (see Texture::isValid()) on failure.
    Texture loadTexture(const std::filesystem::path& imageFilePath);
    Texture loadTexture(std::span<const uint8_t> imageData);

    // Encodes to PNG. Returns false on failure.
    bool saveTexture(const std::filesystem::path& imageFilePath, const Framebuffer& framebuffer);
    bool saveTexture(const std::filesystem::path& imageFilePath, uint32_t width, uint32_t height, std::span<const color_t> pixels);
} // namespace p5cpp
