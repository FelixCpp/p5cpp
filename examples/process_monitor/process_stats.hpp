#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace procmon
{
    struct ProcessInfo
    {
        int32_t pid;
        std::string name;
    };

    // One point-in-time raw sample, pulled from proc_pidinfo(PROC_PIDTASKINFO) and proc_pid_rusage(RUSAGE_INFO_V4).
    struct RawSample
    {
        uint64_t cpuTimeNanos;
        uint64_t residentBytes;
        int32_t threadCount;
        int64_t pageFaults;       // cumulative, from pti_faults
        int64_t contextSwitches;  // cumulative, from pti_csw
        uint64_t diskBytesRead;   // cumulative; only valid if diskIoAvailable
        uint64_t diskBytesWritten;
        bool diskIoAvailable = false; // proc_pid_rusage can fail even when proc_pidinfo succeeds
        double wallClockSeconds;
    };

    // Derived from a pair of RawSamples; all rate fields are only meaningful once two samples exist.
    struct Sample
    {
        bool ok = false;
        double cpuPercent = 0.0;
        uint64_t residentBytes = 0;
        int32_t threadCount = 0;
        double pageFaultsPerSec = 0.0;
        double contextSwitchesPerSec = 0.0;
        bool diskIoAvailable = false;
        double diskReadBytesPerSec = 0.0;
        double diskWriteBytesPerSec = 0.0;
    };

    // Enumerates currently visible pids and resolves each to a short display name.
    // Pids whose name can't be resolved (e.g. a race with process exit) are skipped.
    std::vector<ProcessInfo> listProcesses();

    // Reads one raw sample for pid. Returns std::nullopt if the pid no longer exists
    // or is owned by another user (proc_pidinfo requires matching privileges).
    std::optional<RawSample> readRawSample(int32_t pid);

    // Pure delta computation between two samples of the same process.
    Sample deriveSample(const RawSample& previous, const RawSample& current);
} // namespace procmon
