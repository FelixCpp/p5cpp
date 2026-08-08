#include <p5cpp/p5cpp.hpp>

namespace p5
{
    Next::Next(std::span<const std::unique_ptr<Plugin>> chain, size_t index, Context* context, void (*step)(Plugin&, Context&, const Next&), const void* payload)
        : m_chain(chain), m_index(index), context(context), m_step(step), payload(payload)
    {
    }

    void Next::operator()() const
    {
        if (m_index >= m_chain.size()) {
            return;
        }

        const Next following {m_chain, m_index + 1, context, m_step, payload};
        m_step(*m_chain[m_index], *context, following);
    }
} // namespace p5
