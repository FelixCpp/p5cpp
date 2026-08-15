#include "process_stats.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <format>
#include <optional>
#include <p5cpp/p5cpp.hpp>
#include <string>

using namespace p5;

namespace
{
    // Dark dashboard palette (validated categorical + chart-chrome roles).
    constexpr color_t kPagePlane = rgba(13, 13, 13);
    constexpr color_t kSurface = rgba(26, 26, 25);
    constexpr color_t kBorder = rgba(255, 255, 255, 26);
    constexpr color_t kPrimaryInk = rgba(255, 255, 255);
    constexpr color_t kSecondaryInk = rgba(195, 194, 183);
    constexpr color_t kMutedInk = rgba(137, 135, 129);
    constexpr color_t kGridline = rgba(44, 44, 42);
    constexpr color_t kGood = rgba(12, 163, 12);
    constexpr color_t kCritical = rgba(208, 59, 59);

    constexpr color_t kAccent = rgba(57, 135, 229);      // blue   — also CPU series / UI accent
    constexpr color_t kSeriesMemory = rgba(25, 158, 112);  // aqua
    constexpr color_t kSeriesThreads = rgba(201, 133, 0);   // yellow
    constexpr color_t kSeriesFaults = rgba(144, 133, 233); // violet
    constexpr color_t kSeriesCsw = rgba(230, 103, 103); // red
    constexpr color_t kSeriesDiskRead = rgba(20, 150, 20);   // green
    constexpr color_t kSeriesDiskWrite = rgba(217, 89, 38);   // orange

    constexpr float kGraphInset = 4.0f; // keeps a flat/constant trace visibly separate from the plot edge
    constexpr float kCardPad = 14.0f;
    constexpr float kCardRadius = 10.0f;

    color_t withAlpha(color_t color, int32_t alpha)
    {
        return rgba(getRed(color), getGreen(color), getBlue(color), alpha);
    }

    void pushSample(std::deque<float>& history, float value, float& runningMax, size_t capacity)
    {
        history.push_back(value);
        if (history.size() > capacity) history.pop_front();
        runningMax = std::max(runningMax, value);
    }

    std::string formatBytesPerSec(double bytesPerSec)
    {
        static constexpr std::array<const char*, 4> units = {"B/s", "KB/s", "MB/s", "GB/s"};
        size_t unitIndex = 0;
        double value = bytesPerSec;
        while (value >= 1024.0 and unitIndex + 1 < units.size()) {
            value /= 1024.0;
            ++unitIndex;
        }
        return std::format("{:.1f} {}", value, units[unitIndex]);
    }

    struct Cell
    {
        float left, top, width, height;
        float right() const { return left + width; }
        float bottom() const { return top + height; }
    };

    void drawCardShell(const Cell& cell)
    {
        noStroke();
        fill(kSurface);
        stroke(kBorder);
        strokeWeight(1.0f);
        rect(cell.left, cell.top, cell.width, cell.height, BorderRadius::all(kCardRadius));
    }

    // Kicker label (small, muted, letter-spaced) + a big primary-ink value, with a colored
    // identity dot in front of the title so each card reads as its own metric at a glance.
    void drawCardHeader(std::string_view title, std::string_view valueLabel, color_t accent, const Cell& cell)
    {
        noStroke();
        fill(accent);
        circle(cell.left + kCardPad + 3.0f, cell.top + kCardPad + 4.0f, 3.5f);

        fill(kMutedInk);
        textSize(11);
        textLetterSpacing(1.0f);
        textAlign(TextAlignment::topLeft);
        text(title, cell.left + kCardPad + 12.0f, cell.top + kCardPad);
        textLetterSpacing(0.0f);

        fill(kPrimaryInk);
        textSize(19);
        textAlign(TextAlignment::topRight);
        text(valueLabel, cell.right() - kCardPad, cell.top + kCardPad - 3.0f, cell.width - 2.0f * kCardPad);
    }

    void drawGridlines(float left, float right, float top, float bottom)
    {
        stroke(kGridline);
        strokeWeight(1.0f);
        for (float t : {0.25f, 0.5f, 0.75f}) {
            const float y = map(t, 0.0f, 1.0f, bottom, top);
            line(left, y, right, y);
        }
    }

    void plotSeries(const std::deque<float>& history, float runningMax, color_t seriesColor,
                     float left, float right, float top, float bottom, bool fillArea)
    {
        if (history.size() < 2) return;

        auto sampleY = [&](float value) { return map(value, 0.0f, runningMax, bottom - kGraphInset, top + kGraphInset); };
        auto sampleX = [&](size_t i) { return map(static_cast<float>(i), 0.0f, static_cast<float>(history.size() - 1), left, right); };

        if (fillArea) {
            noStroke();
            fill(withAlpha(seriesColor, 36));
            beginShape(ShapeMode::polygon);
            for (size_t i = 0; i < history.size(); ++i) vertex(sampleX(i), sampleY(history[i]));
            vertex(right, bottom);
            vertex(left, bottom);
            endShape(true);
        }

        noFill();
        stroke(seriesColor);
        strokeWeight(2.0f);
        beginShape(ShapeMode::path);
        for (size_t i = 0; i < history.size(); ++i) vertex(sampleX(i), sampleY(history[i]));
        endShape(false);
    }

    void drawGraphCard(std::string_view title, const std::deque<float>& history, float runningMax, color_t accent,
                        std::string_view valueLabel, const Cell& cell)
    {
        drawCardShell(cell);
        drawCardHeader(title, valueLabel, accent, cell);

        const float plotLeft = cell.left + kCardPad;
        const float plotRight = cell.right() - kCardPad;
        const float plotTop = cell.top + kCardPad + 32.0f;
        const float plotBottom = cell.bottom() - kCardPad;

        drawGridlines(plotLeft, plotRight, plotTop, plotBottom);
        plotSeries(history, runningMax, accent, plotLeft, plotRight, plotTop, plotBottom, true);
    }

    // Disk I/O needs two series (read/write) sharing one scale, with a small legend to tell them apart.
    void drawDualGraphCard(std::string_view title, const std::deque<float>& historyA, std::string_view labelA, color_t colorA,
                            const std::deque<float>& historyB, std::string_view labelB, color_t colorB,
                            float runningMax, std::string_view valueLabel, const Cell& cell)
    {
        drawCardShell(cell);
        drawCardHeader(title, valueLabel, colorA, cell);

        const float plotLeft = cell.left + kCardPad;
        const float plotRight = cell.right() - kCardPad;
        const float plotTop = cell.top + kCardPad + 32.0f;
        const float plotBottom = cell.bottom() - kCardPad - 18.0f; // room for legend row

        drawGridlines(plotLeft, plotRight, plotTop, plotBottom);
        plotSeries(historyA, runningMax, colorA, plotLeft, plotRight, plotTop, plotBottom, false);
        plotSeries(historyB, runningMax, colorB, plotLeft, plotRight, plotTop, plotBottom, false);

        textSize(11);
        textAlign(TextAlignment::topLeft);
        noStroke();
        fill(colorA);
        circle(plotLeft + 3.0f, plotBottom + 12.0f, 3.0f);
        fill(kSecondaryInk);
        text(labelA, plotLeft + 10.0f, plotBottom + 7.0f);
        fill(colorB);
        circle(plotLeft + 73.0f, plotBottom + 12.0f, 3.0f);
        fill(kSecondaryInk);
        text(labelB, plotLeft + 80.0f, plotBottom + 7.0f);
    }

    void drawUnavailableCard(std::string_view title, const Cell& cell)
    {
        drawCardShell(cell);
        drawCardHeader(title, "N/A", kMutedInk, cell);

        noStroke();
        fill(kMutedInk);
        textSize(12);
        textAlign(TextAlignment::center);
        text("not available for this process", cell.left + cell.width / 2.0f, cell.top + cell.height / 2.0f + 8.0f);
    }

    // Small bordered pill with centered text — used for the "back to list" affordance.
    void drawPillLabel(std::string_view label, float right, float top, float height)
    {
        textSize(12);
        const float width = textWidth(label) + 28.0f;
        const float left = right - width;

        noFill();
        stroke(kBorder);
        strokeWeight(1.0f);
        rect(left, top, width, height, BorderRadius::all(height / 2.0f));

        noStroke();
        fill(kSecondaryInk);
        textAlign(TextAlignment::center);
        text(label, left + width / 2.0f, top + height / 2.0f + 1.0f);
    }
} // namespace

enum class View
{
    Browsing,
    Monitoring,
};

struct ProcessMonitorSketch : public Sketch
{
    void setup() override
    {
        setWindowSize(1080, 800);
        setWindowTitle("Process Monitor");
        m_processList = procmon::listProcesses();
    }

    void event(const WindowEvent& event) override
    {
        if (m_view != View::Browsing) return;
        if (not event.is<WindowEvent::MouseScroll>()) return;

        const auto& scroll = event.as<WindowEvent::MouseScroll>();
        const uint2 winSize = getWindowSize();
        const float visibleHeight = static_cast<float>(winSize.y) - kListTop - kMargin;
        const float contentHeight = static_cast<float>(m_processList.size()) * kRowHeight;
        const float maxScroll = std::max(0.0f, contentHeight - visibleHeight);

        m_scrollOffset = std::clamp(m_scrollOffset - static_cast<float>(scroll.yOffset) * kRowHeight, 0.0f, maxScroll);
    }

    void draw() override
    {
        sampleIfDue();
        if (m_view == View::Browsing) drawBrowsingView();
        else drawMonitoringView();
    }

private:
    static constexpr float kMargin = 24.0f;
    static constexpr float kListTop = 96.0f;
    static constexpr float kRowHeight = 30.0f;
    static constexpr float kPidGutter = 64.0f;
    static constexpr size_t kHistoryCapacity = 120; // ~2 minutes at 1 Hz

    void drawBrowsingView()
    {
        background(kPagePlane);

        m_listRefreshAccumulator += getDeltaTime();
        if (m_listRefreshAccumulator >= 2.0 or m_processList.empty()) {
            m_processList = procmon::listProcesses();
            m_listRefreshAccumulator = 0.0;
        }

        const uint2 winSize = getWindowSize();

        noStroke();
        fill(kPrimaryInk);
        textSize(24);
        textAlign(TextAlignment::topLeft);
        text("Process Monitor", kMargin, kMargin);

        fill(kMutedInk);
        textSize(13);
        text("Select a process to start monitoring", kMargin, kMargin + 32.0f);

        textAlign(TextAlignment::topRight);
        text(std::format("{} processes", m_processList.size()), static_cast<float>(winSize.x) - kMargin, kMargin + 4.0f);

        const Cell panel {kMargin, kListTop, static_cast<float>(winSize.x) - 2.0f * kMargin, static_cast<float>(winSize.y) - kListTop - kMargin};
        drawCardShell(panel);

        const double mouseX = getMouseX();
        const double mouseY = getMouseY();
        const bool clicked = isMouseButtonPressed(MouseButton::Left);

        // Clip the scrolling row content to the panel body so rows don't bleed past its rounded corners.
        clip(panel.left, panel.top, panel.width, panel.height);
        for (size_t i = 0; i < m_processList.size(); ++i) {
            const float rowTop = panel.top + 6.0f + static_cast<float>(i) * kRowHeight - m_scrollOffset;
            if (rowTop + kRowHeight < panel.top or rowTop > panel.bottom()) continue;

            const bool hovered = mouseX >= panel.left and mouseX <= panel.right() and
                                  mouseY >= rowTop and mouseY <= rowTop + kRowHeight;

            if (hovered) {
                noStroke();
                fill(withAlpha(kAccent, 28));
                rect(panel.left + 3.0f, rowTop, panel.width - 3.0f, kRowHeight);
                fill(kAccent);
                rect(panel.left, rowTop, 3.0f, kRowHeight);
            }

            fill(kMutedInk);
            textSize(13);
            textAlign(TextAlignment::centerRight);
            text(std::format("{}", m_processList[i].pid), panel.left + kPidGutter, rowTop + kRowHeight / 2.0f);

            fill(hovered ? kPrimaryInk : kSecondaryInk);
            textAlign(TextAlignment::centerLeft);
            text(m_processList[i].name, panel.left + kPidGutter + 16.0f, rowTop + kRowHeight / 2.0f);

            noStroke();
            fill(kGridline);
            rect(panel.left + kCardPad, rowTop + kRowHeight - 1.0f, panel.width - 2.0f * kCardPad, 1.0f);

            if (hovered and clicked) startMonitoring(m_processList[i].pid);
        }
        noClip();

        drawScrollbar(panel);
    }

    void drawScrollbar(const Cell& panel)
    {
        const float contentHeight = static_cast<float>(m_processList.size()) * kRowHeight;
        const float visibleHeight = panel.height - 12.0f;
        if (contentHeight <= visibleHeight) return; // everything fits, no scrollbar needed

        const float trackTop = panel.top + 6.0f;
        const float trackHeight = panel.height - 12.0f;
        const float thumbHeight = std::max(24.0f, trackHeight * (visibleHeight / contentHeight));
        const float maxScroll = contentHeight - visibleHeight;
        const float thumbTop = trackTop + (trackHeight - thumbHeight) * (maxScroll > 0.0f ? m_scrollOffset / maxScroll : 0.0f);

        noStroke();
        fill(kBorder);
        rect(panel.right() - 8.0f, trackTop, 4.0f, trackHeight, BorderRadius::all(2.0f));
        fill(kMutedInk);
        rect(panel.right() - 8.0f, thumbTop, 4.0f, thumbHeight, BorderRadius::all(2.0f));
    }

    void drawMonitoringView()
    {
        background(kPagePlane);

        // Note: Escape is reserved by p5cpp's LifecyclePlugin as a global quit shortcut
        // (it closes the whole window), so it can't be reused as a "back" key here.
        if (isKeyPressed(Key::Backspace)) {
            stopMonitoring();
            return;
        }

        const uint2 winSize = getWindowSize();

        noStroke();
        fill(m_processGone ? kCritical : kGood);
        circle(kMargin + 4.0f, kMargin + 15.0f, 4.0f);

        fill(kPrimaryInk);
        textSize(22);
        textAlign(TextAlignment::topLeft);
        text(m_selectedName, kMargin + 16.0f, kMargin);

        fill(kMutedInk);
        textSize(13);
        text(std::format("pid {}{}", m_selectedPid, m_processGone ? "  ·  process exited" : "  ·  live"),
             kMargin + 16.0f, kMargin + 27.0f);

        drawPillLabel("Backspace ← back", static_cast<float>(winSize.x) - kMargin, kMargin + 2.0f, 26.0f);

        // 3x2 grid of metric cards.
        constexpr int kColumns = 3;
        constexpr int kRows = 2;
        constexpr float kGridTop = 90.0f;
        constexpr float kColumnGap = 20.0f;
        constexpr float kRowGap = 20.0f;

        const float gridWidth = static_cast<float>(winSize.x) - 2.0f * kMargin;
        const float gridHeight = static_cast<float>(winSize.y) - kGridTop - kMargin;
        const float cellWidth = (gridWidth - kColumnGap * (kColumns - 1)) / kColumns;
        const float cellHeight = (gridHeight - kRowGap * (kRows - 1)) / kRows;

        auto cellAt = [&](int col, int row) {
            return Cell {
                kMargin + static_cast<float>(col) * (cellWidth + kColumnGap),
                kGridTop + static_cast<float>(row) * (cellHeight + kRowGap),
                cellWidth, cellHeight,
            };
        };

        drawGraphCard("CPU", m_cpuHistory, m_cpuRunningMax, kAccent,
                      std::format("{:.1f}%", m_cpuHistory.empty() ? 0.0f : m_cpuHistory.back()), cellAt(0, 0));

        drawGraphCard("MEMORY (RSS)", m_memHistory, m_memRunningMax, kSeriesMemory,
                      std::format("{:.1f} MB", m_memHistory.empty() ? 0.0f : m_memHistory.back()), cellAt(1, 0));

        drawGraphCard("THREADS", m_threadHistory, m_threadRunningMax, kSeriesThreads,
                      std::format("{}", m_threadHistory.empty() ? 0 : static_cast<int>(m_threadHistory.back())), cellAt(2, 0));

        drawGraphCard("PAGE FAULTS", m_faultHistory, m_faultRunningMax, kSeriesFaults,
                      std::format("{:.1f}/s", m_faultHistory.empty() ? 0.0f : m_faultHistory.back()), cellAt(0, 1));

        drawGraphCard("CONTEXT SWITCHES", m_cswHistory, m_cswRunningMax, kSeriesCsw,
                      std::format("{:.1f}/s", m_cswHistory.empty() ? 0.0f : m_cswHistory.back()), cellAt(1, 1));

        if (m_diskIoAvailable) {
            const double diskCurrentTotal = (m_diskReadHistory.empty() ? 0.0 : m_diskReadHistory.back()) +
                                             (m_diskWriteHistory.empty() ? 0.0 : m_diskWriteHistory.back());
            drawDualGraphCard("DISK I/O", m_diskReadHistory, "read", kSeriesDiskRead, m_diskWriteHistory, "write", kSeriesDiskWrite,
                              m_diskRunningMax, formatBytesPerSec(diskCurrentTotal), cellAt(2, 1));
        } else {
            drawUnavailableCard("DISK I/O", cellAt(2, 1));
        }
    }

    void sampleIfDue()
    {
        if (m_view != View::Monitoring or m_processGone) return;

        m_sampleAccumulator += getDeltaTime();
        if (m_sampleAccumulator < 1.0) return;
        m_sampleAccumulator = 0.0;

        std::optional<procmon::RawSample> raw = procmon::readRawSample(m_selectedPid);
        if (not raw) {
            m_processGone = true;
            return;
        }
        raw->wallClockSeconds = getGlobalTime();

        if (m_previousRawSample) {
            const procmon::Sample sample = procmon::deriveSample(*m_previousRawSample, *raw);
            pushSample(m_cpuHistory, static_cast<float>(sample.cpuPercent), m_cpuRunningMax, kHistoryCapacity);
            pushSample(m_memHistory, static_cast<float>(sample.residentBytes) / (1024.0f * 1024.0f), m_memRunningMax, kHistoryCapacity);
            pushSample(m_threadHistory, static_cast<float>(sample.threadCount), m_threadRunningMax, kHistoryCapacity);
            pushSample(m_faultHistory, static_cast<float>(sample.pageFaultsPerSec), m_faultRunningMax, kHistoryCapacity);
            pushSample(m_cswHistory, static_cast<float>(sample.contextSwitchesPerSec), m_cswRunningMax, kHistoryCapacity);

            m_diskIoAvailable = sample.diskIoAvailable;
            if (sample.diskIoAvailable) {
                pushSample(m_diskReadHistory, static_cast<float>(sample.diskReadBytesPerSec), m_diskRunningMax, kHistoryCapacity);
                pushSample(m_diskWriteHistory, static_cast<float>(sample.diskWriteBytesPerSec), m_diskRunningMax, kHistoryCapacity);
            }
        }

        m_previousRawSample = raw;
    }

    void startMonitoring(int32_t pid)
    {
        m_selectedPid = pid;
        m_selectedName = "?";
        for (const procmon::ProcessInfo& info : m_processList) {
            if (info.pid == pid) {
                m_selectedName = info.name;
                break;
            }
        }

        m_previousRawSample.reset();
        m_sampleAccumulator = 0.0;
        m_processGone = false;
        m_diskIoAvailable = true; // optimistic default; sampleIfDue corrects this after the first real sample

        m_cpuHistory.clear();
        m_memHistory.clear();
        m_threadHistory.clear();
        m_faultHistory.clear();
        m_cswHistory.clear();
        m_diskReadHistory.clear();
        m_diskWriteHistory.clear();

        m_cpuRunningMax = 10.0f;
        m_memRunningMax = 10.0f;
        m_threadRunningMax = 4.0f;
        m_faultRunningMax = 10.0f;
        m_cswRunningMax = 10.0f;
        m_diskRunningMax = 1024.0f; // 1 KB/s floor

        m_view = View::Monitoring;
    }

    void stopMonitoring()
    {
        m_view = View::Browsing;
        m_selectedPid = -1;
    }

    View m_view = View::Browsing;

    std::vector<procmon::ProcessInfo> m_processList;
    double m_listRefreshAccumulator = 0.0;
    float m_scrollOffset = 0.0f;

    int32_t m_selectedPid = -1;
    std::string m_selectedName;
    std::optional<procmon::RawSample> m_previousRawSample;
    double m_sampleAccumulator = 0.0;
    bool m_processGone = false;

    std::deque<float> m_cpuHistory;
    std::deque<float> m_memHistory;
    std::deque<float> m_threadHistory;
    std::deque<float> m_faultHistory;
    std::deque<float> m_cswHistory;
    std::deque<float> m_diskReadHistory;
    std::deque<float> m_diskWriteHistory;
    bool m_diskIoAvailable = true;

    float m_cpuRunningMax = 10.0f;
    float m_memRunningMax = 10.0f;
    float m_threadRunningMax = 4.0f;
    float m_faultRunningMax = 10.0f;
    float m_cswRunningMax = 10.0f;
    float m_diskRunningMax = 1024.0f;
};

std::unique_ptr<Sketch> p5::createSketch()
{
    return std::make_unique<ProcessMonitorSketch>();
}
