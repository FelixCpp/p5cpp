#include <p5cpp/graphics/renderer.hpp>

#include <cstddef>
#include <stdexcept>
#include <cassert>
#include <limits>
#include <string>

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

            glUseProgram(batch.shader->getShaderProgramId());
            const GLint projectionLocation = batch.shader->getUniformLocation("u_ProjectionMatrix");
            if (projectionLocation >= 0) {
                glUniformMatrix4fv(projectionLocation, 1, GL_TRUE, m_projectionMatrix.m.data());
            }

            const GLint textureLocation = batch.shader->getUniformLocation("u_Texture");
            if (textureLocation >= 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, batch.texture->getTextureId());
                glUniform1i(textureLocation, 0);
            }

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount), GL_UNSIGNED_INT, reinterpret_cast<void*>(batch.indexOffset * sizeof(uint32_t)));
        }

        m_currentVertexOffset = 0;
        m_currentIndexOffset = 0;
        m_batches.clear();
    }

    void Renderer::appendVertex(const Vertex& vertex)
    {
        if (m_currentVertexOffset >= m_maxVertexCount)
            throw std::runtime_error("Renderer: vertex buffer capacity (" + std::to_string(m_maxVertexCount) + ") exceeded; increase initialMaxVertices");

        m_vertices[m_currentVertexOffset++] = vertex;
    }

    void Renderer::appendIndex(uint32_t index)
    {
        if (m_currentIndexOffset >= m_maxIndexCount)
            throw std::runtime_error("Renderer: index buffer capacity (" + std::to_string(m_maxIndexCount) + ") exceeded; increase initialMaxIndices");

        m_indices[m_currentIndexOffset++] = index;
    }

    Renderer::Writer Renderer::write()
    {
        assert(m_currentVertexOffset <= std::numeric_limits<uint32_t>::max());
        return Writer(*this, static_cast<uint32_t>(m_currentVertexOffset), m_currentIndexOffset);
    }

    void Renderer::finish(const Writer& writer, const BlendMode& blendMode, const std::shared_ptr<Texture>& texture, const std::shared_ptr<Shader>& shader)
    {
        const size_t indexCount = m_currentIndexOffset - writer.m_indexOffset;
        if (indexCount == 0)
            return;

        if (not m_batches.empty()) {
            RendererBatch& lastBatch = m_batches.back();
            if (lastBatch.blendMode == blendMode and lastBatch.shader == shader and lastBatch.texture == texture) {
                lastBatch.indexCount += indexCount;
                return;
            }
        }

        m_batches.push_back(RendererBatch {
            .blendMode = blendMode,
            .shader = shader,
            .texture = texture,
            .indexOffset = writer.m_indexOffset,
            .indexCount = indexCount,
        });
    }

    Renderer::Writer::Writer(Renderer& renderer, uint32_t vertexBase, size_t indexOffset)
        : m_renderer(renderer),
          m_vertexBase(vertexBase),
          m_indexOffset(indexOffset)
    {
    }

    void Renderer::Writer::addVertex(const float2& position, const float2& texCoord, const float4& color)
    {
        m_renderer.appendVertex(Vertex {.position = position, .texCoord = texCoord, .color = color});
    }

    void Renderer::Writer::addIndex(uint32_t index)
    {
        m_renderer.appendIndex(m_vertexBase + index);
    }
} // namespace p5
