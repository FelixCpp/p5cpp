#pragma once

#include "renderpassstack.hpp"
#include "drawscope.hpp"

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
    void renderer_end_frame(Renderer& renderer, RenderPassStack& stack, DrawBuffer& drawBuffer);
    void renderer_flush(Renderer& renderer, RenderPassStack& stack, DrawBuffer& drawBuffer);
} // namespace p5
