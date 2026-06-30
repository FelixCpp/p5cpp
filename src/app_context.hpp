#pragma once

#include <cstdint>
#include <memory>
#include <p5cpp.hpp>

namespace p5cpp
{
    struct FrameInfo
    {
        float deltaTime;
        float globalTime;
        uint64_t frameCount;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct LifecycleInfo
    {
        bool closeRequested;
        bool isPaused;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct RenderingInfo
    {
        std::shared_ptr<Shader> defaultShader;
        std::shared_ptr<Shader> textShader;
        std::shared_ptr<Font> defaultFont;
        std::shared_ptr<Framebuffer> defaultFramebuffer;
        std::shared_ptr<Texture> whiteTexture;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Sketch;
    struct Window;
    struct Renderer;
    struct Engine;

    struct AppContext
    {
        FrameInfo frameInfo;
        LifecycleInfo lifecycleInfo;
        RenderingInfo renderingInfo;

        Engine* engine;
        Sketch* sketch;
        Window* window;
    };
} // namespace p5cpp
