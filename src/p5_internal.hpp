#pragma once

#include "geometry.hpp"
#include "p5.hpp"

#include <glad/glad.h>
#include <memory>
#include <optional>

namespace p5
{
    struct DrawSubmission
    {
        std::optional<GLuint> shaderId;
        std::optional<GLuint> textureId;
        BlendMode blendMode;
    };

    struct DrawCommandKey
    {
        uint32_t layer;
        BlendMode blendMode;
        GLuint shaderId;
        GLuint textureId;
    };

    class Renderer
    {
    public:
        static std::unique_ptr<Renderer> create(GeometryBuffer& buffer, size_t maxBatchSize);

        ~Renderer();

        void beginDraw();
        void endDraw();
        void flush();

        inline void draw(const DrawSubmission& submission, std::invocable<GeometryBuilder&> auto&& fn)
        {
            GeometryBuilder builder = getNextBuilder();
            fn(builder);
            submit(builder, submission);
        }

        void submit(const GeometryBuilder& builder, const DrawSubmission& submission);

    private:
        GeometryBuilder getNextBuilder();
        bool needsFlush(size_t additionalVertices, size_t additionalIndices, size_t additionalBatches);

        DrawCommandKey generateKey(const DrawSubmission& submission);

        struct DrawCommand
        {
            DrawCommandKey key;
            size_t indexOffset;
            size_t indexCount;
        };

        GLuint m_vertexArrayId;
        GLuint m_vertexBufferId;
        GLuint m_indexBufferId;
        GLuint m_shaderProgramId;
        GLuint m_whiteTextureId;

        size_t m_maxBatchSize;

        size_t m_vertexOffset;
        size_t m_indexOffset;
        size_t m_commandOffset;

        GeometryBuffer* m_buffer;

        std::unique_ptr<DrawCommand[]> m_commands;
        uint32_t m_submissionCounter;
    };

} // namespace p5
