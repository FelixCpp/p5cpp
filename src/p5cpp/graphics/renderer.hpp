#pragma once

#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/draw_buffer_writer.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/blendmode.hpp>

#include <span>

namespace p5cpp
{
    struct NativeRenderer
    {
        static std::unique_ptr<NativeRenderer> create(size_t vertexCount, size_t indexCount);

        virtual ~NativeRenderer() = default;

        virtual void begin(const Framebuffer& framebuffer) = 0;
        virtual void end() = 0;
        virtual void flush() = 0;

        // `uniforms` is applied verbatim for this draw call — callers pass either the
        // live UniformCache's current snapshot for `shader`, or a snapshot frozen at an
        // earlier time (e.g. a recorded RenderGroup op), decoupled from whatever the
        // live cache holds now.
        virtual void submit(DrawBufferWriter& scope, std::span<const UniformSnapshot> uniforms, const Shader& shader, const BlendMode& blendMode, const Texture& texture) = 0;

        virtual DrawBufferWriter& getDrawScope() = 0;
    };
} // namespace p5cpp
