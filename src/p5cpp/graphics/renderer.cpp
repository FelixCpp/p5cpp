#include <p5cpp/graphics/shader.hpp>
#include <p5cpp/graphics/renderer.hpp>
#include <p5cpp/graphics/uniform_cache.hpp>
#include <p5cpp/math/matrix4x4.hpp>

#include <glad/glad.h>

#include <array>
#include <cassert>
#include <cstring>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace p5cpp
{
    struct RenderVertex
    {
        float2 position;
        float2 texcoord;
        float4 color;
        float textureSlot;
    };
} // namespace p5cpp

namespace p5cpp
{
    class ConcreteDrawBufferWriter : public DrawBufferWriter
    {
    public:
        void init(RenderVertex* vertices, uint32_t* indices, size_t maxVertices, size_t maxIndices)
        {
            m_vertices = vertices;
            m_indices = indices;
            m_maxVertices = maxVertices;
            m_maxIndices = maxIndices;
            reset();
        }

        void reset()
        {
            m_vertexCount = 0;
            m_indexCount = 0;
            m_submitVertexBase = 0;
            m_submitIndexBase = 0;
        }

        // After flushing submitted batches mid-frame, relocate any in-progress (not-yet-submitted)
        // vertices and indices to the start of the buffer so writes can continue.
        void resetPreservingPending()
        {
            const size_t pendingVerts = m_vertexCount - m_submitVertexBase;
            const size_t pendingIndices = m_indexCount - m_submitIndexBase;

            if (pendingVerts > 0)
                std::memmove(m_vertices, m_vertices + m_submitVertexBase, pendingVerts * sizeof(RenderVertex));

            for (size_t i = 0; i < pendingIndices; ++i)
                m_indices[i] = m_indices[m_submitIndexBase + i] - static_cast<uint32_t>(m_submitVertexBase);

            m_vertexCount = pendingVerts;
            m_indexCount = pendingIndices;
            m_submitVertexBase = 0;
            m_submitIndexBase = 0;
        }

        uint32_t getRelativeCursor() const override
        {
            return static_cast<uint32_t>(m_vertexCount - m_submitVertexBase);
        }

        void pushVertex(const float2& position, const float2& texcoord, const float4& color) override
        {
            if (m_vertexCount >= m_maxVertices) {
                assert(m_submitVertexBase > 0 && "Single shape exceeds vertex buffer capacity");
                if (m_onOverflow) m_onOverflow();
            }
            m_vertices[m_vertexCount++] = RenderVertex {position, texcoord, color, 0.0f};
        }

        void pushTriangle(uint32_t a, uint32_t b, uint32_t c) override
        {
            if (m_indexCount + 3 > m_maxIndices) {
                assert(m_submitIndexBase > 0 && "Single shape exceeds index buffer capacity");
                if (m_onOverflow) m_onOverflow();
            }
            const uint32_t base = static_cast<uint32_t>(m_submitVertexBase);
            m_indices[m_indexCount++] = base + a;
            m_indices[m_indexCount++] = base + b;
            m_indices[m_indexCount++] = base + c;
        }

        size_t m_vertexCount = 0;
        size_t m_indexCount = 0;
        size_t m_submitVertexBase = 0;
        size_t m_submitIndexBase = 0;
        size_t m_maxVertices = 0;
        size_t m_maxIndices = 0;
        RenderVertex* m_vertices = nullptr;
        uint32_t* m_indices = nullptr;
        std::function<void()> m_onOverflow;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct BatchEntry
    {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        Shader shader;
        BlendMode blendMode = BlendMode::none;
        std::array<GLuint, 8> textures {};
        uint8_t textureCount = 0;
        std::vector<UniformSnapshot> uniforms;
    };
} // namespace p5cpp

namespace p5cpp
{
    class NativeOpenGLRenderer : public NativeRenderer
    {
    public:
        static std::unique_ptr<NativeOpenGLRenderer> create(size_t vertexCount, size_t indexCount)
        {
            return std::unique_ptr<NativeOpenGLRenderer>(new NativeOpenGLRenderer(vertexCount, indexCount));
        }

        ~NativeOpenGLRenderer() override
        {
            if (m_vao) glDeleteVertexArrays(1, &m_vao);
            if (m_vbo) glDeleteBuffers(1, &m_vbo);
            if (m_ebo) glDeleteBuffers(1, &m_ebo);
        }

        void begin(const Framebuffer& framebuffer) override
        {
            m_viewportSize = framebuffer.size;

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id.value);
            glViewport(0, 0, static_cast<GLsizei>(m_viewportSize.x), static_cast<GLsizei>(m_viewportSize.y));

            m_writer.reset();
            m_batches.clear();
        }

        void end() override
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void flush() override
        {
            renderBatches();
            m_writer.reset();
        }

        void submit(DrawBufferWriter& /*scope*/, std::span<const UniformSnapshot> uniforms, const Shader& shader, const BlendMode& blendMode, const Texture& texture) override
        {
            const size_t vertexStart = m_writer.m_submitVertexBase;
            const size_t vertexEnd = m_writer.m_vertexCount;
            const uint32_t indexStart = static_cast<uint32_t>(m_writer.m_submitIndexBase);
            const uint32_t indexEnd = static_cast<uint32_t>(m_writer.m_indexCount);
            const uint32_t indexCount = indexEnd - indexStart;

            m_writer.m_submitVertexBase = vertexEnd;
            m_writer.m_submitIndexBase = indexEnd;

            if (indexCount == 0)
                return;

            const GLuint textureId = texture.id.value;
            const GLuint programId = static_cast<GLuint>(shader.id.value);

            if (!m_batches.empty()) {
                BatchEntry& last = m_batches.back();
                const GLuint lastProgramId = static_cast<GLuint>(last.shader.id.value);

                if (lastProgramId == programId && last.blendMode == blendMode) {
                    uint8_t slot = 0;
                    bool found = false;
                    for (uint8_t i = 0; i < last.textureCount; ++i) {
                        if (last.textures[i] == textureId) {
                            slot = i;
                            found = true;
                            break;
                        }
                    }
                    if (!found && last.textureCount < 8) {
                        slot = last.textureCount;
                        last.textures[slot] = textureId;
                        last.textureCount++;
                        found = true;
                    }
                    if (found) {
                        const float slotF = static_cast<float>(slot);
                        for (size_t i = vertexStart; i < vertexEnd; ++i) {
                            m_vertices[i].textureSlot = slotF;
                        }
                        last.indexCount += indexCount;
                        return;
                    }
                }
            }

            BatchEntry entry;
            entry.indexOffset = indexStart;
            entry.indexCount = indexCount;
            entry.shader = shader;
            entry.blendMode = blendMode;
            entry.textures.fill(0);
            entry.textures[0] = textureId;
            entry.textureCount = 1;
            entry.uniforms.assign(uniforms.begin(), uniforms.end());

            for (size_t i = vertexStart; i < vertexEnd; ++i) {
                m_vertices[i].textureSlot = 0.0f;
            }

            m_batches.push_back(std::move(entry));
        }

        DrawBufferWriter& getDrawScope() override
        {
            return m_writer;
        }

    private:
        explicit NativeOpenGLRenderer(size_t vertexCount, size_t indexCount)
            : m_vertices(std::make_unique<RenderVertex[]>(vertexCount)),
              m_indices(std::make_unique<uint32_t[]>(indexCount)),
              m_maxVertices(vertexCount),
              m_maxIndices(indexCount)
        {
            m_writer.init(m_vertices.get(), m_indices.get(), vertexCount, indexCount);
            m_writer.m_onOverflow = [this]() {
                midFrameFlush();
            };

            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);
            glGenBuffers(1, &m_ebo);

            glBindVertexArray(m_vao);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexCount * sizeof(RenderVertex)), nullptr, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indexCount * sizeof(uint32_t)), nullptr, GL_DYNAMIC_DRAW);

            constexpr GLsizei stride = static_cast<GLsizei>(sizeof(RenderVertex));
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(RenderVertex, position)));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(RenderVertex, texcoord)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(RenderVertex, color)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(RenderVertex, textureSlot)));
            glEnableVertexAttribArray(3);

            glBindVertexArray(0);
        }

        // Renders all accumulated batches to the GPU and clears the batch list.
        // Uses m_submitVertexBase/m_submitIndexBase so that any in-progress (not-yet-submitted)
        // pending vertices/indices are not uploaded — only fully-submitted data is rendered.
        void renderBatches()
        {
            if (m_batches.empty())
                return;

            glBindVertexArray(m_vao);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_writer.m_submitVertexBase * sizeof(RenderVertex)), m_vertices.get());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_writer.m_submitIndexBase * sizeof(uint32_t)), m_indices.get());

            glEnable(GL_BLEND);

            const matrix4x4 projection = matrix4x4::ortho(0.0f, static_cast<float>(m_viewportSize.y), static_cast<float>(m_viewportSize.x), 0.0f, -1.0f, 1.0f);

            GLuint lastProgramId = 0;
            BlendMode lastBlendMode = BlendMode::none;
            bool firstBatch = true;

            for (const BatchEntry& batch : m_batches) {
                const GLuint programId = static_cast<GLuint>(batch.shader.id.value);
                if (programId == 0) continue;

                if (firstBatch || programId != lastProgramId) {
                    glUseProgram(programId);
                    lastProgramId = programId;

                    const auto projLoc = getUniformLocation(batch.shader, "u_ProjectionMatrix");
                    if (projLoc.has_value()) {
                        applyUniformCached(programId, *projLoc, uniform(projection));
                    }

                    const auto texLoc = getUniformLocation(batch.shader, "u_Textures");
                    if (texLoc.has_value() && !m_samplerArrayApplied.contains(programId)) {
                        const GLint samplers[8] = {0, 1, 2, 3, 4, 5, 6, 7};
                        glUniform1iv(texLoc->value, 8, samplers);
                        m_samplerArrayApplied.insert(programId);
                    }
                }

                for (const UniformSnapshot& snap : batch.uniforms) {
                    applyUniformCached(programId, snap.location, snap.variable);
                }

                if (firstBatch || !(batch.blendMode == lastBlendMode)) {
                    applyBlendMode(batch.blendMode);
                    lastBlendMode = batch.blendMode;
                }

                for (uint8_t i = 0; i < batch.textureCount; ++i) {
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, batch.textures[i]);
                }

                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void*>(static_cast<uintptr_t>(batch.indexOffset * sizeof(uint32_t))));
                firstBatch = false;
            }

            glBindVertexArray(0);
            m_batches.clear();
        }

        // Called when the writer detects an imminent buffer overflow mid-frame.
        // Renders all submitted batches to the GPU, then relocates any pending (not-yet-submitted)
        // vertices and indices to the start of the staging buffers so writing can continue.
        void midFrameFlush()
        {
            renderBatches();
            m_writer.resetPreservingPending();
        }

        // float3/float4/matrix4x4 don't have their own operator== (see math/value3.hpp,
        // value4.hpp, matrix4x4.hpp), so std::variant's built-in == can't be used here -
        // compare field-by-field instead, same as before the union->variant conversion.
        static bool uniformVariableEquals(const UniformVariable& a, const UniformVariable& b)
        {
            if (a.index() != b.index())
                return false;

            return std::visit([&](const auto& aValue) {
                using T = std::decay_t<decltype(aValue)>;
                const T& bValue = std::get<T>(b);
                if constexpr (std::is_same_v<T, float3>) {
                    return aValue.x == bValue.x and aValue.y == bValue.y and aValue.z == bValue.z;
                } else if constexpr (std::is_same_v<T, float4>) {
                    return aValue.x == bValue.x and aValue.y == bValue.y and aValue.z == bValue.z and aValue.w == bValue.w;
                } else if constexpr (std::is_same_v<T, matrix4x4>) {
                    return std::memcmp(aValue.data(), bValue.data(), 16 * sizeof(float)) == 0;
                } else {
                    return aValue == bValue; // float, float2, TextureUniformValue all have operator==
                }
            }, a);
        }

        // Skips the glUniform* call if the same value is already active for this program,
        // since uniform state lives on the GL program object and survives across batches/frames.
        void applyUniformCached(GLuint programId, const UniformLocation& location, const UniformVariable& variable)
        {
            // Texture bindings live in global GL texture-unit state, not in the program's
            // uniform state, so another draw call can rebind the same unit between two
            // uses of this program/uniform. Always re-bind rather than trusting the cache.
            if (const auto* tex = std::get_if<TextureUniformValue>(&variable)) {
                glActiveTexture(GL_TEXTURE0 + tex->unit);
                glBindTexture(GL_TEXTURE_2D, tex->textureId.value);
                glUniform1i(location.value, tex->unit);
                return;
            }

            std::unordered_map<int32_t, UniformVariable>& programCache = m_appliedUniforms.try_emplace(programId).first->second;
            const auto it = programCache.find(location.value);
            if (it != programCache.end() && uniformVariableEquals(it->second, variable))
                return;

            std::visit([&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, float>) {
                    glUniform1f(location.value, value);
                } else if constexpr (std::is_same_v<T, float2>) {
                    glUniform2f(location.value, value.x, value.y);
                } else if constexpr (std::is_same_v<T, float3>) {
                    glUniform3f(location.value, value.x, value.y, value.z);
                } else if constexpr (std::is_same_v<T, float4>) {
                    glUniform4f(location.value, value.x, value.y, value.z, value.w);
                } else if constexpr (std::is_same_v<T, matrix4x4>) {
                    glUniformMatrix4fv(location.value, 1, GL_FALSE, value.data());
                }
                // TextureUniformValue already handled above via the early return.
            }, variable);
            programCache.insert_or_assign(location.value, variable);
        }

        static void applyBlendMode(const BlendMode& mode)
        {
            const auto toFactor = [](BlendMode::Factor f) -> GLenum {
                switch (f) {
                    case BlendMode::Factor::zero: return GL_ZERO;
                    case BlendMode::Factor::one: return GL_ONE;
                    case BlendMode::Factor::srcColor: return GL_SRC_COLOR;
                    case BlendMode::Factor::oneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
                    case BlendMode::Factor::dstColor: return GL_DST_COLOR;
                    case BlendMode::Factor::oneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
                    case BlendMode::Factor::srcAlpha: return GL_SRC_ALPHA;
                    case BlendMode::Factor::oneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
                    case BlendMode::Factor::dstAlpha: return GL_DST_ALPHA;
                    case BlendMode::Factor::oneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
                }
                return GL_ONE;
            };

            const auto toEquation = [](BlendMode::Equation e) -> GLenum {
                switch (e) {
                    case BlendMode::Equation::add: return GL_FUNC_ADD;
                    case BlendMode::Equation::subtract: return GL_FUNC_SUBTRACT;
                    case BlendMode::Equation::reverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
                    case BlendMode::Equation::min: return GL_MIN;
                    case BlendMode::Equation::max: return GL_MAX;
                }
                return GL_FUNC_ADD;
            };

            glBlendFuncSeparate(toFactor(mode.srcColorFactor), toFactor(mode.dstColorFactor), toFactor(mode.srcAlphaFactor), toFactor(mode.dstAlphaFactor));
            glBlendEquationSeparate(toEquation(mode.colorEquation), toEquation(mode.alphaEquation));
        }

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_ebo = 0;

        std::unique_ptr<RenderVertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;
        size_t m_maxVertices;
        size_t m_maxIndices;

        ConcreteDrawBufferWriter m_writer;
        std::vector<BatchEntry> m_batches;

        uint2 m_viewportSize {0, 0};

        std::unordered_map<GLuint, std::unordered_map<int32_t, UniformVariable>> m_appliedUniforms;
        std::unordered_set<GLuint> m_samplerArrayApplied;
    };
} // namespace p5cpp

namespace p5cpp
{
    std::unique_ptr<NativeRenderer> NativeRenderer::create(size_t vertexCount, size_t indexCount)
    {
        return NativeOpenGLRenderer::create(vertexCount, indexCount);
    }
} // namespace p5cpp
