#pragma once

#include <cstdint>
#include <p5cpp.hpp>

namespace p5cpp
{
    struct FrameInfo
    {
        float deltaTime;
        float globalTime;
        float framesPerSecond;
        uint64_t frameCount;

        int targetFrameRate;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct InputInfo
    {
        int mouseX;
        int mouseY;
        int pmouseX;
        int pmouseY;

        int logicalWidth;
        int logicalHeight;
        int physicalWidth;
        int physicalHeight;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct LifecycleInfo
    {
        bool closeRequested = false;
        bool isPaused = false;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Renderer;
    struct RenderStateStack;

    struct RenderingInfo
    {
        Shader* defaultShader;
        Shader* textShader;
        Font* defaultFont;
        Framebuffer* defaultFramebuffer;
        Texture* whiteTexture;
        Renderer* renderer;
        RenderStateStack* renderStateStack;
    };
} // namespace p5cpp

namespace p5cpp
{
    struct Sketch;
    struct Window;
    struct Engine;

    struct AppContext
    {
        FrameInfo frameInfo;
        InputInfo inputInfo;
        LifecycleInfo lifecycleInfo;
        RenderingInfo renderingInfo;

        Engine* engine;
        Sketch* sketch;
        Window* window;
    };
} // namespace p5cpp
