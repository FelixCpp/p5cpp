#ifndef P5CPP_HPP
#define P5CPP_HPP

#include <memory>
#include <string_view>
#include <typeindex>
#include <format>
#include <vector>
#include <span>
#include <unordered_map>
#include <any>

namespace p5
{
    template <typename T> struct value2
    {
        T x, y;
    };
} // namespace p5

namespace p5
{
    template <typename T> struct value3
    {
        T x, y, z;
    };
} // namespace p5

namespace p5
{
    template <typename T> struct value4
    {
        T x, y, z, w;
    };
} // namespace p5

namespace p5
{
    enum WindowEventType
    {
        closed,
    };

    struct WindowEvent
    {
        WindowEventType type;
    };
} // namespace p5

namespace p5
{
    class PluginPipeline
    {
    public:
        virtual ~PluginPipeline() = default;
        virtual void next() = 0;
    };

    struct Plugin
    {
        virtual ~Plugin() = default;
        virtual void setup() = 0;
        virtual void event(const WindowEvent& event) = 0;
        virtual void draw() = 0;
        virtual void destroy() = 0;
    };
} // namespace p5

namespace p5
{
    struct Service
    {
        virtual ~Service() = default;
        virtual std::type_index getTypeIndex() const = 0;
    };

    template <typename T> struct BaseService : public T
    {
        inline std::type_index getTypeIndex() const override
        {
            return typeid(T);
        }
    };

    struct ServiceLocator
    {
    public:
        template <std::derived_from<Service> T> void registerService(std::shared_ptr<Service> service)
        {
            const auto type = service->getTypeIndex();
            if (services.contains(type)) {
                throw std::runtime_error(std::format("Type {} has already been registered", type.name()));
            }

            services.emplace(std::make_pair(type, std::move(service)));
        }

    private:
        std::unordered_map<std::type_index, std::shared_ptr<Service>> services;
    };
} // namespace p5

namespace p5
{
    struct PluginResolution
    {
        virtual ~PluginResolution() = default;
        virtual void add(std::unique_ptr<Plugin> plugin) = 0;
    };

    struct Environment
    {
        void addPlugin(std::unique_ptr<Plugin> plugin);
    };
} // namespace p5

namespace p5
{
    void logInfo(std::string message);
    void logWarn(std::string message);
    void logError(std::string message);
} // namespace p5

#define info(...) p5::logInfo(std::format(__VA_ARGS__))
#define warn(...) p5::logWarn(std::format(__VA_ARGS__))
#define error(...) p5::logError(std::format(__VA_ARGS__))

namespace p5
{
    struct Sketch
    {
        virtual ~Sketch() = default;

        virtual void setup();
        virtual void event(const WindowEvent& event);
        virtual void draw();
        virtual void destroy();
    };

    extern std::unique_ptr<Sketch> createSketch();
} // namespace p5

namespace p5
{
    void setExitCode(int exitCode);
    void close(int exitCode);
    void close();
} // namespace p5

namespace p5
{
    void setWindowSize(int width, int height);
    void setWindowPosition(int x, int y);
    void setWindowTitle(std::string_view title);
} // namespace p5

#endif // P5CPP_HPP
