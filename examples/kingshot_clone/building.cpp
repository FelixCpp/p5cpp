#include "building.hpp"

#include <array>

namespace kingshot
{
    using p5::int2;

    namespace
    {
        // clang-format off
        constexpr std::array<BuildingDef, static_cast<size_t>(BuildingType::Count)> catalog {{
            { .type = BuildingType::TownHall, .name = "Town Hall", .footprint = {3, 3}, .fillColor = p5::rgba(200, 170, 60),  .strokeColor = p5::rgba(255, 220, 120), .placeableByPlayer = false },
            { .type = BuildingType::Wall,     .name = "Wall",      .footprint = {1, 1}, .fillColor = p5::rgba(120, 120, 130), .strokeColor = p5::rgba(200, 200, 210), .placeableByPlayer = true  },
            { .type = BuildingType::Barracks, .name = "Barracks",  .footprint = {2, 2}, .fillColor = p5::rgba(160, 60, 60),   .strokeColor = p5::rgba(230, 100, 100), .placeableByPlayer = true  },
            { .type = BuildingType::Farm,     .name = "Farm",      .footprint = {2, 2}, .fillColor = p5::rgba(70, 140, 70),   .strokeColor = p5::rgba(130, 220, 130), .placeableByPlayer = true  },
        }};
        // clang-format on
    } // namespace

    const BuildingDef& getBuildingDef(BuildingType type)
    {
        return catalog[static_cast<size_t>(type)];
    }

    std::span<const BuildingDef> getBuildingCatalog()
    {
        return catalog;
    }

    BaseState::BaseState(Grid grid)
        : m_grid(grid)
    {
        const BuildingDef& townHall = getBuildingDef(BuildingType::TownHall);
        const int2 center {
            .x = (grid.columns - townHall.footprint.x) / 2,
            .y = (grid.rows - townHall.footprint.y) / 2,
        };
        m_buildings.push_back({.type = BuildingType::TownHall, .cell = center});
    }

    const Grid& BaseState::getGrid() const
    {
        return m_grid;
    }

    std::span<const PlacedBuilding> BaseState::getBuildings() const
    {
        return m_buildings;
    }

    bool BaseState::canPlaceAt(BuildingType type, const int2& originCell) const
    {
        const BuildingDef& def = getBuildingDef(type);
        if (not isFootprintInBounds(m_grid, originCell, def.footprint)) {
            return false;
        }

        const int2 newMin = originCell;
        const int2 newMax {.x = originCell.x + def.footprint.x - 1, .y = originCell.y + def.footprint.y - 1};

        for (const PlacedBuilding& other : m_buildings) {
            const BuildingDef& otherDef = getBuildingDef(other.type);
            const int2 otherMin = other.cell;
            const int2 otherMax {.x = other.cell.x + otherDef.footprint.x - 1, .y = other.cell.y + otherDef.footprint.y - 1};

            const bool overlaps = newMin.x <= otherMax.x and newMax.x >= otherMin.x and
                newMin.y <= otherMax.y and newMax.y >= otherMin.y;
            if (overlaps) {
                return false;
            }
        }

        return true;
    }

    bool BaseState::tryPlace(BuildingType type, const int2& originCell)
    {
        if (not canPlaceAt(type, originCell)) {
            return false;
        }

        m_buildings.push_back({.type = type, .cell = originCell});
        return true;
    }

    void BaseState::drawBuilding(const PlacedBuilding& building) const
    {
        const BuildingDef& def = getBuildingDef(building.type);
        const p5::float2 topLeft = cellToWorld(m_grid, building.cell);
        const p5::float2 size = p5::float2 {.x = static_cast<float>(def.footprint.x), .y = static_cast<float>(def.footprint.y)} * m_grid.cellSize;

        p5::fill(def.fillColor);
        p5::stroke(def.strokeColor);
        p5::strokeWeight(2.0f);
        p5::rect(topLeft.x, topLeft.y, size.x, size.y, p5::BorderRadius::all(4.0f));
    }

    void BaseState::draw(std::optional<int2> hoveredCell, BuildingType previewType) const
    {
        drawGrid(m_grid);

        for (const PlacedBuilding& building : m_buildings) {
            drawBuilding(building);
        }

        if (not hoveredCell.has_value()) {
            return;
        }

        const BuildingDef& def = getBuildingDef(previewType);
        const bool valid = canPlaceAt(previewType, *hoveredCell);
        const p5::float2 topLeft = cellToWorld(m_grid, *hoveredCell);
        const p5::float2 size = p5::float2 {.x = static_cast<float>(def.footprint.x), .y = static_cast<float>(def.footprint.y)} * m_grid.cellSize;

        p5::fill(valid ? p5::rgba(80, 220, 100, 120) : p5::rgba(220, 60, 60, 120));
        p5::stroke(valid ? p5::rgba(120, 255, 140) : p5::rgba(255, 90, 90));
        p5::strokeWeight(2.0f);
        p5::rect(topLeft.x, topLeft.y, size.x, size.y, p5::BorderRadius::all(4.0f));
    }
} // namespace kingshot
