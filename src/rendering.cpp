#include "rendering.hpp"
#include "canvas.hpp"
#include "drawscope.hpp"
#include "p5.hpp"
#include "uniform_cache.hpp"

namespace p5
{
    static void setBlendMode(BlendMode mode)
    {
        // clang-format off
        switch (mode) {
            case BlendMode::none: glDisable(GL_BLEND); break;
            case BlendMode::alpha: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
            case BlendMode::additive: glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
            case BlendMode::multiply: glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR, GL_ZERO); break;
        }
        // clang-format on
    }
} // namespace p5

namespace p5
{
    Renderer renderer_create(size_t vertexCount, size_t indexCount)
    {
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        GLuint ebo = 0;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texIndex));

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        static constexpr uint8_t whitePixel[] = {255, 255, 255, 255};
        GLuint whiteTexture = 0;
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        return Renderer {
            .vao = vao,
            .vbo = vbo,
            .ebo = ebo,
            .whiteTexture = whiteTexture
        };
    }

    void renderer_begin_frame(Renderer& renderer)
    {
        glBindVertexArray(renderer.vao);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
    }

    void renderer_end_frame(Renderer& renderer, DrawBuffer& drawBuffer)
    {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    static void render_canvas(Renderer& renderer, UniformCache& cache, const Canvas& canvas)
    {
        const uint2 canvasSize = canvas.framebuffer->getSize();           // logical (for projection)
        const uint2 viewportSize = canvas.framebuffer->getViewportSize(); // physical pixels (for glViewport)
        const matrix4x4 orthoProjection = ortho(0.0f, static_cast<float>(canvasSize.y), static_cast<float>(canvasSize.x), 0.0f, -1.0f, 1.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, canvas.framebuffer->getRendererId());
        glViewport(0, 0, viewportSize.x, viewportSize.y);

        for (const DrawCommand& drawCall : canvas.drawCommands) {
            setBlendMode(drawCall.blendMode);

            for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, drawCall.textureUnits[i]);
            }

            ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(cache, drawCall.shader.get());
            glUseProgram(drawCall.shader->getRendererId());
            for (const UniformSnapshot& uniform : drawCall.uniforms) {
                const UniformVariable& entry = uniform.variable;
                const int location = shader_uniform_cache_get_location(shaderCache, uniform.name, [&](const std::string& name) {
                    return glGetUniformLocation(drawCall.shader->getRendererId(), name.c_str());
                });

                switch (entry.type) {
                    case UniformVariable::Type::float1: glUniform1f(location, entry.floatValue); break;
                    case UniformVariable::Type::float2: glUniform2f(location, entry.float2Value.x, entry.float2Value.y); break;
                    case UniformVariable::Type::float4: glUniform4f(location, entry.float4Value.x, entry.float4Value.y, entry.float4Value.z, entry.float4Value.w); break;
                    case UniformVariable::Type::matrix4x4: glUniformMatrix4fv(location, 1, GL_FALSE, entry.matrix4x4Value.m.data()); break;
                }
            }

            static constexpr int samplers[] = {0, 1, 2, 3, 4, 5, 6, 7};
            if (const GLint samplersLocation = drawCall.shader->getUniformLocation("u_Textures"); samplersLocation != -1) {
                glUniform1iv(samplersLocation, static_cast<GLsizei>(drawCall.textureUnitCount), samplers);
            }

            if (const GLint projectionMatrixLocation = drawCall.shader->getUniformLocation("u_ProjectionMatrix"); projectionMatrixLocation != -1) {
                glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, orthoProjection.m.data());
            }

            glDrawElements(GL_TRIANGLES, drawCall.drawBufferIndexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(drawCall.drawBufferIndexStart * sizeof(uint32_t)));
            // glDrawElements(GL_LINES, drawCall.drawBufferIndexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(drawCall.drawBufferIndexStart * sizeof(uint32_t)));
        }
    }

    void renderer_flush(Renderer& renderer, UniformCache& cache, Canvas& canvas, DrawBuffer& drawBuffer)
    {
        if (drawBuffer.vertexCursor == 0 && canvas.drawCommands.empty())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, drawBuffer.vertexCursor * sizeof(Vertex), drawBuffer.vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, drawBuffer.indexCursor * sizeof(uint32_t), drawBuffer.indices.get());

        render_canvas(renderer, cache, canvas);

        // Clear the already-rendered commands so a subsequent flush on the same
        // canvas does not re-render stale draw commands against a fresh buffer.
        canvas.drawCommands.clear();
        draw_buffer_clear(drawBuffer);
    }
} // namespace p5
