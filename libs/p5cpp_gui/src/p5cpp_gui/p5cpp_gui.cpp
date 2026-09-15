#include <p5cpp_gui/p5cpp_gui.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace p5::gui
{
    int toMuButton(MouseButton button)
    {
        switch (button) {
            case MouseButton::left: return MU_MOUSE_LEFT;
            case MouseButton::right: return MU_MOUSE_RIGHT;
            case MouseButton::middle: return MU_MOUSE_MIDDLE;
            default: return 0;
        }
    }

    int toMuKey(Key key)
    {
        switch (key) {
            case Key::LeftShift:
            case Key::RightShift: return MU_KEY_SHIFT;
            case Key::LeftControl:
            case Key::RightControl: return MU_KEY_CTRL;
            case Key::LeftAlt:
            case Key::RightAlt: return MU_KEY_ALT;
            case Key::Backspace: return MU_KEY_BACKSPACE;
            case Key::Enter:
            case Key::KeypadEnter: return MU_KEY_RETURN;
            default: return 0;
        }
    }

    color_t toColor(mu_Color color)
    {
        return rgba(color.r, color.g, color.b, color.a);
    }

    void appendUtf8(std::string& out, uint32_t codepoint)
    {
        if (codepoint < 0x80) {
            out += static_cast<char>(codepoint);
        } else if (codepoint < 0x800) {
            out += static_cast<char>(0xC0 | (codepoint >> 6));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            out += static_cast<char>(0xE0 | (codepoint >> 12));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (codepoint >> 18));
            out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    constexpr mu_Color kDefaultColors[MU_COLOR_MAX] = {
        {225, 225, 230, 255}, // MU_COLOR_TEXT
        {0, 0, 0, 0},         // MU_COLOR_BORDER
        {32, 32, 38, 255},    // MU_COLOR_WINDOWBG
        {24, 24, 29, 255},    // MU_COLOR_TITLEBG
        {235, 235, 240, 255}, // MU_COLOR_TITLETEXT
        {0, 0, 0, 0},         // MU_COLOR_PANELBG
        {58, 58, 68, 255},    // MU_COLOR_BUTTON
        {72, 72, 86, 255},    // MU_COLOR_BUTTONHOVER
        {86, 133, 230, 255},  // MU_COLOR_BUTTONFOCUS
        {24, 24, 29, 255},    // MU_COLOR_BASE
        {30, 30, 37, 255},    // MU_COLOR_BASEHOVER
        {36, 36, 44, 255},    // MU_COLOR_BASEFOCUS
        {20, 20, 24, 255},    // MU_COLOR_SCROLLBASE
        {95, 95, 112, 255},   // MU_COLOR_SCROLLTHUMB
    };

    void applyDefaultStyle(mu_Style& style)
    {
        for (int i = 0; i < MU_COLOR_MAX; ++i) {
            style.colors[i] = kDefaultColors[i];
        }
        style.padding = 6;
        style.spacing = 6;
        style.title_height = 26;
        style.scrollbar_size = 12;
        style.thumb_size = 8;
        style.size = mu_vec2(70, 20);
    }
} // namespace p5::gui

namespace p5::gui
{
    class GuiController;
    extern thread_local std::unique_ptr<GuiController> controller;
} // namespace p5::gui

namespace p5::gui
{
    class GuiController
    {
    public:
        GuiController()
        {
            mu_init(&m_ctx);
            m_ctx.text_width = &measureTextWidth;
            m_ctx.text_height = &measureTextHeight;
            applyDefaultStyle(*m_ctx.style);
        }

        mu_Context* context() { return &m_ctx; }

        void onEvent(const WindowEvent& event)
        {
            event.on(
                [this](const WindowEvent::MouseMove& e) {
                    mu_input_mousemove(&m_ctx, static_cast<int>(e.x), static_cast<int>(e.y));
                },
                [this](const WindowEvent::MouseButtonPress& e) {
                    if (const int button = toMuButton(e.button); button != 0) {
                        mu_input_mousedown(&m_ctx, static_cast<int>(e.x), static_cast<int>(e.y), button);
                    }
                },
                [this](const WindowEvent::MouseButtonRelease& e) {
                    if (const int button = toMuButton(e.button); button != 0) {
                        mu_input_mouseup(&m_ctx, static_cast<int>(e.x), static_cast<int>(e.y), button);
                    }
                },
                [this](const WindowEvent::MouseScroll& e) {
                    mu_input_scroll(&m_ctx, static_cast<int>(-e.xOffset * kScrollSensitivity), static_cast<int>(-e.yOffset * kScrollSensitivity));
                },
                [this](const WindowEvent::KeyPress& e) {
                    if (const int key = toMuKey(e.key); key != 0) {
                        mu_input_keydown(&m_ctx, key);
                    }
                },
                [this](const WindowEvent::KeyRelease& e) {
                    if (const int key = toMuKey(e.key); key != 0) {
                        mu_input_keyup(&m_ctx, key);
                    }
                },
                [this](const WindowEvent::CharInput& e) {
                    std::string utf8;
                    appendUtf8(utf8, e.codepoint);
                    mu_input_text(&m_ctx, utf8.c_str());
                }
            );
        }

        void begin()
        {
            mu_begin(&m_ctx);
        }

        void end()
        {
            mu_end(&m_ctx);
            render();
        }

        void setTextSize(float pixels) { m_textSize = pixels; }
        float getTextSize() const { return m_textSize; }

        void setCornerRadius(float pixels) { m_cornerRadius = pixels; }
        float getCornerRadius() const { return m_cornerRadius; }

    private:
        static int measureTextWidth(mu_Font, const char* str, int len)
        {
            if (controller == nullptr) {
                return 0;
            }
            return controller->textWidthInPixels(std::string_view(str, static_cast<size_t>(len)));
        }

        static int measureTextHeight(mu_Font)
        {
            if (controller == nullptr) {
                return 0;
            }
            return controller->textHeightInPixels();
        }

        int textWidthInPixels(std::string_view str) const
        {
            push();
            noTextFont();
            textSize(m_textSize);
            const float width = textWidth(str);
            pop();
            return static_cast<int>(std::ceil(width));
        }

        int textHeightInPixels() const
        {
            return static_cast<int>(std::ceil(m_textSize * kLineHeightFactor));
        }

        void render()
        {
            push();
            setMatrix(identityMatrix());
            noStroke();
            textAlign(TextAlignment::topLeft);
            textSize(m_textSize);
            noTextFont();

            mu_Command* cmd = nullptr;
            while (mu_next_command(&m_ctx, &cmd)) {
                switch (cmd->type) {
                    case MU_COMMAND_RECT:
                        noStroke();
                        fill(toColor(cmd->rect.color));
                        rect(static_cast<float>(cmd->rect.rect.x), static_cast<float>(cmd->rect.rect.y), static_cast<float>(cmd->rect.rect.w), static_cast<float>(cmd->rect.rect.h), BorderRadius::all(m_cornerRadius));
                        break;
                    case MU_COMMAND_TEXT:
                        fill(toColor(cmd->text.color));
                        text(std::string_view(cmd->text.str), static_cast<float>(cmd->text.pos.x), static_cast<float>(cmd->text.pos.y));
                        break;
                    case MU_COMMAND_ICON:
                        drawIcon(cmd->icon);
                        break;
                    case MU_COMMAND_CLIP:
                        clip(static_cast<float>(cmd->clip.rect.x), static_cast<float>(cmd->clip.rect.y), static_cast<float>(cmd->clip.rect.w), static_cast<float>(cmd->clip.rect.h));
                        break;
                    default: break;
                }
            }

            pop();
        }

        static void drawIcon(const mu_IconCommand& icon)
        {
            const float x = static_cast<float>(icon.rect.x);
            const float y = static_cast<float>(icon.rect.y);
            const float w = static_cast<float>(icon.rect.w);
            const float h = static_cast<float>(icon.rect.h);
            const float cx = x + w * 0.5f;
            const float cy = y + h * 0.5f;
            const float s = std::min(w, h) * 0.25f;

            switch (icon.id) {
                case MU_ICON_CLOSE:
                    noFill();
                    stroke(toColor(icon.color));
                    strokeWeight(1.0f);
                    line(cx - s, cy - s, cx + s, cy + s);
                    line(cx - s, cy + s, cx + s, cy - s);
                    break;
                case MU_ICON_CHECK:
                    noFill();
                    stroke(toColor(icon.color));
                    strokeWeight(1.0f);
                    line(cx - s, cy, cx - s * 0.2f, cy + s);
                    line(cx - s * 0.2f, cy + s, cx + s, cy - s);
                    break;
                case MU_ICON_COLLAPSED:
                    noStroke();
                    fill(toColor(icon.color));
                    triangle(cx - s, cy - s, cx - s, cy + s, cx + s, cy);
                    break;
                case MU_ICON_EXPANDED:
                    noStroke();
                    fill(toColor(icon.color));
                    triangle(cx - s, cy - s, cx + s, cy - s, cx, cy + s);
                    break;
                default: break;
            }
        }

        static constexpr float kLineHeightFactor = 1.2f;
        static constexpr double kScrollSensitivity = 30.0;

        mu_Context m_ctx {};
        float m_textSize = 14.0f;
        float m_cornerRadius = 4.0f;
    };
} // namespace p5::gui

namespace p5::gui
{
    thread_local std::unique_ptr<GuiController> controller;
} // namespace p5::gui

namespace p5::gui
{
    class GuiPlugin : public Plugin
    {
    public:
        void setup(const Next& next) override
        {
            controller = std::make_unique<GuiController>();
            provideDependency(controller.get());

            next();
        }

        void event(const Next& next, const WindowEvent& windowEvent) override
        {
            controller->onEvent(windowEvent);
            next();
        }

        void draw(const Next& next) override
        {
            next();
        }

        void destroy(const Next& next) override
        {
            next();

            removeDependency<GuiController>();
            controller.reset();
        }
    };
} // namespace p5::gui

namespace p5::gui
{
    mu_Context* getGuiContext()
    {
        if (controller == nullptr) {
            error("GUI is not initialized. Please add the GuiPlugin to your sketch.");
            return nullptr;
        }

        return controller->context();
    }

    void beginGui()
    {
        if (controller == nullptr) {
            error("GUI is not initialized. Please add the GuiPlugin to your sketch.");
            return;
        }

        controller->begin();
    }

    void endGui()
    {
        if (controller == nullptr) {
            error("GUI is not initialized. Please add the GuiPlugin to your sketch.");
            return;
        }

        controller->end();
    }

    void setGuiTextSize(float pixels)
    {
        if (controller != nullptr) {
            controller->setTextSize(pixels);
        }
    }

    float getGuiTextSize()
    {
        return controller != nullptr ? controller->getTextSize() : 14.0f;
    }

    void setGuiCornerRadius(float pixels)
    {
        if (controller != nullptr) {
            controller->setCornerRadius(pixels);
        }
    }

    float getGuiCornerRadius()
    {
        return controller != nullptr ? controller->getCornerRadius() : 4.0f;
    }
} // namespace p5::gui

namespace p5::gui
{
    std::unique_ptr<Plugin> createGuiPlugin()
    {
        return std::make_unique<GuiPlugin>();
    }
} // namespace p5::gui
