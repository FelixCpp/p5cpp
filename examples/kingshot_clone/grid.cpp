#include "grid.hpp"

#include <cmath>

namespace kingshot
{
    using p5::float2;
    using p5::int2;

    int2 worldToCell(const Grid& grid, const float2& worldPos)
    {
        return int2 {
            .x = static_cast<int32_t>(std::floor(worldPos.x / grid.cellSize)),
            .y = static_cast<int32_t>(std::floor(worldPos.y / grid.cellSize)),
        };
    }

    float2 cellToWorld(const Grid& grid, const int2& cell)
    {
        return float2 {
            .x = static_cast<float>(cell.x) * grid.cellSize,
            .y = static_cast<float>(cell.y) * grid.cellSize,
        };
    }

    float2 gridWorldSize(const Grid& grid)
    {
        return float2 {
            .x = static_cast<float>(grid.columns) * grid.cellSize,
            .y = static_cast<float>(grid.rows) * grid.cellSize,
        };
    }

    float2 gridWorldCenter(const Grid& grid)
    {
        return gridWorldSize(grid) * 0.5f;
    }

    bool isCellInBounds(const Grid& grid, const int2& cell)
    {
        return cell.x >= 0 and cell.y >= 0 and cell.x < grid.columns and cell.y < grid.rows;
    }

    bool isFootprintInBounds(const Grid& grid, const int2& origin, const int2& footprint)
    {
        const int2 lastCell {.x = origin.x + footprint.x - 1, .y = origin.y + footprint.y - 1};
        return isCellInBounds(grid, origin) and isCellInBounds(grid, lastCell);
    }

    void drawGrid(const Grid& grid)
    {
        const float2 size = gridWorldSize(grid);

        p5::noFill();
        p5::stroke(p5::rgba(80));
        p5::strokeWeight(1.0f);

        for (int32_t x = 0; x <= grid.columns; ++x) {
            const float worldX = static_cast<float>(x) * grid.cellSize;
            p5::line(worldX, 0.0f, worldX, size.y);
        }

        for (int32_t y = 0; y <= grid.rows; ++y) {
            const float worldY = static_cast<float>(y) * grid.cellSize;
            p5::line(0.0f, worldY, size.x, worldY);
        }
    }
} // namespace kingshot
