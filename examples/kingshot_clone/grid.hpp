#pragma once

#include <p5cpp/p5cpp.hpp>

namespace kingshot
{
    // The base is laid out on a fixed-size orthogonal grid of square cells. Grid
    // coordinates are in cells (int2), world coordinates are in pixels (float2) with
    // the grid's top-left corner sitting at world-space (0, 0).
    struct Grid
    {
        int32_t columns = 24;
        int32_t rows = 24;
        float cellSize = 48.0f;
    };

    p5::int2 worldToCell(const Grid& grid, const p5::float2& worldPos);
    p5::float2 cellToWorld(const Grid& grid, const p5::int2& cell); // top-left corner of the cell
    p5::float2 gridWorldSize(const Grid& grid);
    p5::float2 gridWorldCenter(const Grid& grid);
    bool isCellInBounds(const Grid& grid, const p5::int2& cell);

    // True if every cell in the `footprint`-sized rectangle starting at `origin` is
    // in bounds. Does not check occupancy -- that's BaseState's job.
    bool isFootprintInBounds(const Grid& grid, const p5::int2& origin, const p5::int2& footprint);

    void drawGrid(const Grid& grid);
} // namespace kingshot
