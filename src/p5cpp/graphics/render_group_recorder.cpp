#include <p5cpp/graphics/render_group_recorder.hpp>

#include <utility>

namespace p5cpp
{
    MatrixStack& RenderGroupRecorder::activeMatrixStack(MatrixStack& liveStack)
    {
        return m_activeContext ? m_activeContext->matrixStack : liveStack;
    }

    RenderStateStack& RenderGroupRecorder::activeRenderStateStack(RenderStateStack& liveStack)
    {
        return m_activeContext ? m_activeContext->renderStateStack : liveStack;
    }

    DrawBufferWriter& RenderGroupRecorder::beginDrawOp(NativeRenderer& liveRenderer)
    {
        if (m_activeContext) {
            return m_activeContext->recordingSink.writer;
        }

        return liveRenderer.getDrawScope();
    }

    void RenderGroupRecorder::endDrawOp(NativeRenderer& liveRenderer, DrawBufferWriter& writer, const Shader& shader, const BlendMode& blendMode, const Texture& texture, std::span<const UniformSnapshot> uniforms)
    {
        if (m_activeContext) {
            m_activeContext->recordingSink.commit(shader, blendMode, texture, std::vector<UniformSnapshot>(uniforms.begin(), uniforms.end()));
            return;
        }

        liveRenderer.submit(writer, uniforms, shader, blendMode, texture);
    }

    RenderGroup RenderGroupRecorder::build(const std::function<void()>& buildFn)
    {
        DrawContext context;
        DrawContext* previous = m_activeContext;
        m_activeContext = &context; // "from now on: use this context"

        struct RestoreContext
        {
            RenderGroupRecorder& recorder;
            DrawContext* previous;
            ~RestoreContext() { recorder.m_activeContext = previous; }
        } restore {*this, previous};

        buildFn();

        return RenderGroup(std::make_shared<const RenderGroupImpl>(RenderGroupImpl {std::move(context.recordingSink.ops)}));
    }
} // namespace p5cpp
