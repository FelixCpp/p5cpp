#pragma once

#include <p5cpp.hpp>

#include <memory>

namespace p5cpp
{
    struct UniformSnapshot
    {
        std::string name;
        UniformVariable variable;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct UniformSnapshotCollection
    {
        std::vector<UniformSnapshot> snapshots;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct TextureLookup
    {
        std::array<std::weak_ptr<Texture>, 8> textures;
        size_t textureUnitCount;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct DrawCommand
    {
        size_t indexStart;
        size_t indexCount;

        BlendMode blendMode;
        TextureLookup textureLookup;
        UniformSnapshotCollection uniformSnapshots;

        std::weak_ptr<Shader> shader;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct DrawCommandList
    {
        static std::unique_ptr<DrawCommandList> create();

        virtual ~DrawCommandList() = default;
        virtual void submit(const DrawCommand& command) = 0;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Renderer
    {
        static std::unique_ptr<Renderer> create(size_t vertexCount, size_t indexCount);

        virtual ~Renderer() = default;

        virtual void begin(std::weak_ptr<Framebuffer> framebuffer) = 0;
        virtual void end() = 0;
        virtual void flush() = 0;
    };
} // namespace p5cpp
