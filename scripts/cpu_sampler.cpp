// External x64 stack sampler for local diagnostics. No guest memory is written.
// Suspends one target thread at a time, always resumes it before symbol lookup.
// CPU deltas identify active threads, not the precise cost of a sampled stack.
// This is a statistical fallback when kernel ETW profiling is unavailable.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>

struct Sample
{
    DWORD thread;
    ULONGLONG cpu_delta;
    std::vector<DWORD64> frames;
};

static ULONGLONG file_time(FILETIME value)
{
    return ((ULONGLONG)value.dwHighDateTime << 32) | value.dwLowDateTime;
}

static std::string symbol(HANDLE process, DWORD64 address)
{
    alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    SYMBOL_INFO *info = (SYMBOL_INFO *)storage;
    info->SizeOfStruct = sizeof(SYMBOL_INFO);
    info->MaxNameLen = MAX_SYM_NAME;
    DWORD64 displacement = 0;
    char fallback[512];
    if (SymFromAddr(process, address, &displacement, info))
    {
        std::snprintf(fallback, sizeof(fallback), "%s+0x%llx", info->Name,
                      (unsigned long long)displacement);
        return fallback;
    }
    IMAGEHLP_MODULE64 module = {};
    module.SizeOfStruct = sizeof(module);
    if (SymGetModuleInfo64(process, address, &module))
    {
        std::snprintf(fallback, sizeof(fallback), "%s+0x%llx", module.ModuleName,
                      (unsigned long long)(address - module.BaseOfImage));
    }
    else
    {
        std::snprintf(fallback, sizeof(fallback), "0x%llx", (unsigned long long)address);
    }
    return fallback;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::fprintf(stderr, "Usage: cpu-sampler PID SECONDS OUTPUT.csv\n");
        return 1;
    }
    DWORD target = (DWORD)std::strtoul(argv[1], nullptr, 10);
    unsigned seconds = (unsigned)std::strtoul(argv[2], nullptr, 10);
    if (!target || target == GetCurrentProcessId() || seconds < 1 || seconds > 60)
    {
        return 1;
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, target);
    if (!process)
    {
        std::fprintf(stderr, "OpenProcess failed: %lu\n", GetLastError());
        return 1;
    }
    FILE *output = std::fopen(argv[3], "wx");
    if (!output)
    {
        CloseHandle(process);
        return 1;
    }
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        std::fclose(output);
        CloseHandle(process);
        return 1;
    }
    std::map<DWORD, ULONGLONG> previous_cpu;
    std::vector<Sample> samples;
    unsigned failures = 0;
    ULONGLONG end = GetTickCount64() + seconds * 1000;
    while (GetTickCount64() < end && WaitForSingleObject(process, 0) == WAIT_TIMEOUT)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        if (snapshot != INVALID_HANDLE_VALUE && Thread32First(snapshot, &entry))
        {
            do
            {
                if (entry.th32OwnerProcessID != target)
                {
                    continue;
                }
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                                           FALSE, entry.th32ThreadID);
                if (!thread)
                {
                    failures++;
                    continue;
                }
                FILETIME created, exited, kernel, user;
                if (GetThreadTimes(thread, &created, &exited, &kernel, &user))
                {
                    ULONGLONG cpu = file_time(kernel) + file_time(user);
                    auto found = previous_cpu.find(entry.th32ThreadID);
                    ULONGLONG delta = found == previous_cpu.end() ? 0 : cpu - found->second;
                    previous_cpu[entry.th32ThreadID] = cpu;
                    if (delta && SuspendThread(thread) != (DWORD)-1)
                    {
                        // Allocate before suspension; collect addresses only.
                        // No target function is executed and no locks acquired
                        // from the target. DbgHelp is used on this thread only.
                        CONTEXT context = {};
                        context.ContextFlags = CONTEXT_FULL;
                        DWORD64 addresses[32] = {};
                        unsigned count = 0;
                        if (GetThreadContext(thread, &context))
                        {
                            addresses[count++] = context.Rip;
                            STACKFRAME64 frame = {};
                            frame.AddrPC.Offset = context.Rip;
                            frame.AddrStack.Offset = context.Rsp;
                            frame.AddrFrame.Offset = context.Rbp;
                            frame.AddrPC.Mode = frame.AddrStack.Mode = frame.AddrFrame.Mode = AddrModeFlat;
                            bool skipped_initial = false;
                            while (count < 32 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread,
                                &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                            {
                                if (frame.AddrPC.Offset == addresses[count - 1] && count == 1 && !skipped_initial)
                                {
                                    skipped_initial = true;
                                    continue;
                                }
                                if (!frame.AddrPC.Offset || frame.AddrPC.Offset == addresses[count - 1])
                                {
                                    break;
                                }
                                addresses[count++] = frame.AddrPC.Offset;
                            }
                        }
                        else
                        {
                            failures++;
                        }
                        if (ResumeThread(thread) == (DWORD)-1)
                        {
                            // A dying thread may no longer accept operations.
                            failures++;
                        }
                        if (count)
                        {
                            samples.push_back({entry.th32ThreadID, delta,
                                               std::vector<DWORD64>(addresses, addresses + count)});
                        }
                    }
                }
                CloseHandle(thread);
            }
            while (Thread32Next(snapshot, &entry));
        }
        if (snapshot != INVALID_HANDLE_VALUE)
        {
            CloseHandle(snapshot);
        }
        Sleep(50);
    }
    std::map<DWORD64, std::string> names;
    std::fprintf(output, "thread,cpu_delta_ms,leaf_address,leaf_file,leaf_line,stack\n");
    for (const Sample &sample : samples)
    {
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD displacement = 0;
        bool has_line = SymGetLineFromAddr64(process, sample.frames[0], &displacement, &line) != FALSE;
        std::fprintf(output, "%lu,%.4f,0x%llx,\"%s\",%lu,\"", sample.thread, sample.cpu_delta / 10000.0,
                      (unsigned long long)sample.frames[0], has_line ? line.FileName : "", has_line ? line.LineNumber : 0);
        for (size_t i = 0; i < sample.frames.size(); i++)
        {
            DWORD64 address = sample.frames[i];
            if (!names.count(address))
            {
                names[address] = symbol(process, address);
            }
            std::fprintf(output, "%s%s", i ? " <- " : "", names[address].c_str());
        }
        std::fprintf(output, "\"\n");
    }
    std::fclose(output);
    std::fprintf(stderr, "Saved %zu active-thread context samples; failures=%u. CPU deltas cover intervals, not stack costs.\n",
                 samples.size(), failures);
    SymCleanup(process);
    CloseHandle(process);
    return samples.empty() ? 1 : 0;
}
