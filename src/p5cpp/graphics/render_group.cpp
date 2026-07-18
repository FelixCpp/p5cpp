#include <p5cpp/graphics/render_group.hpp>

#include <utility>

namespace p5cpp
{
    RenderGroup::RenderGroup() : impl(nullptr) {}

    RenderGroup::RenderGroup(std::shared_ptr<const RenderGroupImpl> impl) : impl(std::move(impl)) {}

    const std::shared_ptr<const RenderGroupImpl>& RenderGroup::getImpl() const
    {
        return impl;
    }
} // namespace p5cpp
