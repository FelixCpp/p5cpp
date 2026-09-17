#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
#include <p5cpp_gif/p5cpp_gif.hpp>

using namespace p5;
using namespace p5::animation;
using namespace p5::gif;

enum class ShapeType
{
    o,
    i,
    t,
    l,
    j,
    s,
    z,
};

enum class Rotation
{
    north,
    east,
    south,
    west
};

Rotation rotated_forwards(Rotation rotation)
{
    switch (rotation) {
        case Rotation::north: return Rotation::east;
        case Rotation::east: return Rotation::south;
        case Rotation::south: return Rotation::west;
        case Rotation::west: return Rotation::north;
    }
}

Rotation rotated_backwards(Rotation rotation)
{
    switch (rotation) {
        case Rotation::north: return Rotation::west;
        case Rotation::east: return Rotation::north;
        case Rotation::south: return Rotation::east;
        case Rotation::west: return Rotation::south;
    }
}

typedef std::array<int, 16> ShapeData;

struct Shape
{
    color_t color;
    ShapeData data;
};

inline static ShapeData get_o_shape(Rotation rotation)
{
    // clang-format off
    return {
        0, 0, 0, 0,
        0, 1, 1, 0,
        0, 1, 1, 0,
        0, 0, 0, 0
    };
    // clang-format on
}

inline static ShapeData get_i_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 0, 0, 0,
                1, 1, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 1, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 0, 0, 0,
                1, 1, 1, 1,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 1, 0, 0,
                0, 1, 0, 0,
                0, 1, 0, 0,
                0, 1, 0, 0,
            };
            break;
    }
    // clang-format on
}

inline static ShapeData get_s_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 0, 1, 1,
                0, 1, 1, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 1, 0,
                0, 0, 1, 1,
                0, 0, 0, 1,
                0, 0, 0, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 0, 1, 1,
                0, 1, 1, 0,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 1, 0, 0,
                0, 1, 1, 0,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
            break;
    }
    // clang-format on
}

inline static ShapeData get_z_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 1, 1, 0,
                0, 0, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 0, 1,
                0, 0, 1, 1,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 1, 1, 0,
                0, 0, 1, 1,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 0, 1, 0,
                0, 1, 1, 0,
                0, 1, 0, 0,
                0, 0, 0, 0,
            };
            break;
    }
    // clang-format on
}
inline static ShapeData get_t_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 0, 1, 0,
                0, 1, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 1, 0,
                0, 0, 1, 1,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 1, 1, 1,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 0, 1, 0,
                0, 1, 1, 0,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
            break;
    }
    // clang-format on
}

inline static ShapeData get_l_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 0, 0, 1,
                0, 1, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 1, 1,
                0, 0, 0, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 1, 1, 1,
                0, 1, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 1, 1, 0,
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
            break;
    }
    // clang-format on
}

inline static ShapeData get_j_shape(Rotation rotation)
{
    // clang-format off
    switch (rotation)
    {
        case Rotation::north:
            return {
                0, 1, 0, 0,
                0, 1, 1, 1,
                0, 0, 0, 0,
                0, 0, 0, 0,
            };
        case Rotation::east:
            return {
                0, 0, 1, 1,
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 0, 0, 0,
            };
        case Rotation::south:
            return {
                0, 0, 0, 0,
                0, 1, 1, 1,
                0, 0, 0, 1,
                0, 0, 0, 0,
            };
        case Rotation::west:
            return {
                0, 0, 1, 0,
                0, 0, 1, 0,
                0, 1, 1, 0,
                0, 0, 0, 0,
            };
            break;
    }
    // clang-format on
}

Shape getShape(ShapeType type, Rotation rotation)
{
    switch (type) {
        case ShapeType::o:
            return {
                .color = rgba(240, 210, 60),
                .data = get_o_shape(rotation),
            };

        case ShapeType::i:
            return {
                .color = rgba(70, 220, 230),
                .data = get_i_shape(rotation),
            };
        case ShapeType::t:
            return {
                .color = rgba(190, 100, 230),
                .data = get_t_shape(rotation),
            };
        case ShapeType::l:
            return {
                .color = rgba(230, 150, 60),
                .data = get_l_shape(rotation),
            };

        case ShapeType::j:
            return {
                .color = rgba(80, 120, 235),
                .data = get_j_shape(rotation),
            };
        case ShapeType::s:
            return {
                .color = rgba(90, 210, 110),
                .data = get_s_shape(rotation),
            };
        case ShapeType::z:
            return {
                .color = rgba(230, 90, 100),
                .data = get_z_shape(rotation),
            };
    }
}

inline static constexpr std::array<ShapeType, 7> allShapeTypes = {
    ShapeType::o,
    ShapeType::i,
    ShapeType::t,
    ShapeType::l,
    ShapeType::j,
    ShapeType::s,
    ShapeType::z,
};

ShapeType draw_from_bag(std::vector<ShapeType>& bag)
{
    if (bag.empty()) {
        bag.assign(allShapeTypes.begin(), allShapeTypes.end());
        for (size_t i = bag.size() - 1; i > 0; --i) {
            const size_t j = static_cast<size_t>(std::floor(random(0.0f, static_cast<float>(i + 1))));
            std::swap(bag[i], bag[j]);
        }
    }

    const ShapeType type = bag.back();
    bag.pop_back();
    return type;
}

struct CellState
{
    bool isFilled;
    color_t color;
};

enum class GameState
{
    playing,
    paused,
    clearingLines,
    gameOver,
};

struct HeldDirection
{
    bool wasDown = false;
    float dasTimer = 0.0f;
    float arrTimer = 0.0f;
};

struct Tetris : Sketch
{
    inline static constexpr int boardColumns = 10;
    inline static constexpr int boardRows = 20;
    inline static constexpr int cellSize = 32;
    inline static constexpr int boardMargin = 30;
    inline static constexpr int panelWidth = 220;
    inline static constexpr int panelGap = 30;

    inline static constexpr int boardPixelWidth = boardColumns * cellSize;
    inline static constexpr int boardPixelHeight = boardRows * cellSize;
    inline static constexpr int boardX = boardMargin;
    inline static constexpr int boardY = boardMargin;
    inline static constexpr int panelX = boardX + boardPixelWidth + panelGap;

    inline static constexpr int windowWidth = panelX + panelWidth + boardMargin;
    inline static constexpr int windowHeight = boardY + boardPixelHeight + boardMargin;

    inline static constexpr float baseDropInterval = 0.8f;
    inline static constexpr float minDropInterval = 0.1f;
    inline static constexpr float dropIntervalStep = 0.07f;
    inline static constexpr float softDropInterval = 0.045f;
    inline static constexpr float dasDelay = 0.15f;
    inline static constexpr float arrInterval = 0.035f;
    inline static constexpr float lineClearDuration = 0.28f;

    Font uiFont = loadFont("fonts/Lexend_Deca/static/LexendDeca-Bold.ttf").value();

    GameState state = GameState::playing;

    std::vector<ShapeType> bag;
    ShapeType currentShape = ShapeType::i;
    ShapeType nextShape = ShapeType::i;
    Rotation currentRotation = Rotation::north;
    int offsetX = 0;
    int offsetY = 0;
    float dropElapsed = 0.0f;

    HeldDirection leftHeld;
    HeldDirection rightHeld;

    std::vector<CellState> cells;
    std::vector<int> clearingRows;
    TweenTransition lineClearTween = tween(lineClearDuration, curves::easeOutQuad);

    int score = 0;
    int linesCleared = 0;
    int level = 1;

    void setup() override
    {
        setWindowSize(windowWidth, windowHeight);
        setWindowResizable(false);
        setWindowTitle("Tetris");
        resetGame();
    }

    void resetGame()
    {
        cells.assign(static_cast<size_t>(boardColumns * boardRows), CellState {.isFilled = false, .color = rgba(0, 0)});
        bag.clear();
        clearingRows.clear();
        leftHeld = {};
        rightHeld = {};
        score = 0;
        linesCleared = 0;
        level = 1;

        nextShape = draw_from_bag(bag);
        spawnPiece();
    }

    Shape currentShapeData() const { return getShape(currentShape, currentRotation); }

    float currentDropInterval() const { return std::max(minDropInterval, baseDropInterval - static_cast<float>(level - 1) * dropIntervalStep); }

    bool canPlace(const ShapeData& shape, int testOffsetX, int testOffsetY) const
    {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (not shape[y * 4 + x]) {
                    continue;
                }

                const int px = testOffsetX + x;
                const int py = testOffsetY + y;
                if (px < 0 or px >= boardColumns or py >= boardRows) {
                    return false;
                }
                if (py >= 0 and cells[py * boardColumns + px].isFilled) {
                    return false;
                }
            }
        }
        return true;
    }

    void spawnPiece()
    {
        currentShape = nextShape;
        nextShape = draw_from_bag(bag);
        currentRotation = Rotation::north;
        offsetX = (boardColumns - 4) / 2;
        offsetY = 0;
        dropElapsed = 0.0f;

        state = canPlace(currentShapeData().data, offsetX, offsetY) ? GameState::playing : GameState::gameOver;
    }

    void tryMove(int dx, int dy)
    {
        if (canPlace(currentShapeData().data, offsetX + dx, offsetY + dy)) {
            offsetX += dx;
            offsetY += dy;
        }
    }

    bool tryMoveDown()
    {
        if (not canPlace(currentShapeData().data, offsetX, offsetY + 1)) {
            return false;
        }
        ++offsetY;
        return true;
    }

    void tryRotate(bool clockwise)
    {
        const Rotation newRotation = clockwise ? rotated_forwards(currentRotation) : rotated_backwards(currentRotation);
        const ShapeData newShapeData = getShape(currentShape, newRotation).data;

        static constexpr std::array<int, 5> kicks = {0, -1, 1, -2, 2};
        for (const int kick : kicks) {
            if (canPlace(newShapeData, offsetX + kick, offsetY)) {
                currentRotation = newRotation;
                offsetX += kick;
                return;
            }
        }
    }

    int hardDropDistance() const
    {
        int distance = 0;
        while (canPlace(currentShapeData().data, offsetX, offsetY + distance + 1)) {
            ++distance;
        }
        return distance;
    }

    void hardDrop()
    {
        const int distance = hardDropDistance();
        offsetY += distance;
        score += distance * 2;
        lockPiece();
    }

    void lockPiece()
    {
        const Shape shape = currentShapeData();
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (not shape.data[y * 4 + x]) {
                    continue;
                }
                cells[(offsetY + y) * boardColumns + (offsetX + x)] = CellState {.isFilled = true, .color = shape.color};
            }
        }

        clearingRows.clear();
        for (int y = 0; y < boardRows; ++y) {
            bool full = true;
            for (int x = 0; x < boardColumns; ++x) {
                if (not cells[y * boardColumns + x].isFilled) {
                    full = false;
                    break;
                }
            }
            if (full) {
                clearingRows.push_back(y);
            }
        }

        if (clearingRows.empty()) {
            spawnPiece();
        } else {
            lineClearTween.restart();
            state = GameState::clearingLines;
        }
    }

    void finishLineClear()
    {
        std::vector<CellState> newCells(cells.size(), CellState {.isFilled = false, .color = rgba(0, 0)});

        int writeRow = boardRows - 1;
        for (int y = boardRows - 1; y >= 0; --y) {
            if (std::find(clearingRows.begin(), clearingRows.end(), y) != clearingRows.end()) {
                continue;
            }
            for (int x = 0; x < boardColumns; ++x) {
                newCells[writeRow * boardColumns + x] = cells[y * boardColumns + x];
            }
            --writeRow;
        }
        cells = std::move(newCells);

        const int cleared = static_cast<int>(clearingRows.size());
        linesCleared += cleared;
        level = linesCleared / 10 + 1;

        static constexpr std::array<int, 5> lineScores = {0, 100, 300, 500, 800};
        score += lineScores[std::min(cleared, 4)] * level;

        clearingRows.clear();
        spawnPiece();
    }

    void updateHorizontalRepeat(HeldDirection& heldDirection, bool keyDown, float deltaTime, const std::function<void()>& moveFn)
    {
        if (not keyDown) {
            heldDirection = {};
            return;
        }

        if (not heldDirection.wasDown) {
            heldDirection = {.wasDown = true, .dasTimer = 0.0f, .arrTimer = 0.0f};
            moveFn();
            return;
        }

        heldDirection.dasTimer += deltaTime;
        if (heldDirection.dasTimer < dasDelay) {
            return;
        }

        heldDirection.arrTimer += deltaTime;
        while (heldDirection.arrTimer >= arrInterval) {
            heldDirection.arrTimer -= arrInterval;
            moveFn();
        }
    }

    void updateInput(float deltaTime)
    {
        updateHorizontalRepeat(leftHeld, isKeyDown(Key::Left), deltaTime, [this] {
            tryMove(-1, 0);
        });
        updateHorizontalRepeat(rightHeld, isKeyDown(Key::Right), deltaTime, [this] {
            tryMove(1, 0);
        });

        if (isKeyPressed(Key::Up) or isKeyPressed(Key::X)) {
            tryRotate(true);
        }
        if (isKeyPressed(Key::Z)) {
            tryRotate(false);
        }
        if (isKeyPressed(Key::Space)) {
            hardDrop();
        }
    }

    void updateGravity(float deltaTime)
    {
        const bool softDropping = isKeyDown(Key::Down);
        const float interval = softDropping ? softDropInterval : currentDropInterval();

        dropElapsed += deltaTime;
        while (state == GameState::playing and dropElapsed >= interval) {
            dropElapsed -= interval;
            if (tryMoveDown()) {
                if (softDropping) {
                    ++score;
                }
            } else {
                lockPiece();
            }
        }
    }

    void draw() override
    {
        const float deltaTime = static_cast<float>(getDeltaTime());
        textFont(uiFont);

        // if (isKeyPressed(Key::G)) {
        //     if (recording.has_value() and recording->isActive()) {
        //         recording->cancel();
        //     } else {
        //         recording = recordGif("tetris.gif", recordUntil([](float) {
        //                                   return false;
        //                               }),
        //                               {.framesPerSecond = 20.0f});
        //     }
        // }

        if (state != GameState::gameOver and isKeyPressed(Key::P)) {
            state = (state == GameState::paused) ? GameState::playing : GameState::paused;
        }
        if (state == GameState::gameOver and isKeyPressed(Key::Enter)) {
            resetGame();
        }

        if (state == GameState::playing) {
            updateInput(deltaTime);
            updateGravity(deltaTime);
        } else if (state == GameState::clearingLines) {
            if (lineClearTween.advance(deltaTime).isCompleted) {
                finishLineClear();
            }
        }

        background(rgba(16, 16, 24));
        renderBoardPanel();
        renderBoard();
        renderSidePanel();

        if (state == GameState::paused) {
            renderOverlay("PAUSED", "Press P to resume");
        } else if (state == GameState::gameOver) {
            renderOverlay("GAME OVER", std::format("Score {}\nPress Enter to restart", score));
        }
    }

    void renderBoardPanel()
    {
        constexpr float outline = 8.0f;

        fill(rgba(26, 26, 38));
        stroke(rgba(255, 255, 255, 22));
        strokeWeight(1.0f);
        rect(boardX - outline, boardY - outline, boardPixelWidth + outline * 2, boardPixelHeight + outline * 2, BorderRadius::all(10.0f));
    }

    void drawBlock(float px, float py, color_t color, float size = cellSize, float alpha = 1.0f)
    {
        constexpr float inset = 2.0f;
        const float radius = size * 0.18f;

        fill(withOpacity(color, alpha));
        stroke(withOpacity(rgba(0, 0, 0, 90), alpha));
        strokeWeight(1.0f);
        rect(px + inset, py + inset, size - inset * 2, size - inset * 2, BorderRadius::all(radius));

        noStroke();
        fill(withOpacity(rgba(255), alpha * 0.3f));
        rect(px + inset * 2, py + inset * 2, size - inset * 4, (size - inset * 4) * 0.4f, BorderRadius::all(radius * 0.6f));
    }

    void drawGhostBlock(float px, float py, color_t color)
    {
        constexpr float inset = 2.0f;
        const float radius = cellSize * 0.18f;

        noFill();
        stroke(withOpacity(color, 0.85f));
        strokeWeight(2.0f);
        rect(px + inset, py + inset, cellSize - inset * 2, cellSize - inset * 2, BorderRadius::all(radius));
    }

    void renderBoard()
    {
        withClip(static_cast<float>(boardX), static_cast<float>(boardY), static_cast<float>(boardPixelWidth), static_cast<float>(boardPixelHeight), [this] {
            noFill();
            stroke(rgba(255, 255, 255, 14));
            strokeWeight(1.0f);
            for (int x = 1; x < boardColumns; ++x) {
                const float lineX = static_cast<float>(boardX + x * cellSize);
                line(lineX, static_cast<float>(boardY), lineX, static_cast<float>(boardY + boardPixelHeight));
            }
            for (int y = 1; y < boardRows; ++y) {
                const float lineY = static_cast<float>(boardY + y * cellSize);
                line(static_cast<float>(boardX), lineY, static_cast<float>(boardX + boardPixelWidth), lineY);
            }

            for (int y = 0; y < boardRows; ++y) {
                for (int x = 0; x < boardColumns; ++x) {
                    const CellState& cellState = cells[y * boardColumns + x];
                    if (not cellState.isFilled) {
                        continue;
                    }
                    drawBlock(static_cast<float>(boardX + x * cellSize), static_cast<float>(boardY + y * cellSize), cellState.color);
                }
            }

            if (state == GameState::playing) {
                const Shape shape = currentShapeData();

                int ghostOffsetY = offsetY;
                while (canPlace(shape.data, offsetX, ghostOffsetY + 1)) {
                    ++ghostOffsetY;
                }

                if (ghostOffsetY != offsetY) {
                    for (int y = 0; y < 4; ++y) {
                        for (int x = 0; x < 4; ++x) {
                            if (shape.data[y * 4 + x]) {
                                drawGhostBlock(static_cast<float>(boardX + (offsetX + x) * cellSize), static_cast<float>(boardY + (ghostOffsetY + y) * cellSize), shape.color);
                            }
                        }
                    }
                }

                for (int y = 0; y < 4; ++y) {
                    for (int x = 0; x < 4; ++x) {
                        if (shape.data[y * 4 + x]) {
                            drawBlock(static_cast<float>(boardX + (offsetX + x) * cellSize), static_cast<float>(boardY + (offsetY + y) * cellSize), shape.color);
                        }
                    }
                }
            }

            if (state == GameState::clearingLines) {
                const float flash = 1.0f - lineClearTween.progress();
                noStroke();
                fill(withOpacity(rgba(255), flash));
                for (const int row : clearingRows) {
                    rect(static_cast<float>(boardX), static_cast<float>(boardY + row * cellSize), static_cast<float>(boardPixelWidth), static_cast<float>(cellSize));
                }
            }
        });
    }

    template <std::invocable ContentFn> void renderPanelBox(float x, float y, float w, float h, std::string_view title, ContentFn&& contentFn)
    {
        fill(rgba(26, 26, 38));
        stroke(rgba(255, 255, 255, 22));
        strokeWeight(1.0f);
        rect(x, y, w, h, BorderRadius::all(10.0f));

        withMatrix([&] {
            translate(x + 18.0f, y + 18.0f);

            noStroke();
            fill(rgba(140, 150, 175));
            textAlign(TextAlignment::topLeft);
            textSize(13.0f);
            text(std::string(title), 0.0f, 0.0f);

            translate(0.0f, 26.0f);
            contentFn();
        });
    }

    void renderNextPiecePreview()
    {
        const Shape shape = getShape(nextShape, Rotation::north);

        int minX = 4, maxX = -1, minY = 4, maxY = -1;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (shape.data[y * 4 + x]) {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
        }

        constexpr float previewCell = 24.0f;
        constexpr float areaWidth = panelWidth - 36.0f;
        constexpr float areaHeight = 96.0f;

        const float shapeWidth = static_cast<float>(maxX - minX + 1) * previewCell;
        const float shapeHeight = static_cast<float>(maxY - minY + 1) * previewCell;
        const float startX = (areaWidth - shapeWidth) * 0.5f;
        const float startY = (areaHeight - shapeHeight) * 0.5f;

        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (not shape.data[y * 4 + x]) {
                    continue;
                }
                const float px = startX + static_cast<float>(x - minX) * previewCell;
                const float py = startY + static_cast<float>(y - minY) * previewCell;
                drawBlock(px, py, shape.color, previewCell);
            }
        }
    }

    void renderStatRow(float y, std::string_view label, const std::string& value)
    {
        withMatrix([&] {
            translate(0.0f, y);
            noStroke();
            textAlign(TextAlignment::topLeft);

            fill(rgba(140, 150, 175));
            textSize(12.0f);
            text(std::string(label), 0.0f, 0.0f);

            fill(rgba(235, 235, 245));
            textSize(22.0f);
            text(value, 0.0f, 16.0f);
        });
    }

    void renderHeader()
    {
        withMatrix([&] {
            translate(static_cast<float>(panelX), static_cast<float>(boardY));
            noStroke();
            textAlign(TextAlignment::topLeft);
            fill(rgba(110, 195, 255));
            textSize(30.0f);
            text("TETRIS", 0.0f, 0.0f);
        });
    }

    void renderControlsHint(float y)
    {
        withMatrix([&] {
            translate(static_cast<float>(panelX), y);
            noStroke();
            fill(rgba(120, 128, 150));
            textAlign(TextAlignment::topLeft);
            textSize(12.0f);
            textLeading(19.0f);
            text("LEFT / RIGHT  move\nUP  rotate\nDOWN  soft drop\nSPACE  hard drop\nP  pause", 0.0f, 0.0f, static_cast<float>(panelWidth));
        });
    }

    void renderSidePanel()
    {
        constexpr float headerHeight = 50.0f;
        constexpr float nextHeight = 150.0f;
        constexpr float statsHeight = 220.0f;
        constexpr float boxGap = 20.0f;

        renderHeader();

        const float nextY = static_cast<float>(boardY) + headerHeight + boxGap;
        renderPanelBox(static_cast<float>(panelX), nextY, static_cast<float>(panelWidth), nextHeight, "NEXT", [this] {
            renderNextPiecePreview();
        });

        const float statsY = nextY + nextHeight + boxGap;
        renderPanelBox(static_cast<float>(panelX), statsY, static_cast<float>(panelWidth), statsHeight, "STATS", [this] {
            renderStatRow(18.0f, "SCORE", std::to_string(score));
            renderStatRow(62.0f, "LINES", std::to_string(linesCleared));
            renderStatRow(106.0f, "LEVEL", std::to_string(level));
        });

        renderControlsHint(statsY + statsHeight + boxGap);
    }

    void renderOverlay(std::string_view title, const std::string& subtitle)
    {
        noStroke();
        fill(rgba(10, 10, 16, 190));
        rect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));

        textAlign(TextAlignment::center);

        fill(rgba(235, 235, 245));
        textSize(36.0f);
        text(std::string(title), windowWidth * 0.5f, windowHeight * 0.5f - 24.0f);

        fill(rgba(170, 178, 200));
        textSize(16.0f);
        textLeading(22.0f);
        text(subtitle, windowWidth * 0.5f, windowHeight * 0.5f + 24.0f);
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;

            plugins.push_back(
                gif::createGifRecorderPlugin(
                    RecordingShortcutOptions {
                        .saveFilepath = "tetris.gif",
                        .toggleRecordingKey = Key::G,
                        .recordingOptions = {
                            .framesPerSecond = 20.0f,
                        },
                    }
                )
            );

            return plugins;
        },
        .sketch = [] {
            return std::make_unique<Tetris>();
        }
    };
}
