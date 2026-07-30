#pragma once

#include <p5cpp/graphics/matrix_stack.hpp>
#include <p5cpp/graphics/render_state_stack.hpp>
#include <p5cpp/graphics/render_group_impl.hpp>
#include <p5cpp/graphics/render_group.hpp>
#include <p5cpp/graphics/renderer.hpp>

#include <functional>
#include <span>

namespace p5cpp
{
    class RenderGroupRecorder
    {
    public:
        bool isActive() const { return m_activeContext != nullptr; }

        MatrixStack& activeMatrixStack(MatrixStack& liveStack);
        RenderStateStack& activeRenderStateStack(RenderStateStack& liveStack);

        DrawBufferWriter& beginDrawOp(NativeRenderer& liveRenderer);
        void endDrawOp(NativeRenderer& liveRenderer, DrawBufferWriter& writer, const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::span<const UniformSnapshot> uniforms);

        RenderGroup build(const std::function<void()>& buildFn);

    private:
        struct DrawContext
        {
            MatrixStack matrixStack;
            RenderStateStack renderStateStack;
            RecordingSink recordingSink;
        };

        DrawContext* m_activeContext = nullptr;
    };
} // namespace p5cpp
