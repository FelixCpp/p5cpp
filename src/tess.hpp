#pragma once

#include "p5.hpp"

#include <memory>
#include <span>

#include "tesselator.h"

namespace p5
{
    class Tesselator
    {
    public:
        static std::unique_ptr<Tesselator> create();

        ~Tesselator();

        void addContour(const std::span<const float2>& points);
        bool tesselate();

        std::span<const float> vertices();
        std::span<const int> indices();

        const int* getVertexIndices();
        int getVertexCount();
        int getElementCount();

    private:
        TESStesselator* m_tess;
    };
} // namespace p5
