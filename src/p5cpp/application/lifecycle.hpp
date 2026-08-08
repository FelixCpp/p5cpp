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

        bool shouldClose() const;
        bool shouldRestart() const;
        int getExitCode() const;

    private:
        bool m_shouldClose;
        bool m_shouldRestart;
        int m_exitCode;
    };
} // namespace p5
