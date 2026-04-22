#include "tess.hpp"
#include "tesselator.h"
#include <vector>

namespace p5
{
    std::unique_ptr<Tesselator> Tesselator::create()
    {
        auto tesselator = new Tesselator {};
        tesselator->m_tess = tessNewTess(nullptr);

        return std::unique_ptr<Tesselator>(tesselator);
    }

    Tesselator::~Tesselator()
    {
        tessDeleteTess(m_tess);
    }

    void Tesselator::addContour(const std::span<const float2>& points)
    {
        std::vector<float> pts;
        for (size_t i = 0; i < points.size(); ++i) {
            pts.push_back(points[i].x);
            pts.push_back(points[i].y);
            pts.push_back(0.0f);
        }

        tessDeleteTess(m_tess);
        m_tess = tessNewTess(nullptr);

        tessAddContour(m_tess, 3, pts.data(), sizeof(float) * 3, static_cast<int>(pts.size()));
    }

    bool Tesselator::tesselate()
    {
        return tessTesselate(m_tess, TESS_WINDING_ODD, TESS_POLYGONS, 3, 3, nullptr);
    }

    std::span<const float> Tesselator::vertices()
    {
        return {tessGetVertices(m_tess), static_cast<size_t>(tessGetVertexCount(m_tess))};
    }
    std::span<const int> Tesselator::indices()
    {
        return {tessGetElements(m_tess), static_cast<size_t>(tessGetElementCount(m_tess))};
    }

    const int* Tesselator::getVertexIndices()
    {
        return tessGetVertexIndices(m_tess);
    }

    int Tesselator::getVertexCount()
    {
        return tessGetVertexCount(m_tess);
    }

    int Tesselator::getElementCount()
    {
        return tessGetElementCount(m_tess);
    }
} // namespace p5
