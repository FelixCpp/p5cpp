#include <p5cpp/p5cpp.hpp>

using namespace p5;

inline static constexpr int windowWidth = 800;
inline static constexpr int windowHeight = 600;
inline static constexpr int cellWidth = 15;
inline static constexpr int cellHeight = 15;
inline static constexpr int columns = windowWidth / cellWidth;
inline static constexpr int rows = windowHeight / cellHeight;

// clang-format off
inline static constexpr char characterSet[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    ';', ':', '$', '%', '(', '=', '?', ')', '!', '{', '}', '[', ']', '#', '+', '-', '*', '/',
};
// clang-format on

inline static char get_random_character()
{
    constexpr size_t characterSetLength = std::size(characterSet);
    const size_t randomIndex = static_cast<size_t>(std::floor(random(0.0f, static_cast<float>(characterSetLength))));
    return characterSet[randomIndex];
}

struct CharacterGrid
{
    std::unique_ptr<char[]> characters;
};

inline static void character_grid_randomize_character(CharacterGrid& grid, size_t cellX, size_t cellY)
{
    if (cellX < 0 or cellX >= columns or cellY < 0 or cellY >= rows) {
        return;
    }

    const size_t cellIndex = cellY * columns + cellX;
    grid.characters[cellIndex] = get_random_character();
}

inline static void character_grid_draw_cell(const CharacterGrid& grid, size_t cellX, size_t cellY, bool highlighted)
{
    if (cellX < 0 or cellX >= columns or cellY < 0 or cellY >= rows) {
        return;
    }

    const float cellCenterX = static_cast<float>(cellX * cellWidth) + static_cast<float>(cellWidth) * 0.5f;
    const float cellCenterY = static_cast<float>(cellY * cellHeight) + static_cast<float>(cellHeight) * 0.5f;

    const float cellSize = static_cast<float>(std::min(cellWidth, cellHeight));
    const size_t cellIndex = cellY * columns + cellX;
    const char character = grid.characters[cellIndex];

    constexpr color_t regularColor = rgba(30, 220, 190);
    constexpr color_t highlightedColor = rgba(255);

    fill(highlighted ? highlightedColor : regularColor);
    noStroke();
    textSize(cellSize);
    textAlign(TextAlignment::center);
    text(std::format("{}", character), cellCenterX, cellCenterY);
}

struct Slice
{
    int upperEnd;
    int lowerEnd;
};

inline static void slice_make_step(Slice& slice)
{
    ++slice.upperEnd;
    ++slice.lowerEnd;
}

inline static void slice_draw(const Slice& slice, const CharacterGrid& characterGrid, int column)
{
    for (int i = slice.upperEnd; i <= slice.lowerEnd; ++i) {
        const bool isHighlighted = i == slice.lowerEnd;
        character_grid_draw_cell(characterGrid, column, i, isHighlighted);
    }
}

struct MatrixColumn
{
    int position;
    float stepInterval;
    float elapsedTimeSinceLastStep;

    std::array<Slice, 3> slices;
};

inline static void matrix_column_make_step(MatrixColumn& column, CharacterGrid& grid)
{
    for (size_t i = 0; i < column.slices.size(); ++i) {
        Slice& slice = column.slices.at(i);

        slice_make_step(slice);
        character_grid_randomize_character(grid, column.position, slice.lowerEnd);
    }
}

inline static int compute_random_slice_length()
{
    static constexpr int MIN_SLICE_LENGTH = 5;
    static constexpr int MAX_SLICE_LENGTH = MIN_SLICE_LENGTH + ((rows / 2) - MIN_SLICE_LENGTH);
    return static_cast<int>(std::floor(random(MIN_SLICE_LENGTH, MAX_SLICE_LENGTH)));
}

inline static int compute_random_slice_gap()
{
    static constexpr int MIN_SLICE_GAP = 3;
    static constexpr int MAX_SLICE_GAP = MIN_SLICE_GAP + ((rows / 4) - MIN_SLICE_GAP);
    return static_cast<int>(std::floor(random(MIN_SLICE_GAP, MAX_SLICE_GAP)));
}

inline static std::array<Slice, 3> spawn_slices()
{
    std::array<Slice, 3> slices;

    static constexpr size_t MIN_INITIAL_OFFSET = 0;
    static constexpr size_t MAX_INITIAL_OFFSET = rows / 2;
    size_t offset = static_cast<size_t>(std::floor(random(MIN_INITIAL_OFFSET, MAX_INITIAL_OFFSET)));

    for (size_t i = 0; i < slices.size(); ++i) {
        Slice& slice = slices.at(i);
        const int length = compute_random_slice_length();

        slice.lowerEnd = 0 - offset;
        slice.upperEnd = slice.lowerEnd - length;

        const int gap = compute_random_slice_gap();
        offset += (length + gap);
    }

    return slices;
}

inline static void matrix_column_reset_slice(const MatrixColumn& column, Slice& slice)
{
    int highestUpperEnd = std::numeric_limits<int>::max();
    for (size_t i = 0; i < column.slices.size(); ++i) {
        highestUpperEnd = std::min(highestUpperEnd, column.slices.at(i).upperEnd);
    }

    const int gap = compute_random_slice_gap();
    const int lowerEnd = std::min(highestUpperEnd - gap, 0);

    const int length = compute_random_slice_length();
    slice.lowerEnd = lowerEnd;
    slice.upperEnd = slice.lowerEnd - length;
}

inline static void matrix_column_update_slices(MatrixColumn& column)
{
    for (size_t i = 0; i < column.slices.size(); ++i) {
        Slice& slice = column.slices.at(i);
        const bool isOutOfSight = slice.upperEnd > rows;

        if (isOutOfSight) {
            matrix_column_reset_slice(column, slice);
        }
    }
}

inline static void matrix_column_update(MatrixColumn& column, CharacterGrid& characterGrid, float deltaTime)
{
    column.elapsedTimeSinceLastStep += deltaTime;
    while (column.elapsedTimeSinceLastStep >= column.stepInterval) {
        matrix_column_make_step(column, characterGrid);
        column.elapsedTimeSinceLastStep -= column.stepInterval;
    }

    matrix_column_update_slices(column);
}

inline static void matrix_column_draw(const MatrixColumn& column, const CharacterGrid& characterGrid)
{
    for (size_t i = 0; i < column.slices.size(); ++i) {
        slice_draw(column.slices.at(i), characterGrid, column.position);
    }
}

struct Matrix : Sketch
{
    std::array<MatrixColumn, columns> matrixColumns;
    CharacterGrid grid;

    void setup() override
    {
        setWindowSize(windowWidth, windowHeight);
        setWindowResizable(false);

        grid = CharacterGrid {
            .characters = std::make_unique<char[]>(columns * rows),
        };

        for (size_t i = 0; i < matrixColumns.size(); ++i) {
            matrixColumns[i] = MatrixColumn {
                .position = static_cast<int>(i),
                .stepInterval = random(0.025f, 0.05f),
                .elapsedTimeSinceLastStep = 0.0f,
                .slices = spawn_slices()
            };
        }
    }

    Font font = loadFont("fonts/Arial Rounded Bold.ttf").value();

    void draw() override
    {
        const float deltaTime = getDeltaTime();
        // textFont(font);

        for (MatrixColumn& column : matrixColumns) {
            matrix_column_update(column, grid, deltaTime);
        }

        background(rgba(31, 31, 51));

        for (MatrixColumn& column : matrixColumns) {
            matrix_column_draw(column, grid);
        }

        if (false)
            draw_debug_grid();
    }

    void draw_debug_grid()
    {
        for (size_t y = 0; y < rows; ++y) {
            for (size_t x = 0; x < columns; ++x) {
                const float px = static_cast<float>(x * cellWidth);
                const float py = static_cast<float>(y * cellHeight);
                const float width = static_cast<float>(cellWidth);
                const float height = static_cast<float>(cellHeight);

                noFill();
                stroke(rgba(255, 100));
                strokeWeight(1.0f);
                rect(px, py, width, height);

                const size_t index = y * columns + x;
                const char character = grid.characters[index];

                // textSize(cellWidth);
                // fill(rgba(255, 100));
                // noStroke();
                // textAlign(TextAlignment::center);
                // text(std::format("{}", character), px + cellWidth / 2, py + cellHeight / 2);
            }
        }
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            // plugins.emplace_back(gui::createGuiPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<Matrix>();
        },
    };
}
