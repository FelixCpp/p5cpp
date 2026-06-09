#include "renderer.hpp"

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
    static void render_canvas(Renderer& renderer)
    {
        const uint2 canvasSize = renderer.framebuffer->getSize();           // logical (for projection)
        const uint2 viewportSize = renderer.framebuffer->getViewportSize(); // physical pixels (for glViewport)
        const matrix4x4 orthoProjection = ortho(0.0f, static_cast<float>(canvasSize.y), static_cast<float>(canvasSize.x), 0.0f, -1.0f, 1.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, renderer.framebuffer->getRendererId());
        glViewport(0, 0, viewportSize.x, viewportSize.y);

        for (const DrawCommand& drawCall : renderer.drawCommands) {
            setBlendMode(drawCall.blendMode);

            for (size_t i = 0; i < drawCall.textureUnitCount; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, drawCall.textureUnits[i]);
            }

            ShaderUniformCache& shaderCache = uniform_cache_get_shader_cache(renderer.uniformCache, drawCall.shader.get());
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
        }

        draw_commands_clear(renderer.drawCommands);
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

        return Renderer {
            .vao = vao,
            .vbo = vbo,
            .ebo = ebo,
            .drawBuffer = draw_buffer_create(vertexCount, indexCount),
            .uniformCache = uniform_cache_create(),
            .drawCommands = {},
            .framebuffer = nullptr,
        };
    }

    void renderer_begin_frame(Renderer& renderer, std::shared_ptr<Framebuffer> targetFramebuffer)
    {
        renderer.framebuffer = std::move(targetFramebuffer);

        glBindVertexArray(renderer.vao);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
    }

    void renderer_end_frame(Renderer& renderer)
    {
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        renderer.framebuffer = nullptr;
    }

    void renderer_flush(Renderer& renderer)
    {
        if (renderer.framebuffer == nullptr)
            return;

        if (renderer.drawCommands.empty())
            return;

        if (renderer.drawBuffer.indexCursor == 0 or renderer.drawBuffer.vertexCursor == 0)
            return;

        glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, renderer.drawBuffer.vertexCursor * sizeof(Vertex), renderer.drawBuffer.vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, renderer.drawBuffer.indexCursor * sizeof(uint32_t), renderer.drawBuffer.indices.get());

        render_canvas(renderer);
        draw_buffer_clear(renderer.drawBuffer);
    }
} // namespace p5

namespace p5
{
    void renderer_submit(Renderer& renderer, const DrawScope& scope, std::shared_ptr<Shader> shader, BlendMode blendMode, uint32_t texture)
    {
        draw_commands_submit(renderer.drawCommands, renderer.uniformCache, scope, std::move(shader), blendMode, texture);
    }
} // namespace p5
