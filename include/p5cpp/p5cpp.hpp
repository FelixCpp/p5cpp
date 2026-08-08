#pragma once

#include <cstdint>
#include <variant>
#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <span>

namespace p5
{
    class WindowEvent
    {
    public:
        struct EventTypeTag
        {
        };

        struct Close : EventTypeTag
        {
        };

        struct WindowResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FramebufferResize : EventTypeTag
        {
            uint32_t width;
            uint32_t height;
        };

        struct FocusGained : EventTypeTag
        {
        };

        struct FocusLost : EventTypeTag
        {
        };

        using EventType = std::variant<Close, WindowResize, FramebufferResize, FocusGained, FocusLost>;

        constexpr WindowEvent(const EventType& eventType);

        template <std::derived_from<EventTypeTag> T> constexpr bool is() const;

    private:
        EventType m_eventType;
    };
} // namespace p5

namespace p5
{
    struct Sketch
    {
        virtual ~Sketch() = default;
        virtual void setup() {}
        virtual void event(const WindowEvent& event) {}
        virtual void draw() {}
        virtual void destroy() {}
    };

    extern std::unique_ptr<Sketch> createSketch();
} // namespace p5

namespace p5
{
    class Context
    {
    public:
        template <typename T> void provide(T* instance);
        template <typename T> void remove();
        template <typename T> T& require();
        template <typename T> T* get() const;
        template <typename T> bool has() const;

    private:
        std::unordered_map<std::type_index, void*> m_instances;
    };

} // namespace p5

namespace p5
{
    struct Plugin;

    class Next
    {
    public:
        explicit Next(std::span<const std::unique_ptr<Plugin>> chain, size_t index, Context* context, void (*step)(Plugin&, Context&, const Next&), const void* payload);
        void operator()() const;

        const void* getPayload() const;

    private:
        using Step = void (*)(Plugin&, Context&, const Next&);

        std::span<const std::unique_ptr<Plugin>> m_chain;
        size_t m_index;
        Context* context;
        Step m_step;
        const void* payload;
    };

    struct Plugin
    {
        virtual ~Plugin() = default;
        virtual void setup(Context& context, const Next& next);
        virtual void event(Context& context, const Next& next, const WindowEvent& event);
        virtual void draw(Context& context, const Next& next);
        virtual void destroy(Context& context, const Next& next);
    };
} // namespace p5

namespace p5
{
    constexpr WindowEvent::WindowEvent(const EventType& eventType)
        : m_eventType(std::move(eventType))
    {
    }

    template <std::derived_from<WindowEvent::EventTypeTag> T>
    constexpr bool WindowEvent::is() const
    {
        return std::holds_alternative<T>(m_eventType);
    }
} // namespace p5

namespace p5
{
    template <typename T>
    void Context::provide(T* instance)
    {
        m_instances.try_emplace(typeid(T), instance);
    }

    template <typename T>
    void Context::remove()
    {
        m_instances.erase(typeid(T));
    }

    template <typename T>
    T& Context::require()
    {
        return *get<T>();
    }

    template <typename T>
    T* Context::get() const
    {
        return static_cast<T*>(m_instances.at(typeid(T)));
    }

    template <typename T>
    bool Context::has() const
    {
        return m_instances.contains(typeid(T));
    }

} // namespace p5
