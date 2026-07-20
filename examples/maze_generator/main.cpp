#include <p5cpp/p5cpp.hpp>

using namespace p5cpp;

#include <stack>
#include <unordered_set>

struct Cell
{
    constexpr Cell(size_t column, size_t row)
        : column(column), row(row)
    {
    }

    size_t column;
    size_t row;
    bool hasLeftWall = true;
    bool hasTopWall = true;
    bool hasRightWall = true;
    bool hasBottomWall = true;

    void show(int cellWidth, int cellHeight) const
    {
        const float left = static_cast<float>(column * cellWidth);
        const float top = static_cast<float>(row * cellHeight);
        const float right = static_cast<float>((column + 1) * cellWidth);
        const float bottom = static_cast<float>((row + 1) * cellHeight);

        noFill();
        strokeWeight(2);
        stroke(0);

        if (hasLeftWall) line(left, top, left, bottom);
        if (hasTopWall) line(left, top, right, top);
        if (hasRightWall) line(right, top, right, bottom);
        if (hasBottomWall) line(left, bottom, right, bottom);
    }

    void buildWallsTo(Cell* neighbor)
    {
        if (const bool isLeftNeighbor = neighbor->column < column) {
            hasLeftWall = false;
            neighbor->hasRightWall = false;
        }

        if (const bool isTopNeighbor = neighbor->row < row) {
            hasTopWall = false;
            neighbor->hasBottomWall = false;
        }

        if (const bool isRightNeighbor = neighbor->column > column) {
            hasRightWall = false;
            neighbor->hasLeftWall = false;
        }

        if (const bool isBottomNeighbor = neighbor->row > row) {
            hasBottomWall = false;
            neighbor->hasTopWall = false;
        }
    }
};

class MazeGeneratorSketch : public Sketch
{
private:
    inline static constexpr int COLUMNS = 40;
    inline static constexpr int ROWS = 40;

    int cellWidth = 0;
    int cellHeight = 0;
    std::vector<Cell> grid;
    std::unordered_set<Cell*> visitedCells;
    std::stack<Cell*> visitorHistory;

    uint2 playerCell {0, 0};

public:
    void setup() override
    {
        setWindowSize(800, 800);
        // frameRate(20);
    }

    void event(const WindowEvent& event) override
    {
        handleResize(event);
        handlePlayerMovement(event);
    }

    void handleResize(const WindowEvent& event)
    {
        if (event.type == EventType::windowResize) {
            const int windowWidth = event.windowResize.width;
            const int windowHeight = event.windowResize.height;

            cellWidth = windowWidth / COLUMNS;
            cellHeight = windowHeight / ROWS;

            grid.clear();

            for (size_t y = 0; y < ROWS; ++y) {
                for (size_t x = 0; x < COLUMNS; ++x) {
                    grid.emplace_back(Cell {x, y});
                }
            }

            visitorHistory.push(&grid.at(0));
        }
    }

    void handlePlayerMovement(const WindowEvent& event)
    {
        if (event.type == EventType::keyPress) {
            if (event.keyEvent.key == Key::up) {
                if (Cell* currentCell = getNeighbor(playerCell.x, playerCell.y)) {
                    if (not currentCell->hasTopWall) {
                        playerCell.y -= 1;
                    }
                }
            } else if (event.keyEvent.key == Key::down) {
                if (Cell* currentCell = getNeighbor(playerCell.x, playerCell.y)) {
                    if (not currentCell->hasBottomWall) {
                        playerCell.y += 1;
                    }
                }
            } else if (event.keyEvent.key == Key::left) {
                if (Cell* currentCell = getNeighbor(playerCell.x, playerCell.y)) {
                    if (not currentCell->hasLeftWall) {
                        playerCell.x -= 1;
                    }
                }
            } else if (event.keyEvent.key == Key::right) {
                if (Cell* currentCell = getNeighbor(playerCell.x, playerCell.y)) {
                    if (not currentCell->hasRightWall) {
                        playerCell.x += 1;
                    }
                }
            }
        }
    }

    void draw() override
    {
        background(255);

        for (Cell* visitedCell : visitedCells) {
            highlightVisitedCell(visitedCell);
        }

        if (not visitorHistory.empty()) {
            Cell* currentVisitor = visitorHistory.top();
            highlightCell(currentVisitor);
        }

        for (size_t i = 0; i < 10; ++i) {
            if (not visitorHistory.empty()) {
                Cell* currentVisitor = visitorHistory.top();
                visitedCells.insert(currentVisitor);
            }

            if (not visitorHistory.empty()) {
                Cell* currentVisitor = visitorHistory.top();
                Cell* nextCellToVisit = getRandomUnvisitedNeighbor(currentVisitor);

                if (nextCellToVisit == nullptr) {
                    visitorHistory.pop();
                } else {
                    visitorHistory.push(nextCellToVisit);
                    currentVisitor->buildWallsTo(nextCellToVisit);
                }
            }
        }

        for (Cell& cell : grid) {
            cell.show(cellWidth, cellHeight);
        }

        highlightCellAsPlayer(playerCell.x, playerCell.y);
    }

private:
    void highlightCellAsPlayer(uint32_t playerColumn, uint32_t playerRow)
    {
        const float left = static_cast<float>(playerColumn * cellWidth);
        const float top = static_cast<float>(playerRow * cellHeight);
        const float horizontalPadding = 2.0f;
        const float verticalPadding = 2.0f;

        noStroke();
        fill(21, 255);
        rect(left + horizontalPadding, top + verticalPadding, cellWidth - horizontalPadding * 2.0f, cellHeight - verticalPadding * 2.0f);
    }

    void highlightCell(Cell* cell)
    {
        const float left = static_cast<float>(cell->column * cellWidth);
        const float top = static_cast<float>(cell->row * cellHeight);

        noStroke();
        fill(255, 0, 255, 255);
        rect(left, top, cellWidth, cellHeight);
    }

    void highlightVisitedCell(Cell* cell)
    {
        const float horizontalRatio = static_cast<float>(cell->column) / static_cast<float>(COLUMNS);
        const float verticalRatio = static_cast<float>(cell->row) / static_cast<float>(ROWS);
        const float hue = (horizontalRatio + verticalRatio) / 2.0f;
        const color_t cellColor = hsv(hue * 360.0f, 1.0f, 1.0f);

        const float left = static_cast<float>(cell->column * cellWidth);
        const float top = static_cast<float>(cell->row * cellHeight);

        noStroke();
        // fill(255, 0, 0, 100);
        fill(cellColor);
        rect(left, top, cellWidth, cellHeight);
    }

    Cell* getRandomUnvisitedNeighbor(Cell* currentVisitor)
    {
        std::vector<Cell*> unvisitedNeighbors = getUnvisitedNeighbors(currentVisitor);
        const int unvisitedNeighborsCount = static_cast<int>(unvisitedNeighbors.size());
        if (unvisitedNeighborsCount == 0) {
            return nullptr;
        }

        const int randomIndex = randomInt(0, unvisitedNeighborsCount - 1);
        return unvisitedNeighbors.at(randomIndex);
    }

    bool isCellVisited(Cell* cell)
    {
        return visitedCells.contains(cell);
    }

    std::vector<Cell*> getUnvisitedNeighbors(Cell* currentVisitor)
    {
        std::vector<Cell*> neighbors;
        neighbors.reserve(4);

        if (Cell* leftNeighbor = getNeighbor(currentVisitor->column - 1, currentVisitor->row)) {
            if (not isCellVisited(leftNeighbor)) {
                neighbors.push_back(leftNeighbor);
            }
        }

        if (Cell* topNeighbor = getNeighbor(currentVisitor->column, currentVisitor->row - 1)) {
            if (not isCellVisited(topNeighbor)) {
                neighbors.push_back(topNeighbor);
            }
        }

        if (Cell* rightNeighbor = getNeighbor(currentVisitor->column + 1, currentVisitor->row)) {
            if (not isCellVisited(rightNeighbor)) {
                neighbors.push_back(rightNeighbor);
            }
        }

        if (Cell* bottomNeighbor = getNeighbor(currentVisitor->column, currentVisitor->row + 1)) {
            if (not isCellVisited(bottomNeighbor)) {
                neighbors.push_back(bottomNeighbor);
            }
        }

        return neighbors;
    }

    Cell* getNeighbor(size_t column, size_t row)
    {
        if (column < 0 or column >= COLUMNS) return nullptr;
        if (row < 0 or row >= ROWS) return nullptr;

        const size_t index = row * COLUMNS + column;
        return &grid.at(index);
    }
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<MazeGeneratorSketch>();
    }
} // namespace p5cpp
