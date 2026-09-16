#include <p5cpp/p5cpp.hpp>
#include <p5cpp_animation/p5cpp_animation.hpp>
#include <p5cpp_gui/p5cpp_gui.hpp>

using namespace p5;
using namespace p5::animation;
using namespace p5::gui;

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

struct MatrixDisplayCell
{
    char character;
    ValueTweenTransition<float> opacity;
    ValueTweenTransition<float> highlight;
};

struct MatrixDisplay
{
    std::vector<MatrixDisplayCell> cells;

    int cellWidth;
    int cellHeight;
    int columns;
    int rows;

    color_t regularColor;
};

inline static void matrix_display_update(MatrixDisplay& display, float deltaTime)
{
    for (size_t i = 0; i < display.cells.size(); ++i) {
        MatrixDisplayCell& cell = display.cells[i];

        cell.opacity.advance(deltaTime);
        cell.highlight.advance(deltaTime);
    }
}

inline static int matrix_display_get_cell_index(const MatrixDisplay& display, int cellX, int cellY)
{
    return cellY * display.columns + cellX;
}

inline static void matrix_display_show(const MatrixDisplay& display)
{
    const float cellSize = static_cast<float>(std::min(display.cellWidth, display.cellHeight));

    push();
    noStroke();
    textAlign(TextAlignment::center);
    textSize(cellSize);

    for (size_t y = 0; y < display.rows; ++y) {
        for (int x = 0; x < display.columns; ++x) {
            const int cellIndex = matrix_display_get_cell_index(display, x, y);
            const MatrixDisplayCell& cell = display.cells[cellIndex];

            const float cellCenterX = static_cast<float>(x * display.cellWidth) + static_cast<float>(display.cellWidth) * 0.5f;
            const float cellCenterY = static_cast<float>(y * display.cellHeight) + static_cast<float>(display.cellHeight) * 0.5f;

            constexpr color_t highlightedColor = rgba(255);

            const color_t transparentColor = withOpacity(lerpColor(display.regularColor, highlightedColor, cell.highlight.value()), cell.opacity.value());

            fill(transparentColor);
            text(std::format("{}", cell.character), cellCenterX, cellCenterY);
        }
    }
    pop();
}

inline static bool matrix_display_is_valid_cell(const MatrixDisplay& display, int cellX, int cellY)
{
    return cellX >= 0 and cellX < display.columns and cellY >= 0 and cellY < display.rows;
}

inline static void matrix_display_despawn_cell(MatrixDisplay& display, int cellX, int cellY)
{
    if (not matrix_display_is_valid_cell(display, cellX, cellY)) {
        return;
    }

    const int cellIndex = matrix_display_get_cell_index(display, cellX, cellY);
    MatrixDisplayCell& cell = display.cells[cellIndex];

    cell.opacity = valueTween(1.0f, 0.0f, 0.5f, curves::easeLinear);
}

inline static void matrix_display_spawn_cell(MatrixDisplay& display, int cellX, int cellY)
{
    if (not matrix_display_is_valid_cell(display, cellX, cellY)) {
        return;
    }

    const int cellIndex = matrix_display_get_cell_index(display, cellX, cellY);
    MatrixDisplayCell& cell = display.cells[cellIndex];

    cell.character = get_random_character();
    cell.highlight = valueTween(1.0f, 0.0f, 0.25f, curves::easeLinear);
    cell.opacity = valueTween(0.0f, 1.0f, 0.1f, curves::easeLinear);
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

struct MatrixColumn
{
    int position;
    float stepInterval;
    float elapsedTimeSinceLastStep;

    std::array<Slice, 3> slices;
};

inline static int compute_random_slice_length(int rows)
{
    static constexpr int MIN_SLICE_LENGTH = 5;
    const int MAX_SLICE_LENGTH = MIN_SLICE_LENGTH + ((rows / 2) - MIN_SLICE_LENGTH);
    return static_cast<int>(std::floor(random(MIN_SLICE_LENGTH, MAX_SLICE_LENGTH)));
}

inline static int compute_random_slice_gap(int rows)
{
    static constexpr int MIN_SLICE_GAP = 10;
    const int MAX_SLICE_GAP = MIN_SLICE_GAP + ((rows / 2) - MIN_SLICE_GAP);
    return static_cast<int>(std::floor(random(MIN_SLICE_GAP, MAX_SLICE_GAP)));
}

inline static int matrix_column_get_topmost_upper_end(const MatrixColumn& column)
{
    int topmostUpperEnd = std::numeric_limits<int>::max();

    for (const Slice& slice : column.slices) {
        topmostUpperEnd = std::min(topmostUpperEnd, slice.upperEnd);
    }

    return topmostUpperEnd;
}

inline static void matrix_column_reset_slice(const MatrixColumn& column, Slice& slice, const MatrixDisplay& display)
{
    const int topmostUpperEnd = matrix_column_get_topmost_upper_end(column);
    const int gap = compute_random_slice_gap(display.rows);
    const int lowerEnd = std::min(topmostUpperEnd - gap, 0);

    const int length = compute_random_slice_length(display.rows);
    slice.lowerEnd = lowerEnd;
    slice.upperEnd = lowerEnd - length;
}

inline static void matrix_column_make_step(MatrixColumn& column, MatrixDisplay& display)
{
    for (Slice& slice : column.slices) {
        matrix_display_despawn_cell(display, column.position, slice.upperEnd);
        slice_make_step(slice);
        matrix_display_spawn_cell(display, column.position, slice.lowerEnd);

        const bool isOutOfSight = slice.upperEnd > display.rows;
        // info("UpperEnd > Rows ({} > {}) = {}", slice.upperEnd, display.rows, isOutOfSight);
        if (isOutOfSight) {
            matrix_column_reset_slice(column, slice, display);
        }
    }
}

inline static std::array<Slice, 3> spawn_slices(int rows)
{
    std::array<Slice, 3> slices;

    static constexpr int MIN_INITIAL_OFFSET = 0;
    const int MAX_INITIAL_OFFSET = rows / 2;
    int offset = static_cast<int>(std::floor(random(MIN_INITIAL_OFFSET, MAX_INITIAL_OFFSET)));

    for (size_t i = 0; i < slices.size(); ++i) {
        Slice& slice = slices.at(i);
        const int length = compute_random_slice_length(rows);

        slice.lowerEnd = 0 - offset;
        slice.upperEnd = slice.lowerEnd - length;

        const int gap = compute_random_slice_gap(rows);
        offset += (length + gap);
    }

    return slices;
}

inline static void matrix_column_update(MatrixColumn& column, MatrixDisplay& display, float deltaTime)
{
    column.elapsedTimeSinceLastStep += deltaTime;
    while (column.elapsedTimeSinceLastStep >= column.stepInterval) {
        matrix_column_make_step(column, display);
        column.elapsedTimeSinceLastStep -= column.stepInterval;
    }
}

struct Matrix : Sketch
{
    std::vector<MatrixColumn> matrixColumns;
    MatrixDisplay display;

    Font font = loadFont("fonts/Arial Rounded Bold.ttf").value();

    float colorR = 30.0f;
    float colorG = 220.0f;
    float colorB = 190.0f;
    bool showColorPanel = false;

    float cellWidth = 15;
    float cellHeight = 15;

    void setup() override
    {
        const int windowWidth = 800;
        const int windowHeight = 600;
        setWindowSize(windowWidth, windowHeight);
        rebuild(windowWidth, windowHeight);
    }

    inline static std::vector<MatrixDisplayCell> generate_matrix_display_cells(int columns, int rows)
    {
        MatrixDisplayCell initialCell = {
            .character = '\0',
            .opacity = valueTweenJump(0.0f),
            .highlight = valueTweenJump(0.0f),
        };

        return {static_cast<size_t>(columns * rows), initialCell};
    }

    void rebuild(int windowWidth, int windowHeight)
    {
        const int cellWidthInt = static_cast<int>(cellWidth);
        const int cellHeightInt = static_cast<int>(cellHeight);
        const int columns = (windowWidth + cellWidthInt - 1) / cellWidthInt;
        const int rows = (windowHeight + cellHeightInt - 1) / cellHeightInt;

        display = {
            .cells = generate_matrix_display_cells(columns, rows),
            .cellWidth = cellWidthInt,
            .cellHeight = cellHeightInt,
            .columns = columns,
            .rows = rows,
            .regularColor = rgba(static_cast<int32_t>(colorR), static_cast<int32_t>(colorG), static_cast<int32_t>(colorB)),
        };

        matrixColumns.resize(columns);
        for (size_t i = 0; i < matrixColumns.size(); ++i) {
            matrixColumns[i] = MatrixColumn {
                .position = static_cast<int>(i),
                .stepInterval = random(0.05f, 0.1f),
                .elapsedTimeSinceLastStep = 0.0f,
                .slices = spawn_slices(display.rows)
            };
        }
    }

    void event(const WindowEvent& event) override
    {
        event.on(
            [this](const WindowEvent::WindowResize& resize) {
                rebuild(resize.width, resize.height);
            },
            [this](const WindowEvent::KeyPress& keyPress) {
                if (keyPress.key == Key::C) {
                    showColorPanel = not showColorPanel;
                }
            }
        );
    }

    void draw() override
    {
        const float deltaTime = getDeltaTime();
        textFont(font);

        matrix_display_update(display, deltaTime);

        for (MatrixColumn& column : matrixColumns) {
            matrix_column_update(column, display, deltaTime);
        }

        display.regularColor = rgba(static_cast<int32_t>(colorR), static_cast<int32_t>(colorG), static_cast<int32_t>(colorB));

        background(rgba(31, 31, 51));
        matrix_display_show(display);

        with([] {
            noStroke();
            textSize(16.0f);
            textAlign(TextAlignment::topLeft);

            std::string message = "Press 'C' to open Control Panel";
            rect2f boundingBox = textBounds(message);
            constexpr float padding = 10.0f;
            boundingBox.left -= padding;
            boundingBox.top -= padding;
            boundingBox.width += padding * 2.0f;
            boundingBox.height += padding * 2.0f;

            translate(30.0f, 30.0f);
            fill(rgba(0, 150));
            rect(boundingBox.left, boundingBox.top, boundingBox.width, boundingBox.height);

            fill(rgba(255));
            text("Press 'C' to open Control Panel", 0.0f, 0.0f);
        },
             false);

        if (showColorPanel) {
            withGui([this] {
                mu_Context* ctx = getGuiContext();
                if (mu_begin_window_ex(ctx, "Color", mu_rect(50, 50, 320, 240), MU_OPT_NOCLOSE)) {
                    const int widths[] = {120, -1};
                    mu_layout_row(ctx, 2, widths, 0);
                    mu_label(ctx, "Red:");
                    mu_slider(ctx, &colorR, 0, 255);
                    mu_label(ctx, "Green:");
                    mu_slider(ctx, &colorG, 0, 255);
                    mu_label(ctx, "Blue:");
                    mu_slider(ctx, &colorB, 0, 255);

                    mu_label(ctx, "Cell Width:");
                    mu_slider(ctx, &cellWidth, 1, 40);
                    mu_label(ctx, "Cell Height:");
                    mu_slider(ctx, &cellHeight, 1, 40);

                    if (mu_button_ex(ctx, "Submit", 0, 0)) {
                        const auto [windowWidth, windowHeight] = getWindowSize();
                        rebuild(windowWidth, windowHeight);
                    }

                    mu_end_window(ctx);
                }
            });
        }
    }
};

SketchSpec p5::createSpec()
{
    return SketchSpec {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.emplace_back(gui::createGuiPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<Matrix>();
        },
    };
}
