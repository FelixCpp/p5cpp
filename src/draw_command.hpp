#pragma once

#include <p5cpp.hpp>

namespace p5cpp
{
    struct UniformSnapshot
    {
        std::string name;
        UniformVariable variable;
    };

    // A Draw-Command represents a single draw call made by the user, such as
    // drawing a rectangle, a line, or a text. It captures all the necessary information
    // for the renderer to execute this draw call, such as the shader to use, the blend mode as well as the textures and more.
    struct DrawCommand
    {
        size_t drawBufferIndexStart;           // Where in the global DrawBuffer this draw command starts
        size_t drawBufferIndexCount;           // How many indices this draw command has in the global DrawBuffer
        Shader* shader;                        // Which shader to use for this draw command
        std::vector<UniformSnapshot> uniforms; // A snapshot of the uniform cache at the time this draw command was created, this is used to upload the correct uniform values when this draw command is executed by the renderer
        BlendMode blendMode;                   // Which blend mode to use for this draw command
        std::array<uint32_t, 8> textureUnits;  // Which texture units to bind for this draw command
        size_t textureUnitCount;               // The number of texture units used by this draw command
    };
} // namespace p5cpp
