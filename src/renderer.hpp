#pragma once

#include <p5cpp.hpp>
#include <glad/glad.h>

#include "vertex.hpp"
#include "draw_scope.hpp"
#include "uniform_cache.hpp"
#include "draw_command_list.hpp"

namespace p5cpp
{
    struct DrawBuffer
    {
        std::unique_ptr<Vertex[]> vertices;
        std::unique_ptr<uint32_t[]> indices;

        size_t vertexCount;
        size_t indexCount;

        size_t vertexCursor;
        size_t indexCursor;
    };

    DrawBuffer draw_buffer_create(size_t maxVertices, size_t maxIndices);
    DrawScope draw_buffer_get_scope(DrawBuffer& buffer);
    void draw_buffer_clear(DrawBuffer& buffer);
} // namespace p5cpp

namespace p5cpp
{
    struct Renderer
    {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        DrawBuffer drawBuffer;
        UniformCache uniformCache;
        DrawCommandList drawCommands;

        std::shared_ptr<Framebuffer> framebuffer;
    };

    Renderer renderer_create(size_t vertexCount, size_t indexCount);
    void renderer_begin_frame(Renderer& renderer, std::shared_ptr<Framebuffer> targetFramebuffer);
    void renderer_end_frame(Renderer& renderer);
    void renderer_flush(Renderer& renderer);
    void renderer_submit(Renderer& renderer, const DrawScope& scope, Shader* shader, BlendMode blendMode, uint32_t texture);
} // namespace p5cpp
