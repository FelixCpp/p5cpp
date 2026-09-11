#include <p5cpp/p5cpp.hpp>

namespace p5
{
    Next::Next(const std::deque<std::unique_ptr<Plugin>>& chain, size_t index, Step step, const void* payload)
        : m_chain(chain), m_index(index), m_step(step), payload(payload)
    {
    }

    void Next::operator()() const
    {
        if (m_index >= m_chain.size()) {
            return;
        }

        const Next following {m_chain, m_index + 1, m_step, payload};
        m_step(*m_chain[m_index], following);
    }

    const void* Next::getPayload() const
    {
        return payload;
    }
} // namespace p5
