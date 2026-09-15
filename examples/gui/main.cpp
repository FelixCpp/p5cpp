#include <p5cpp/p5cpp.hpp>
#include <p5cpp_gui/p5cpp_gui.hpp>

#include <cmath>
#include <format>

using namespace p5;
using namespace p5::gui;

struct GuiExample : Sketch
{
    float circleRadius = 80.0f;
    float circleSpeed = 1.0f;
    float backgroundBrightness = 18.0f;
    int showCircle = 1;

    static constexpr const char* kColorNames[MU_COLOR_MAX] = {
        "text", "border", "windowbg", "titlebg", "titletext", "panelbg",
        "button", "buttonhover", "buttonfocus", "base", "basehover",
        "basefocus", "scrollbase", "scrollthumb",
    };

    struct ThemeValues
    {
        float colors[MU_COLOR_MAX][4];
        float padding;
        float spacing;
        float indent;
        float titleHeight;
        float scrollbarSize;
        float thumbSize;
        float sizeX;
        float sizeY;
        float cornerRadius;
    };

    ThemeValues themeValues {};
    ThemeValues defaultThemeValues {};

    void setup() override
    {
        setWindowSize(1300, 720);

        captureTheme(defaultThemeValues);
        themeValues = defaultThemeValues;
    }

    void captureTheme(ThemeValues& values)
    {
        const mu_Style* style = getGuiContext()->style;
        for (int i = 0; i < MU_COLOR_MAX; ++i) {
            values.colors[i][0] = static_cast<float>(style->colors[i].r);
            values.colors[i][1] = static_cast<float>(style->colors[i].g);
            values.colors[i][2] = static_cast<float>(style->colors[i].b);
            values.colors[i][3] = static_cast<float>(style->colors[i].a);
        }
        values.padding = static_cast<float>(style->padding);
        values.spacing = static_cast<float>(style->spacing);
        values.indent = static_cast<float>(style->indent);
        values.titleHeight = static_cast<float>(style->title_height);
        values.scrollbarSize = static_cast<float>(style->scrollbar_size);
        values.thumbSize = static_cast<float>(style->thumb_size);
        values.sizeX = static_cast<float>(style->size.x);
        values.sizeY = static_cast<float>(style->size.y);
        values.cornerRadius = getGuiCornerRadius();
    }

    void applyTheme()
    {
        mu_Style* style = getGuiContext()->style;
        for (int i = 0; i < MU_COLOR_MAX; ++i) {
            style->colors[i] = mu_color(
                static_cast<int>(themeValues.colors[i][0]),
                static_cast<int>(themeValues.colors[i][1]),
                static_cast<int>(themeValues.colors[i][2]),
                static_cast<int>(themeValues.colors[i][3])
            );
        }
        style->padding = static_cast<int>(themeValues.padding);
        style->spacing = static_cast<int>(themeValues.spacing);
        style->indent = static_cast<int>(themeValues.indent);
        style->title_height = static_cast<int>(themeValues.titleHeight);
        style->scrollbar_size = static_cast<int>(themeValues.scrollbarSize);
        style->thumb_size = static_cast<int>(themeValues.thumbSize);
        style->size = mu_vec2(static_cast<int>(themeValues.sizeX), static_cast<int>(themeValues.sizeY));
        setGuiCornerRadius(themeValues.cornerRadius);
    }

    void draw() override
    {
        background(rgba(static_cast<int32_t>(backgroundBrightness)));

        if (showCircle) {
            const float t = static_cast<float>(getGlobalTime()) * circleSpeed;
            noStroke();
            fill(rgba(90, 140, 220));
            circle(450.0f + std::cos(t) * 200.0f, 300.0f + std::sin(t) * 120.0f, circleRadius);
        }

        mu_Context* ctx = getGuiContext();
        beginGui();
        drawSettingsWindow(ctx);
        drawThemeEditor(ctx);
        endGui();
    }

    void drawThemeEditor(mu_Context* ctx)
    {
        if (mu_begin_window(ctx, "Theme Editor", mu_rect(640, 20, 620, 680))) {
            const int resetWidth[] = {-1};
            mu_layout_row(ctx, 1, resetWidth, 0);
            if (mu_button(ctx, "Reset to default")) {
                themeValues = defaultThemeValues;
            }

            const int colorWidths[] = {110, 110, 110, 110, 110};
            mu_layout_row(ctx, 5, colorWidths, 0);
            mu_label(ctx, "");
            mu_label(ctx, "R");
            mu_label(ctx, "G");
            mu_label(ctx, "B");
            mu_label(ctx, "A");

            for (int i = 0; i < MU_COLOR_MAX; ++i) {
                mu_layout_row(ctx, 5, colorWidths, 0);
                mu_label(ctx, kColorNames[i]);
                mu_slider_ex(ctx, &themeValues.colors[i][0], 0.0f, 255.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
                mu_slider_ex(ctx, &themeValues.colors[i][1], 0.0f, 255.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
                mu_slider_ex(ctx, &themeValues.colors[i][2], 0.0f, 255.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
                mu_slider_ex(ctx, &themeValues.colors[i][3], 0.0f, 255.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            }

            const int metricWidths[] = {160, -1};
            mu_layout_row(ctx, 2, metricWidths, 0);
            mu_label(ctx, "padding");
            mu_slider_ex(ctx, &themeValues.padding, 0.0f, 30.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "spacing");
            mu_slider_ex(ctx, &themeValues.spacing, 0.0f, 30.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "indent");
            mu_slider_ex(ctx, &themeValues.indent, 0.0f, 60.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "title height");
            mu_slider_ex(ctx, &themeValues.titleHeight, 10.0f, 60.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "scrollbar size");
            mu_slider_ex(ctx, &themeValues.scrollbarSize, 0.0f, 30.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "thumb size");
            mu_slider_ex(ctx, &themeValues.thumbSize, 0.0f, 30.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "row width");
            mu_slider_ex(ctx, &themeValues.sizeX, 10.0f, 200.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "row height");
            mu_slider_ex(ctx, &themeValues.sizeY, 10.0f, 60.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);
            mu_label(ctx, "corner radius");
            mu_slider_ex(ctx, &themeValues.cornerRadius, 0.0f, 20.0f, 1.0f, "%.0f", MU_OPT_ALIGNCENTER);

            mu_end_window(ctx);
        }
        applyTheme();
    }

    void drawSettingsWindow(mu_Context* ctx)
    {
        if (mu_begin_window(
                ctx,
                "Settings",
                mu_rect(100, 100, 500, 300)
            )) {

            // Label | Control | Control
            mu_layout_row(
                ctx,
                3,
                (int[]) {120, -100, -1},
                30
            );

            static float volume = 0.0f;
            mu_label(ctx, "Volume:");
            mu_slider(ctx, &volume, 0, 100);
            mu_label(ctx, "100%");

            static char name[64] = "User";
            mu_label(ctx, "Name:");
            mu_textbox(ctx, name, sizeof(name));
            mu_button(ctx, "Apply");

            mu_layout_row(
                ctx,
                2,
                (int[]) {-1, 100},
                30
            );

            mu_label(ctx, "");

            static bool enableFeature = false;
            if (mu_button(ctx, "Save")) {
                // save_settings();
            }

            mu_end_window(ctx);
        }
    }
};

SketchSpec p5::createSpec()
{
    return {
        .plugins = [] {
            std::vector<std::unique_ptr<Plugin>> plugins;
            plugins.push_back(gui::createGuiPlugin());
            return plugins;
        },
        .sketch = [] {
            return std::make_unique<GuiExample>();
        },
    };
}
