#pragma once

#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/texture.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/graphics/draw_buffer_writer.hpp>
#include <p5cpp/graphics/framebuffer.hpp>
#include <p5cpp/graphics/blendmode.hpp>

namespace p5cpp
{
    struct NativeRenderer
    {
        static std::unique_ptr<NativeRenderer> create(size_t vertexCount, size_t indexCount);

        virtual ~NativeRenderer() = default;

        virtual void begin(const Framebuffer& framebuffer) = 0;
        virtual void end() = 0;
        virtual void flush() = 0;

        virtual void submit(DrawBufferWriter& scope, UniformCache& uniformCache, const Shader& shader, const BlendMode& blendMode, const Texture& texture) = 0;

        virtual DrawBufferWriter& getDrawScope() = 0;
    };
} // namespace p5cpp
