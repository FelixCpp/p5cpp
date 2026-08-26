#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include <p5cpp/p5cpp.hpp>

#include "grid.hpp"

namespace kingshot
{
    enum class BuildingType
    {
        TownHall,
        Wall,
        Barracks,
        Farm,
        Count,
    };

    struct BuildingDef
    {
        BuildingType type;
        std::string_view name;
        p5::int2 footprint; // size in cells
        p5::color_t fillColor;
        p5::color_t strokeColor;
        bool placeableByPlayer; // TownHall is pre-placed and fixed, not in the build menu
    };

    const BuildingDef& getBuildingDef(BuildingType type);
    std::span<const BuildingDef> getBuildingCatalog();

    struct PlacedBuilding
    {
        BuildingType type;
        p5::int2 cell; // top-left cell of the footprint
    };

    // Owns the grid and everything built on it. Placement is the only mutation
    // supported for now -- no removal/upgrade yet, that's a later milestone.
    class BaseState
    {
    public:
        explicit BaseState(Grid grid);

        const Grid& getGrid() const;
        std::span<const PlacedBuilding> getBuildings() const;

        bool canPlaceAt(BuildingType type, const p5::int2& originCell) const;
        bool tryPlace(BuildingType type, const p5::int2& originCell);

        // hoveredCell/previewType are optional so the caller can skip the ghost
        // preview entirely (e.g. cursor outside the window).
        void draw(std::optional<p5::int2> hoveredCell, BuildingType previewType) const;

    private:
        Grid m_grid;
        std::vector<PlacedBuilding> m_buildings;

        void drawBuilding(const PlacedBuilding& building) const;
    };
} // namespace kingshot
