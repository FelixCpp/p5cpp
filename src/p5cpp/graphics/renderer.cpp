#include <p5cpp/graphics/renderer.hpp>

#include <cstddef>
#include <stdexcept>
#include <algorithm>

namespace p5
{
    namespace
    {
        void apply(const BlendMode& blendMode)
        {
            constexpr auto blendFactorToGl = [](const BlendMode::Factor factor) -> GLenum {
                switch (factor) {
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
                    default: throw std::invalid_argument("Invalid blend factor");
                }
            };

            constexpr auto blendEquationToGl = [](const BlendMode::Equation equation) -> GLenum {
                switch (equation) {
                    case BlendMode::Equation::add: return GL_FUNC_ADD;
                    case BlendMode::Equation::subtract: return GL_FUNC_SUBTRACT;
                    case BlendMode::Equation::reverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
                    case BlendMode::Equation::min: return GL_MIN;
                    case BlendMode::Equation::max: return GL_MAX;
                    default: throw std::invalid_argument("Invalid blend equation");
                }
            };

            glBlendFuncSeparate(
                blendFactorToGl(blendMode.srcColorFactor),
                blendFactorToGl(blendMode.dstColorFactor),
                blendFactorToGl(blendMode.srcAlphaFactor),
                blendFactorToGl(blendMode.dstAlphaFactor)
            );

            glBlendEquationSeparate(
                blendEquationToGl(blendMode.colorEquation),
                blendEquationToGl(blendMode.alphaEquation)
            );
        }
    } // namespace

    std::unique_ptr<Renderer> Renderer::create(size_t initialMaxVertices, size_t initialMaxIndices)
    {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(initialMaxVertices * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(initialMaxIndices * sizeof(uint32_t)), nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        return std::unique_ptr<Renderer>(new Renderer(vao, vbo, ebo, initialMaxVertices, initialMaxIndices));
    }

    Renderer::Renderer(GLuint vao, GLuint vbo, GLuint ebo, size_t maxVertexCount, size_t maxIndexCount)
        : m_vao(vao),
          m_vbo(vbo),
          m_ebo(ebo),
          m_vertices(std::make_unique<Vertex[]>(maxVertexCount)),
          m_indices(std::make_unique<uint32_t[]>(maxIndexCount)),
          m_maxVertexCount(maxVertexCount),
          m_maxIndexCount(maxIndexCount),
          m_currentVertexOffset(0),
          m_currentIndexOffset(0)
    {
    }

    Renderer::~Renderer()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }

    void Renderer::begin(std::shared_ptr<Framebuffer> framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->getFramebufferId());

        const uint2& size = framebuffer->getSize();
        glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));

        glEnable(GL_BLEND);

        m_projectionMatrix = orthographicProjectionMatrix(0.0f, 0.0f, static_cast<float>(size.x), static_cast<float>(size.y), -1.0f, 1.0f);

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    void Renderer::end()
    {
        flush();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Renderer::flush()
    {
        if (m_currentIndexOffset == 0)
            return;

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_currentVertexOffset * sizeof(Vertex)), m_vertices.get());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(m_currentIndexOffset * sizeof(uint32_t)), m_indices.get());

        for (const RendererBatch& batch : m_batches) {
            apply(batch.blendMode);

            glUseProgram(batch.shaderProgramId);
            const GLint projectionLocation = glGetUniformLocation(batch.shaderProgramId, "u_ProjectionMatrix");
            if (projectionLocation >= 0) {
                glUniformMatrix4fv(projectionLocation, 1, GL_TRUE, m_projectionMatrix.m.data());
            }

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount), GL_UNSIGNED_INT, reinterpret_cast<void*>(batch.indexOffset * sizeof(uint32_t)));
        }

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    void Renderer::grow(size_t requiredVertexCount, size_t requiredIndexCount)
    {
        size_t newMaxVertexCount = m_maxVertexCount > 0 ? m_maxVertexCount : 1;
        while (newMaxVertexCount < requiredVertexCount)
            newMaxVertexCount *= 2;

        size_t newMaxIndexCount = m_maxIndexCount > 0 ? m_maxIndexCount : 1;
        while (newMaxIndexCount < requiredIndexCount)
            newMaxIndexCount *= 2;

        m_vertices = std::make_unique<Vertex[]>(newMaxVertexCount);
        m_indices = std::make_unique<uint32_t[]>(newMaxIndexCount);

        m_maxVertexCount = newMaxVertexCount;
        m_maxIndexCount = newMaxIndexCount;

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_maxVertexCount * sizeof(Vertex)), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_maxIndexCount * sizeof(uint32_t)), nullptr, GL_DYNAMIC_DRAW);
    }

    Renderer::ReservedRange Renderer::reserve(size_t vertexCount, size_t indexCount, const BlendMode& blendMode, GLuint shaderProgramId)
    {
        if (m_currentVertexOffset + vertexCount > m_maxVertexCount or m_currentIndexOffset + indexCount > m_maxIndexCount)
            flush();

        if (vertexCount > m_maxVertexCount or indexCount > m_maxIndexCount)
            grow(vertexCount, indexCount);

        const auto insertNewBatch = [&]() {
            m_batches.push_back(RendererBatch {
                .blendMode = blendMode,
                .shaderProgramId = shaderProgramId,
                .indexOffset = m_currentIndexOffset,
                .indexCount = indexCount,
            });
        };

        if (m_batches.empty()) {
            insertNewBatch();
        } else {
            const RendererBatch& lastBatch = m_batches.back();
            const bool isMergable = lastBatch.blendMode == blendMode and lastBatch.shaderProgramId == shaderProgramId;

            if (isMergable) {
                m_batches.back().indexCount += indexCount;
            } else {
                insertNewBatch();
            }
        }

        const ReservedRange range {
            .vertexOffset = m_currentVertexOffset,
            .indexOffset = m_currentIndexOffset,
        };

        m_currentVertexOffset += vertexCount;
        m_currentIndexOffset += indexCount;

        return range;
    }

    void Renderer::submit(const RendererSubmission& submission)
    {
        const size_t vertexCount = submission.vertices.size();
        const size_t indexCount = submission.indices.size();
        const GLuint shaderProgramId = submission.shaderProgramId->getShaderProgramId();

        const ReservedRange range = reserve(vertexCount, indexCount, submission.blendMode, shaderProgramId);

        std::copy(submission.vertices.begin(), submission.vertices.end(), m_vertices.get() + range.vertexOffset);

        for (size_t i = 0; i < indexCount; ++i)
            m_indices[range.indexOffset + i] = submission.indices[i] + static_cast<uint32_t>(range.vertexOffset);
    }

    Renderer::Writer Renderer::write(size_t maxVertexCount, size_t maxIndexCount, const BlendMode& blendMode, const std::shared_ptr<Shader>& shader)
    {
        const GLuint shaderProgramId = shader->getShaderProgramId();
        const ReservedRange range = reserve(maxVertexCount, maxIndexCount, blendMode, shaderProgramId);

        return Writer(m_vertices.get() + range.vertexOffset, m_indices.get() + range.indexOffset, static_cast<uint32_t>(range.vertexOffset), range.vertexOffset, range.indexOffset);
    }

    void Renderer::finish(const Writer& writer)
    {
        const size_t unusedVertexCount = (m_currentVertexOffset - writer.m_vertexOffset) - writer.m_vertexCount;
        const size_t unusedIndexCount = (m_currentIndexOffset - writer.m_indexOffset) - writer.m_indexCount;

        m_currentVertexOffset -= unusedVertexCount;
        m_currentIndexOffset -= unusedIndexCount;
        m_batches.back().indexCount -= unusedIndexCount;
    }

    Renderer::Writer::Writer(Vertex* vertexCursor, uint32_t* indexCursor, uint32_t vertexBase, size_t vertexOffset, size_t indexOffset)
        : m_vertexCursor(vertexCursor),
          m_indexCursor(indexCursor),
          m_vertexBase(vertexBase),
          m_vertexOffset(vertexOffset),
          m_indexOffset(indexOffset),
          m_vertexCount(0),
          m_indexCount(0)
    {
    }

    void Renderer::Writer::addVertex(const float2& position, const float2& texCoord, const float4& color)
    {
        *m_vertexCursor++ = Vertex {.position = position, .texCoord = texCoord, .color = color};
        ++m_vertexCount;
    }

    void Renderer::Writer::addIndex(uint32_t index)
    {
        *m_indexCursor++ = m_vertexBase + index;
        ++m_indexCount;
    }
} // namespace p5
