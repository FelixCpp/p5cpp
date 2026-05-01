#include "renderer.hpp"
#include <tesselator.h>

#include <glad/glad.h>

namespace p5
{
    static GLenum drawModeToGlId(DrawMode mode)
    {
        switch (mode) {
            case DrawMode::triangles: return GL_TRIANGLES;
            case DrawMode::lineLoop: return GL_LINE_STRIP;
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

inline static constexpr const char* vSource = R"(
  #version 330 core

  layout (location = 0) in vec2 a_Position;
  layout (location = 1) in vec2 a_TexCoord;
  layout (location = 2) in vec4 a_Color;

  out vec2 v_TexCoord;
  out vec4 v_Color;

  uniform mat4 u_ProjectionMatrix;

  void main() {
    gl_Position = u_ProjectionMatrix * vec4(a_Position, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
  }
)";

inline static constexpr const char* fSource = R"(
  #version 330 core

  layout (location = 0) out vec4 o_Color;

  in vec2 v_TexCoord;
  in vec4 v_Color;

  uniform sampler2D u_Texture;

  void main() {
    o_Color = texture(u_Texture, v_TexCoord) * v_Color;
  }
)";

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

            GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vShader, 1, &vSource, nullptr);
            glCompileShader(vShader);

            GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fShader, 1, &fSource, nullptr);
            glCompileShader(fShader);

            defaultShader = glCreateProgram();
            glAttachShader(defaultShader, vShader);
            glAttachShader(defaultShader, fShader);
            glLinkProgram(defaultShader);
            glDetachShader(defaultShader, vShader);
            glDetachShader(defaultShader, fShader);
            glDeleteShader(vShader);
            glDeleteShader(fShader);
        }

        ~BatchRenderer() override
        {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            glDeleteTextures(1, &whiteTexture);
            glDeleteProgram(defaultShader);
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

            m_batches.push_back({settings, m_indexCursor, scope.getIndexCount()});
            m_vertexCursor += scope.getVertexCount();
            m_indexCursor += scope.getIndexCount();
        }

        void endDraw() override
        {
            if (m_batches.empty())
                return;

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexCursor * sizeof(Vertex), m_vertices.get());

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indexCursor * sizeof(uint32_t), m_indices.get());

            glBindVertexArray(vao);

            for (const auto& batch : m_batches) {

                GLuint shaderId = batch.settings.shaderId.value_or(defaultShader);
                GLuint textureId = batch.settings.textureId.value_or(whiteTexture);

                glUseProgram(shaderId);
                glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ProjectionMatrix"), 1, GL_FALSE, m_projectionMatrix.m);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                glUniform1i(glGetUniformLocation(shaderId, "u_Texture"), 0);

                enableBlendMode(batch.settings.blendMode);

                glBindVertexArray(vao);
                glDrawElements(drawModeToGlId(batch.settings.drawMode), batch.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(batch.indexStart * sizeof(uint32_t)));
            }

            glBindVertexArray(0);
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
        GLuint defaultShader;

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
