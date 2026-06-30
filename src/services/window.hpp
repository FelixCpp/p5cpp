namespace p5cpp
{
    struct Window
    {
        virtual ~Window() = default;

        virtual void swapBuffers() = 0;
        virtual void pollEvents() = 0;
    };
} // namespace p5cpp
