#include "renderer.hpp"
#include <tesselator.h>

#include <glad/glad.h>

namespace p5
{
    static GLenum drawModeToGlId(DrawMode mode)
    {
        switch (mode) {
            case DrawMode::triangles: return GL_TRIANGLES;
            case DrawMode::lineLoop: return GL_LINE_LOOP;
        }
    }

    static void enableBlendMode(BlendMode mode)
    {
        switch (mode) {
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
            case BlendMode::none:
                glDisable(GL_BLEND);
                break;
        }
    }

    static bool operator==(const DrawSettings& lhs, const DrawSettings& rhs)
    {
        if (lhs.shaderId != rhs.shaderId) return false;
        if (lhs.textureId != rhs.textureId) return false;
        if (lhs.drawMode != rhs.drawMode) return false;
        if (lhs.blendMode != rhs.blendMode) return false;

        return true;
    }
} // namespace p5

namespace p5
{
    class BatchRenderer : public Renderer
    {
    public:
        static constexpr size_t MAX_VERTICES = 65536;
        static constexpr size_t MAX_INDICES = 131072;

        BatchRenderer()
            : m_vertices(std::make_unique<Vertex[]>(MAX_VERTICES)),
              m_indices(std::make_unique<uint32_t[]>(MAX_INDICES)),
              m_projectionMatrix(matrix4x4::identity)
        {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, position)));
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, texcoord)));
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, color)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);

            uint8_t whitePixel[] = {255, 255, 255, 255};
            glGenTextures(1, &whiteTexture);
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        }

        ~BatchRenderer() override
        {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            glDeleteTextures(1, &whiteTexture);
        }

        void beginDraw(const Camera& camera) override
        {
            m_projectionMatrix = camera.projection;

            m_vertexCursor = 0;
            m_indexCursor = 0;
            m_batches.clear();
        }

        void draw(const DrawSettings& settings, const std::function<void(DrawScope&)>& callback) override
        {
            auto scope = DrawScope {
                {m_vertices.get(), MAX_VERTICES},
                {m_indices.get(), MAX_INDICES},
                m_vertexCursor,
                m_indexCursor,
            };

            callback(scope);

            if (scope.getVertexCount() == 0 or scope.getIndexCount() == 0)
                return;

            // Check if the settings of the current batch match the previous one, if so, we can merge them into a single draw call
            if (not m_batches.empty() and m_batches.back().settings == settings) {
                m_batches.back().indexCount += scope.getIndexCount();
            } else {
                m_batches.push_back({settings, m_indexCursor, scope.getIndexCount()});
            }

            m_vertexCursor += scope.getVertexCount();
            m_indexCursor += scope.getIndexCount();
        }

        void endDraw() override
        {
            if (m_batches.empty())
                return;

            // info("Flushing " + std::to_string(m_batches.size()) + " batches with total " + std::to_string(m_vertexCursor) + " vertices and " + std::to_string(m_indexCursor) + " indices");

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCursor * sizeof(Vertex), m_vertices.get());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexCursor * sizeof(uint32_t), m_indices.get());

            glBindVertexArray(vao);

            std::optional<GLuint> currentTexture;
            Shader* currentShader = nullptr;
            std::optional<BlendMode> currentBlendMode;

            for (const Batch& batch : m_batches) {
                Shader* shader = batch.settings.shaderId.get();
                GLuint textureId = batch.settings.textureId.value_or(whiteTexture);
                BlendMode blendMode = batch.settings.blendMode;

                if (currentBlendMode != blendMode) {
                    enableBlendMode(blendMode);
                    currentBlendMode = blendMode;
                }

                if (currentTexture != textureId) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textureId);
                    currentTexture = textureId;
                }

                if (currentShader != shader) {
                    glUseProgram(shader->getRendererId());
                    shader->uploadMatrix("u_ProjectionMatrix", m_projectionMatrix);
                    shader->uploadTexture("u_Texture", 0);

                    currentShader = shader;
                }

                glDrawElements(drawModeToGlId(batch.settings.drawMode), batch.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(batch.indexStart * sizeof(uint32_t)));
            }
        }

    private:
        struct Batch
        {
            DrawSettings settings;
            size_t indexStart;
            size_t indexCount;
        };

        // GPU Buffer
        GLuint vao, vbo, ebo;
        GLuint whiteTexture;
        // GLuint defaultShader;

        // CPU Staging Buffer
        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;

        // Frame State
        size_t m_vertexCursor = 0;
        size_t m_indexCursor = 0;
        std::vector<Batch> m_batches;

        matrix4x4 m_projectionMatrix;
    };
} // namespace p5

namespace p5
{
    std::unique_ptr<Renderer> createRenderer()
    {
        // return std::make_unique<DefaultRenderer>(10'000, 30'000);
        return std::make_unique<BatchRenderer>();
    }
} // namespace p5
