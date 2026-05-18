#pragma once

#include "drawscope.hpp"
#include "canvas.hpp"

#include <glad/glad.h>

namespace p5
{
    struct Renderer
    {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        GLuint whiteTexture;
    };

    Renderer renderer_create(size_t vertexCount, size_t indexCount);
    void renderer_begin_frame(Renderer& renderer);
    void renderer_end_frame(Renderer& renderer, DrawBuffer& drawBuffer);
    void renderer_flush(Renderer& renderer, Canvas& stack, DrawBuffer& drawBuffer);
} // namespace p5
