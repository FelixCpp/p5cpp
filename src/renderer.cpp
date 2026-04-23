#include "renderer.hpp"
#include <tesselator.h>

#include <glad/glad.h>

namespace p5
{
    inline static float4 color_to_float4(color_t color)
    {
        return float4 {
            .x = static_cast<float>(red(color)) / 255.0f,
            .y = static_cast<float>(green(color)) / 255.0f,
            .z = static_cast<float>(blue(color)) / 255.0f,
            .w = static_cast<float>(alpha(color)) / 255.0f,
        };
    }

    static GLenum drawModeToGlId(DrawMode mode)
    {
        switch (mode) {
            case DrawMode::triangles: return GL_TRIANGLES;
            case DrawMode::lineLoop: return GL_LINE_LOOP;
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
    void convex(DrawScope& scope, const std::span<const DrawPoint>& points)
    {
        for (size_t i = 0; i < points.size(); ++i) {
            const DrawPoint& point = points[i];
            Vertex& vertex = scope.vertices[scope.vertexStart + i];
            vertex.position = float3 {point.position.x, point.position.y, 0.0f};
            vertex.texcoord = point.texcoord;
            vertex.color = color_to_float4(point.fillColor);
        }

        for (size_t i = 0; i < points.size(); ++i) {
            scope.indices[scope.indexStart + i * 3 + 0] = i;
            scope.indices[scope.indexStart + i * 3 + 1] = (i + 1) % points.size();
            scope.indices[scope.indexStart + i * 3 + 2] = (i + 2) % points.size();
        }

        scope.vertexCount += points.size();
        scope.indexCount += points.size() * 3;
    }

    void concave(DrawScope& scope, const std::span<const DrawPoint>& points)
    {
        TESStesselator* tess = tessNewTess(nullptr);
        std::unique_ptr<float[]> tessPts = std::make_unique<float[]>(points.size() * 3);
        for (size_t i = 0; i < points.size(); ++i) {
            tessPts[i * 3 + 0] = points[i].position.x;
            tessPts[i * 3 + 1] = points[i].position.y;
            tessPts[i * 3 + 2] = 0.0f;
        }

        tessAddContour(tess, 3, tessPts.get(), sizeof(float) * 3, points.size());
        // tessTesselate(tess, TESS_WINDING_NONZERO, TESS_POLYGONS, 3, 3, nullptr);
        tessTesselate(tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr);

        const TESSreal* tessVerts = tessGetVertices(tess);
        const int* tessIdx = tessGetElements(tess);
        const int* vertexIndex = tessGetVertexIndices(tess);
        const int vertexCount = tessGetVertexCount(tess);
        const int elemCount = tessGetElementCount(tess);

        for (int i = 0; i < vertexCount; ++i) {
            const int srcIdx = vertexIndex[i];

            if (srcIdx == TESS_UNDEF) {
                scope.vertices[scope.vertexStart + i].position = float3 {tessVerts[i * 3 + 0], tessVerts[i * 3 + 1], 0.0f};
                scope.vertices[scope.vertexStart + i].texcoord = float2 {0.0f, 0.0f};
                scope.vertices[scope.vertexStart + i].color = color_to_float4(points[0].fillColor);
            } else {
                const DrawPoint& src = points[srcIdx];
                scope.vertices[scope.vertexStart + i].position = float3 {src.position.x, src.position.y, 0.0f};
                scope.vertices[scope.vertexStart + i].texcoord = src.texcoord;
                scope.vertices[scope.vertexStart + i].color = color_to_float4(src.fillColor);
            }
        }

        for (int i = 0; i < elemCount; ++i) {
            const int a = tessIdx[i * 3 + 0];
            const int b = tessIdx[i * 3 + 1];
            const int c = tessIdx[i * 3 + 2];
            if (a == TESS_UNDEF || b == TESS_UNDEF || c == TESS_UNDEF) continue;
            scope.indices[scope.indexStart + i * 3 + 0] = a;
            scope.indices[scope.indexStart + i * 3 + 1] = b;
            scope.indices[scope.indexStart + i * 3 + 2] = c;
        }

        tessDeleteTess(tess);
        scope.vertexCount += vertexCount;
        scope.indexCount += elemCount * 3;
    }
} // namespace p5

inline static constexpr const char* vSource = R"(
  #version 330 core

  layout (location = 0) in vec3 a_Position;
  layout (location = 1) in vec2 a_TexCoord;
  layout (location = 2) in vec4 a_Color;

  out vec2 v_TexCoord;
  out vec4 v_Color;

  uniform mat4 u_ProjectionMatrix;

  void main() {
    gl_Position = u_ProjectionMatrix * vec4(a_Position, 1.0);
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
    class DefaultRenderer : public Renderer
    {
    public:
        DefaultRenderer(size_t vertexCount, size_t indexCount) : m_vertices(std::make_unique<Vertex[]>(vertexCount)),
                                                                 m_indices(std::make_unique<uint32_t[]>(indexCount)),
                                                                 m_vertexCount(vertexCount),
                                                                 m_indexCount(indexCount)
        {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, position)));
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

        ~DefaultRenderer() override
        {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
            glDeleteProgram(defaultShader);
            glDeleteTextures(1, &whiteTexture);
        }

        void beginDraw() override
        {
        }

        void endDraw() override
        {
        }

        void draw(const DrawSettings& settings, const std::function<void(DrawScope&)>& callback) override
        {
            DrawScope scope = {
                .vertices = {m_vertices.get(), m_vertexCount},
                .indices = {m_indices.get(), m_indexCount},
                .vertexStart = 0,
                .vertexCount = 0,
                .indexStart = 0,
                .indexCount = 0,
            };

            callback(scope);

            submit(scope, settings);
        }

    private:
        void submit(const DrawScope& scope, const DrawSettings& settings)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, scope.vertexStart * sizeof(Vertex), scope.vertexCount * sizeof(Vertex), scope.vertices.data());

            glBindBuffer(GL_ARRAY_BUFFER, ebo);
            glBufferSubData(GL_ARRAY_BUFFER, scope.indexStart * sizeof(uint32_t), scope.indexCount * sizeof(uint32_t), scope.indices.data());

            GLuint shaderId = settings.shaderId.value_or(defaultShader);
            GLuint textureId = settings.textureId.value_or(whiteTexture);

            // clang-format off
            float l = 0, r = 800, t = 0, b = 600;
            float projectionMatrix[] = {
                 2/(r-l),      0,       0,  0,
                 0,       2/(t-b),      0,  0,
                 0,            0,      -1,  0,
                -(r+l)/(r-l), -(t+b)/(t-b), 0,  1,
            };
            // clang-format on

            glUseProgram(shaderId);
            glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ProjectionMatrix"), 1, GL_FALSE, projectionMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glUniform1i(glGetUniformLocation(shaderId, "u_Texture"), 0);

            glBindVertexArray(vao);
            glDrawElements(drawModeToGlId(settings.drawMode), scope.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(scope.indexStart * sizeof(uint32_t)));
        }

        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        GLuint defaultShader;
        GLuint whiteTexture;

        std::unique_ptr<Vertex[]> m_vertices;
        std::unique_ptr<uint32_t[]> m_indices;

        size_t m_vertexCount;
        size_t m_indexCount;
    };

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
              m_indices(std::make_unique<uint32_t[]>(MAX_INDICES))
        {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(offsetof(Vertex, position)));
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

        void beginDraw() override
        {
            m_vertexCursor = 0;
            m_indexCursor = 0;
            m_batches.clear();
        }

        void draw(const DrawSettings& settings, const std::function<void(DrawScope&)>& callback) override
        {
            DrawScope scope = {
                .vertices = {m_vertices.get(), MAX_VERTICES},
                .indices = {m_indices.get(), MAX_INDICES},
                .vertexStart = m_vertexCursor,
                .vertexCount = 0,
                .indexStart = m_indexCursor,
                .indexCount = 0,
            };

            callback(scope);

            if (scope.vertexCount == 0 or scope.indexCount == 0)
                return;

            m_batches.push_back({settings, m_indexCursor, scope.indexCount});
            m_vertexCursor += scope.vertexCount;
            m_indexCursor += scope.indexCount;
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

            // clang-format off
            float l = 0, r = 800, t = 0, b = 600;
            float projectionMatrix[] = {
                 2/(r-l),      0,       0,  0,
                 0,       2/(t-b),      0,  0,
                 0,            0,      -1,  0,
                -(r+l)/(r-l), -(t+b)/(t-b), 0,  1,
            };
            // clang-format on

            for (const auto& batch : m_batches) {

                GLuint shaderId = batch.settings.shaderId.value_or(defaultShader);
                GLuint textureId = batch.settings.textureId.value_or(whiteTexture);

                glUseProgram(shaderId);
                glUniformMatrix4fv(glGetUniformLocation(shaderId, "u_ProjectionMatrix"), 1, GL_FALSE, projectionMatrix);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
                glUniform1i(glGetUniformLocation(shaderId, "u_Texture"), 0);

                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid*>(batch.indexStart * sizeof(uint32_t)));
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
