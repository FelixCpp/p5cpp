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
    // Redirects "the currently active drawing state" — which matrix/render-state stack
    // is in effect, and where tessellated/stroked triangles end up — so that
    // GraphicsComponent's ordinary primitive-drawing code (rect()/ellipse()/beginShape()-
    // vertex()-endShape()/image()/...) can be reused unchanged both for live drawing and
    // for recording a RenderGroup inside buildRenderGroup(), with zero duplication.
    //
    // Not itself aware of any primitive-drawing logic: GraphicsComponent still owns the
    // "live" MatrixStack/RenderStateStack and calls build()/beginDrawOp()/endDrawOp() as
    // its primitives run, passing those live objects in as the fallback to use when no
    // recording is active.
    class RenderGroupRecorder
    {
    public:
        bool isActive() const { return m_activeContext != nullptr; }

        MatrixStack& activeMatrixStack(MatrixStack& liveStack);
        RenderStateStack& activeRenderStateStack(RenderStateStack& liveStack);

        DrawBufferWriter& beginDrawOp(NativeRenderer& liveRenderer);
        void endDrawOp(NativeRenderer& liveRenderer, DrawBufferWriter& writer, const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::span<const UniformSnapshot> uniforms);

        // Runs buildFn with a freshly isolated matrix stack (starts at identity) and
        // render-state stack (starts at sketch defaults), capturing whatever geometry it
        // submits via beginDrawOp()/endDrawOp() instead of drawing it live. Supports
        // nested calls (a buildFn that itself calls build() again) and calls made while
        // replaying another RenderGroup via beginDrawOp()/endDrawOp() (composition).
        RenderGroup build(const std::function<void()>& buildFn);

    private:
        struct DrawContext
        {
            MatrixStack matrixStack;
            RenderStateStack renderStateStack;
            RecordingSink recordingSink;
        };

        // nullptr = no recording in progress, use the live stacks/renderer passed in.
        DrawContext* m_activeContext = nullptr;
    };
} // namespace p5cpp
