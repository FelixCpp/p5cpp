// mesh_demo.cpp
// -----------------------------------------------------------------------
// Demonstrates mesh(): raw indexed-triangle submission alongside p5cpp's
// usual state-driven draw calls (rect()/circle()/beginShape()-vertex()-
// endShape()/...). Unlike those, mesh() takes explicit vertex positions/
// UVs/colors and a flat triangle-index list — the escape hatch for
// geometry beginShape()/vertex()/endShape() can't express efficiently:
// shared vertices between triangles.
//
// Both meshes below are NxM grids where interior vertices are shared by
// up to 6 triangles - exactly the case that would force point duplication
// in immediate-mode beginShape()/vertex()/endShape().
//
//   - Left mesh    animated wave grid, no texture, per-vertex rainbow
//                  color (mesh(vertices, indices) overload) - proves
//                  vertex reuse *and* per-vertex coloring, neither of
//                  which beginShape()/vertex() can do.
//   - Right mesh   static grid with a checkerboard texture and a cycling
//                  tint() (mesh(vertices, indices, texture) overload) -
//                  proves UV/texture/tint modulation matches image().
//
// Both are drawn under pushMatrix()/translate()/rotate(), proving mesh()
// respects the active transform stack like every other draw call.
//
// Steuerung:
//   Escape - beenden
// -----------------------------------------------------------------------

#include <p5cpp/p5cpp.hpp>

#include <cmath>
#include <vector>

using namespace p5cpp;

namespace
{
    constexpr int GRID_COLS = 24;
    constexpr int GRID_ROWS = 16;
    constexpr float MESH_WIDTH = 360.0f;
    constexpr float MESH_HEIGHT = 240.0f;

    struct GridMesh
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
    };

    // Builds a (cols+1)x(rows+1) vertex grid centered on the origin, with two
    // triangles per cell reusing each interior vertex up to 6 times total.
    GridMesh makeGrid(int cols, int rows, float width, float height)
    {
        GridMesh grid;
        grid.vertices.reserve(static_cast<size_t>((cols + 1) * (rows + 1)));

        for (int y = 0; y <= rows; ++y) {
            for (int x = 0; x <= cols; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(cols);
                const float v = static_cast<float>(y) / static_cast<float>(rows);
                grid.vertices.push_back(MeshVertex {
                    .position = {u * width - width * 0.5f, v * height - height * 0.5f},
                    .texcoord = {u, v},
                });
            }
        }

        grid.indices.reserve(static_cast<size_t>(cols * rows * 6));
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                const uint32_t i0 = static_cast<uint32_t>(y * (cols + 1) + x);
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = i0 + static_cast<uint32_t>(cols + 1);
                const uint32_t i3 = i2 + 1;
                grid.indices.insert(grid.indices.end(), {i0, i2, i1, i1, i2, i3});
            }
        }

        return grid;
    }

    Texture makeCheckerTexture()
    {
        constexpr uint32_t size = 8;
        color_t pixels[size * size];
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                const bool even = ((x + y) % 2) == 0;
                pixels[y * size + x] = even ? rgba(255, 255) : rgba(60, 255);
            }
        }
        return Texture(loadTexture(size, size, pixels));
    }
} // namespace

class MeshDemoSketch : public Sketch
{
public:
    void setup() override
    {
        setWindowSize(960, 540);
        setWindowTitle("p5cpp - mesh() demo");
        setWindowResizable(false);
        frameRate(60);

        waveGrid = makeGrid(GRID_COLS, GRID_ROWS, MESH_WIDTH, MESH_HEIGHT);
        checkerGrid = makeGrid(GRID_COLS, GRID_ROWS, MESH_WIDTH, MESH_HEIGHT);
        checkerTexture = makeCheckerTexture();
    }

    void event(const WindowEvent& e) override
    {
        if (e.type == EventType::keyPress && e.keyEvent.key == Key::escape) {
            quit();
        }
    }

    void draw() override
    {
        background(12, 14, 20, 255);
        const float t = getGlobalTime();

        // Animate the wave grid's vertex positions and per-vertex color each
        // frame - both are only possible via mesh()'s explicit vertex data.
        for (int y = 0; y <= GRID_ROWS; ++y) {
            for (int x = 0; x <= GRID_COLS; ++x) {
                const size_t i = static_cast<size_t>(y * (GRID_COLS + 1) + x);
                MeshVertex& v = waveGrid.vertices[i];
                const float u = static_cast<float>(x) / static_cast<float>(GRID_COLS);
                const float vv = static_cast<float>(y) / static_cast<float>(GRID_ROWS);
                const float wave = std::sin(u * TWO_PI * 2.0f + t * 1.6f) * 18.0f + std::cos(vv * TWO_PI * 1.5f + t * 1.1f) * 10.0f;
                v.position = {u * MESH_WIDTH - MESH_WIDTH * 0.5f, vv * MESH_HEIGHT - MESH_HEIGHT * 0.5f + wave};
                v.color = hsv(std::fmod(u + vv + t * 0.1f, 1.0f) * 360.0f, 0.7f, 1.0f, 255);
            }
        }

        pushMatrix();
        translate(240.0f, 270.0f);
        rotate(std::sin(t * 0.3f) * 0.15f);
        mesh(waveGrid.vertices, waveGrid.indices);
        popMatrix();

        // Static grid, textured + tinted - only the tint color and the
        // transform change per frame.
        const color_t cycleTint = hsv(std::fmod(t * 0.15f, 1.0f) * 360.0f, 0.5f, 1.0f, 255);
        tint(cycleTint);

        pushMatrix();
        translate(700.0f, 270.0f);
        rotate(-std::sin(t * 0.25f) * 0.15f);
        mesh(checkerGrid.vertices, checkerGrid.indices, checkerTexture);
        popMatrix();

        noTint();

        fill(255);
        noStroke();
        textAlign(TextAlign::topLeft);
        textSize(16.0f);
        text("mesh(vertices, indices) - animated, per-vertex color", 20.0f, 20.0f);
        text("mesh(vertices, indices, texture) - textured + tint()", 480.0f, 20.0f);
    }

private:
    GridMesh waveGrid;
    GridMesh checkerGrid;
    Texture checkerTexture;
};

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<MeshDemoSketch>();
    }
} // namespace p5cpp
