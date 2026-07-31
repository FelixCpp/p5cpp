#ifndef P5_APPLICATION_HPP
#define P5_APPLICATION_HPP

#include <p5cpp/p5cpp.hpp>

namespace p5
{
    class Application
    {
    public:
        void add(std::unique_ptr<Plugin> plugin);

        void run();
        void dispatch(const WindowEvent& event);

    private:
        std::vector<std::unique_ptr<Plugin>> plugins;
    };
} // namespace p5

#endif // !P5_APPLICATION_HPP
