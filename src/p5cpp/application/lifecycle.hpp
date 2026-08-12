#pragma once

namespace p5
{
    class Lifecycle
    {
    public:
        Lifecycle();

        void exitCode(int exitCode);
        void close(int exitCode);
        void close();
        void restart();
        void nextFrame();

        bool shouldClose() const;
        bool shouldRestart() const;
        int getExitCode() const;
        int frameCount() const;

    private:
        bool m_shouldClose;
        bool m_shouldRestart;
        int m_exitCode;
        int m_frameCount;
    };
} // namespace p5
