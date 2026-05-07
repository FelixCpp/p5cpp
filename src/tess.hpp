#pragma once

#include "linepath.hpp"
#include "meshwriter.hpp"

namespace p5
{
    struct Tesselator
    {
        virtual ~Tesselator() = default;
        virtual void tesselate(MeshWriter& scope, const PathPoints& points) = 0;
    };

    std::unique_ptr<Tesselator> createTesselator();
} // namespace p5
