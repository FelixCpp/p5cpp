#include "renderer.hpp"
#include "../vertex.hpp"

#include <glad/glad.h>

namespace p5cpp
{
    void DrawScope::pushVertex(const float2& position, const float2& texcoord, const float4& color)
    {
        if (vertexCursor >= vertices.size()) {
            // Buffer is full; the caller should have called flushIfNeeded() before
            // creating this scope.  Silently drop the vertex to prevent an OOB write.
            return;
        }

        vertices[vertexCursor++] = Vertex {
            .position = position,
            .texcoord = texcoord,
            .color = color,
            .texIndex = 0.0f, // Texture index will be assigned later by the renderer based on the current batch state
        };
    }

    void DrawScope::pushTriangle(uint32_t a, uint32_t b, uint32_t c)
    {
        if (indexCursor + 2 >= indices.size()) {
            // Buffer is full; silently drop to prevent an OOB write.
            return;
        }

        indices[indexCursor++] = baseVertex + a;
        indices[indexCursor++] = baseVertex + b;
        indices[indexCursor++] = baseVertex + c;
    }
} // namespace p5cpp

namespace p5cpp
{
    void GlobalUniformCache::setUniform(Shader* shader, const std::string& name, const UniformVariable& variable)
    {
        const int32_t location = shader->getUniformLocation(name);
        if (location == -1) {
            return;
        }

        const UniformSnapshot newSnapshot {
            .location = location,
            .variable = variable,
        };

        const auto insertion = uniformsByShader.try_emplace(
            shader,
            std::vector<UniformSnapshot> {newSnapshot}
        );

        const bool hasBeenInserted = insertion.second;
        if (not hasBeenInserted) {
            std::vector<UniformSnapshot>& snapshots = insertion.first->second;
            const auto variableItr = std::find_if(snapshots.begin(), snapshots.end(), [location](const UniformSnapshot& snapshot) {
                return snapshot.location == location;
            });

            const bool variableExists = variableItr != snapshots.end();
            if (variableExists) {
                variableItr->variable = variable;
            } else {
                snapshots.push_back(newSnapshot);
            }
        }

        dirtyShaders.insert(shader);
    }

    void GlobalUniformCache::markShaderClean(Shader* shader)
    {
        dirtyShaders.erase(shader);
    }

    bool GlobalUniformCache::isShaderDirty(Shader* shader) const
    {
        return dirtyShaders.contains(shader);
    }

    std::vector<UniformSnapshot> GlobalUniformCache::getUniforms(Shader* shader)
    {
        const auto itr = uniformsByShader.find(shader);
        if (itr != uniformsByShader.end()) {
            return itr->second;
        }

        return {};
    }
} // namespace p5cpp

namespace p5cpp
{
    struct DrawCommand
    {
        size_t drawBufferIndexStart;
        size_t drawBufferIndexCount;

        Shader* shader;
        BlendMode blendMode;

        std::array<Texture*, 8> textures;
        size_t textureCount;

        std::vector<UniformSnapshot> uniforms;
    };
} // namespace p5cpp

namespace p5cpp
{
    class OpenGLRenderer : public Renderer
    {
    public:
        static std::unique_ptr<OpenGLRenderer> create(size_t vertexCount, size_t indexCount)
        {
            GLuint vbo = 0;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

            GLuint ebo = 0;
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

            GLuint vao = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

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

            return std::unique_ptr<OpenGLRenderer>(new OpenGLRenderer(vao, vbo, ebo, vertexCount, indexCount));
        }

        ~OpenGLRenderer() override
        {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }

        void begin(Framebuffer* framebuffer) override
        {
            const uint2 logicalSize = framebuffer->getSize();
            orthoProjection = ortho(0.0f, 0.0f, static_cast<float>(logicalSize.x), static_cast<float>(logicalSize.y), -1.0f, 1.0f);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->getRendererId());
            glViewport(0, 0, logicalSize.x, logicalSize.y);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        }

        void end() override
        {
            flush();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        void flush() override
        {
            if (drawCommands.empty()) {
                return;
            }

            if (vertexCursor == 0 or indexCursor == 0) {
                return;
            }

            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCursor * sizeof(Vertex), vertices.data());
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexCursor * sizeof(uint32_t), indices.data());

            for (const DrawCommand& command : drawCommands) {
                activateBlendMode(command.blendMode);

                glUseProgram(command.shader->getRendererId());
                for (const UniformSnapshot& snapshot : command.uniforms) {
                    switch (snapshot.variable.type) {
                        case UniformVariable::Type::float1: glUniform1f(snapshot.location, snapshot.variable.floatValue); break;
                        case UniformVariable::Type::float2: glUniform2f(snapshot.location, snapshot.variable.float2Value.x, snapshot.variable.float2Value.y); break;
                        case UniformVariable::Type::float4: glUniform4f(snapshot.location, snapshot.variable.float4Value.x, snapshot.variable.float4Value.y, snapshot.variable.float4Value.z, snapshot.variable.float4Value.w); break;
                        case UniformVariable::Type::matrix4x4: glUniformMatrix4fv(snapshot.location, 1, GL_FALSE, snapshot.variable.matrix4x4Value.m.data()); break;
                    }
                }

                static constexpr GLint samplers[] = {0, 1, 2, 3, 4, 5, 6, 7};
                glUniform1iv(command.shader->getUniformLocation("u_Textures"), static_cast<GLsizei>(command.textureCount), samplers);
                glUniformMatrix4fv(command.shader->getUniformLocation("u_ProjectionMatrix"), 1, GL_FALSE, orthoProjection.m.data());

                for (size_t i = 0; i < command.textureCount; ++i) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, command.textures[i]->getRendererId());
                }

                glDrawElements(GL_TRIANGLES, command.drawBufferIndexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(command.drawBufferIndexStart * sizeof(uint32_t)));
            }

            drawCommands.clear();
            vertexCursor = 0;
            indexCursor = 0;
        }

        void submit(DrawScope scope, GlobalUniformCache& uniformCache, Shader* shader, BlendMode blendMode, Texture* texture) override
        {
            DrawCommand* targetCommand = nullptr;

            if (drawCommands.empty()) {
                targetCommand = &drawCommands.emplace_back(DrawCommand {
                    .drawBufferIndexStart = scope.baseIndex,
                    .drawBufferIndexCount = 0,
                    .shader = shader,
                    .blendMode = blendMode,
                    .textures = {texture},
                    .textureCount = 1,
                    .uniforms = uniformCache.getUniforms(shader),
                });

                uniformCache.markShaderClean(shader);
            } else {
                const DrawCommand& lastCommand = drawCommands.back();

                bool needsNewCommand = false;
                if (lastCommand.shader != shader) {
                    needsNewCommand = true;
                } else if (lastCommand.blendMode != blendMode) {
                    needsNewCommand = true;
                } else if (uniformCache.isShaderDirty(shader)) {
                    needsNewCommand = true;
                } else {
                    if (lastCommand.textureCount < lastCommand.textures.size()) {
                        needsNewCommand = false;
                    } else {
                        bool textureAlreadyUsed = false;
                        for (size_t i = 0; i < lastCommand.textureCount; ++i) {
                            if (lastCommand.textures[i] == texture) {
                                textureAlreadyUsed = true;
                                break;
                            }
                        }

                        needsNewCommand = not textureAlreadyUsed;
                    }
                }

                if (needsNewCommand) {
                    targetCommand = &drawCommands.emplace_back(DrawCommand {
                        .drawBufferIndexStart = scope.baseIndex,
                        .drawBufferIndexCount = 0,
                        .shader = shader,
                        .blendMode = blendMode,
                        .textures = {texture},
                        .textureCount = 1,
                        .uniforms = uniformCache.getUniforms(shader),
                    });

                    uniformCache.markShaderClean(shader);
                } else {
                    targetCommand = &drawCommands.back();
                }
            }

            targetCommand->drawBufferIndexCount += scope.indexCursor - scope.baseIndex;
            targetCommand->shader = shader;
            targetCommand->blendMode = blendMode;

            int textureIndex = -1;
            for (size_t i = 0; i < targetCommand->textureCount; ++i) {
                if (targetCommand->textures[i] == texture) {
                    textureIndex = static_cast<int>(i);
                    break;
                }
            }

            if (textureIndex == -1) {
                textureIndex = static_cast<int>(targetCommand->textureCount);
                targetCommand->textures[textureIndex] = texture;
                targetCommand->textureCount++;
            }

            const size_t vertexCount = scope.vertexCursor - scope.baseVertex;
            for (size_t i = 0; i < vertexCount; ++i) {
                vertices[scope.baseVertex + i].texIndex = static_cast<float>(textureIndex);
            }
        }

        DrawScope getDrawScope() override
        {
            return DrawScope {
                .baseIndex = indexCursor,
                .baseVertex = vertexCursor,
                .indexCursor = indexCursor,
                .vertexCursor = vertexCursor,
                .maxVertexCount = vertices.size(),
                .maxIndexCount = indices.size(),
                .vertices = vertices,
                .indices = indices,
            };
        }

    private:
        explicit OpenGLRenderer(GLuint vao, GLuint vbo, GLuint ebo, size_t vertexCount, size_t indexCount)
            : vao(vao), vbo(vbo), ebo(ebo), vertices(vertexCount), indices(indexCount), vertexCursor(0), indexCursor(0), orthoProjection(matrix4x4::identity)
        {
        }

        static void activateBlendMode(BlendMode blendMode)
        {
            switch (blendMode) {
                case BlendMode::none: glDisable(GL_BLEND); break;
                case BlendMode::alpha:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                case BlendMode::additive:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    break;
                case BlendMode::multiply:
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_DST_COLOR, GL_ZERO);
                    break;
            }
        }

        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        size_t vertexCursor;
        size_t indexCursor;
        matrix4x4 orthoProjection;

        std::vector<DrawCommand> drawCommands;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<Renderer> Renderer::create(size_t vertexCount, size_t indexCount)
    {
        return OpenGLRenderer::create(vertexCount, indexCount);
    }
} // namespace p5cpp
