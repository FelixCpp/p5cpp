#include "process_stats.hpp"

#include <algorithm>
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/resource.h>

namespace procmon
{
    std::vector<ProcessInfo> listProcesses()
    {
        // proc_listallpids returns the size (in bytes) of the pid buffer, not a raw count.
        const int requiredBytes = proc_listallpids(nullptr, 0);
        if (requiredBytes <= 0) return {};

        // Add margin: the process count can grow between the sizing call and the fill call.
        const size_t capacity = static_cast<size_t>(requiredBytes) / sizeof(pid_t) + 64;
        std::vector<pid_t> pids(capacity);

        const int filledBytes = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
        if (filledBytes <= 0) return {};

        const size_t count = std::min(static_cast<size_t>(filledBytes) / sizeof(pid_t), pids.size());

        std::vector<ProcessInfo> result;
        result.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            const pid_t pid = pids[i];
            if (pid <= 0) continue;

            char nameBuffer[256] = {};
            const int nameLength = proc_name(pid, nameBuffer, sizeof(nameBuffer));
            if (nameLength <= 0) continue; // inaccessible or vanished between listing and naming

            result.push_back(ProcessInfo{.pid = static_cast<int32_t>(pid), .name = std::string(nameBuffer)});
        }
        return result;
    }

    std::optional<RawSample> readRawSample(int32_t pid)
    {
        struct proc_taskinfo info = {};
        const int written = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));
        if (written != sizeof(info)) return std::nullopt;

        RawSample sample;
        sample.cpuTimeNanos = info.pti_total_user + info.pti_total_system;
        sample.residentBytes = info.pti_resident_size;
        sample.threadCount = info.pti_threadnum;
        sample.pageFaults = info.pti_faults;
        sample.contextSwitches = info.pti_csw;
        sample.wallClockSeconds = 0.0; // caller stamps this from its own clock immediately after the call

        // Independent of proc_pidinfo above: can fail on its own (e.g. some system processes
        // don't expose rusage), so degrade gracefully rather than failing the whole sample.
        struct rusage_info_v4 rusage = {};
        if (proc_pid_rusage(pid, RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t*>(&rusage)) == 0) {
            sample.diskBytesRead = rusage.ri_diskio_bytesread;
            sample.diskBytesWritten = rusage.ri_diskio_byteswritten;
            sample.diskIoAvailable = true;
        } else {
            sample.diskBytesRead = 0;
            sample.diskBytesWritten = 0;
            sample.diskIoAvailable = false;
        }

        return sample;
    }

    namespace
    {
        double ratePerSecond(int64_t previousValue, int64_t currentValue, double dtSeconds)
        {
            int64_t delta = currentValue - previousValue;
            if (delta < 0) delta = 0; // guards a counter reset/race rather than reporting a negative rate
            return static_cast<double>(delta) / dtSeconds;
        }
    } // namespace

    Sample deriveSample(const RawSample& previous, const RawSample& current)
    {
        Sample sample;
        sample.ok = true;
        sample.residentBytes = current.residentBytes;
        sample.threadCount = current.threadCount;

        const double dtSeconds = current.wallClockSeconds - previous.wallClockSeconds;
        if (dtSeconds <= 0.0) return sample; // all rates stay at their zero default

        int64_t cpuDeltaNanos = static_cast<int64_t>(current.cpuTimeNanos) - static_cast<int64_t>(previous.cpuTimeNanos);
        if (cpuDeltaNanos < 0) cpuDeltaNanos = 0;
        sample.cpuPercent = (static_cast<double>(cpuDeltaNanos) / (dtSeconds * 1e9)) * 100.0;

        sample.pageFaultsPerSec = ratePerSecond(previous.pageFaults, current.pageFaults, dtSeconds);
        sample.contextSwitchesPerSec = ratePerSecond(previous.contextSwitches, current.contextSwitches, dtSeconds);

        sample.diskIoAvailable = previous.diskIoAvailable and current.diskIoAvailable;
        if (sample.diskIoAvailable) {
            sample.diskReadBytesPerSec = ratePerSecond(static_cast<int64_t>(previous.diskBytesRead), static_cast<int64_t>(current.diskBytesRead), dtSeconds);
            sample.diskWriteBytesPerSec = ratePerSecond(static_cast<int64_t>(previous.diskBytesWritten), static_cast<int64_t>(current.diskBytesWritten), dtSeconds);
        }

        return sample;
    }
} // namespace procmon
