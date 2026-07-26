#include <p5cpp/p5cpp.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    using namespace p5cpp;

    struct DroppedImage
    {
        Texture texture;
        std::string filename;
    };

    struct DragAndDropSketch : Sketch
    {
        Font font;
        std::vector<DroppedImage> images;

        void setup() override
        {
            setWindowSize(900, 600);
            setWindowTitle("Drag & Drop demo");
            font = loadFont("example_assets/MapleMono-NF-Regular.ttf");
            textFont(font);
        }

        // Event style: fires once per drop, at the moment it happens. Good for a
        // one-shot reaction (logging, a sound cue, ...) that doesn't need the paths
        // to outlive the callback.
        void event(const WindowEvent& windowEvent) override
        {
            if (windowEvent.type == EventType::fileDrop) {
                info("Dropped " + std::to_string(windowEvent.fileDrop.count) + " file(s) onto the window");
            }
        }

        // Poll style: getDroppedFiles() hands back the same drop, but as owned
        // std::filesystem::path values that are still valid here in draw() — the
        // natural place to actually act on them (e.g. load images, as below).
        void loadDroppedImages()
        {
            for (const std::filesystem::path& path : getDroppedFiles()) {
                std::unique_ptr<TextureImpl> loaded = loadImage(path);
                if (loaded == nullptr) {
                    continue; // not an image loadImage() understands; it already logged why
                }
                images.push_back(DroppedImage {Texture(std::move(loaded)), path.filename().string()});
            }
        }

        void draw() override
        {
            loadDroppedImages();

            background(30);

            fill(255);
            textSize(20.0f);
            textAlign(TextAlign::topLeft);
            text("Drag & drop image files onto this window", 20, 20);
            text(std::to_string(images.size()) + " image(s) loaded", 20, 50);

            constexpr float thumbWidth = 160.0f;
            constexpr float thumbHeight = 120.0f;
            constexpr float padding = 20.0f;
            constexpr float gridTop = 90.0f;

            const int columns = std::max(1, static_cast<int>((static_cast<float>(getLogicalWidth()) - padding) / (thumbWidth + padding)));

            for (size_t i = 0; i < images.size(); ++i) {
                const int column = static_cast<int>(i) % columns;
                const int row = static_cast<int>(i) / columns;
                const float x = padding + static_cast<float>(column) * (thumbWidth + padding);
                const float y = gridTop + static_cast<float>(row) * (thumbHeight + padding + 20.0f);

                image(images[i].texture, x, y, thumbWidth, thumbHeight);

                fill(255);
                textSize(14.0f);
                textAlign(TextAlign::topLeft);
                text(images[i].filename, x, y + thumbHeight + 4.0f, thumbWidth);
            }
        }
    };
} // namespace

namespace p5cpp
{
    std::unique_ptr<Sketch> createSketch()
    {
        return std::make_unique<DragAndDropSketch>();
    }
} // namespace p5cpp
