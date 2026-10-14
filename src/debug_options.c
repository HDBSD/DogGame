#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <io.h>
#include <share.h>
#include "debug_options.h"
/* Publish options only after every argument has passed validation. */
TitleOptions title_options;
static const char *usage = "DogGame.exe [--nofmv] [--noaudio] [--debug [options]]\n\n"
                           "--nofmv                Skip introductory movies\n"
                           "--noaudio              Mute audio output\n"
                           "--debug                Enable debug title and pause controls\n"
                           "--level <ID>           Start in a level (e.g. H001); requires --debug\n"
                           "--loadsaveslot <1-3>   Restore a save; requires --debug\n"
                           "                       With --level, restore save then warp\n"
                           "--trace                Trace the first rendered frame; requires --debug\n"
                           "--profiler             Capture a 10-second renderer profile; requires --debug\n"
                           "--fpsgraph             Show frame timing graph; requires --debug\n"
                           "--help                 Show this help\n\n"
                           "Pause, then select Debug with F1 or controller Back.\n";

uint32_t title_level_id(const char *name)
{
    return ((uint32_t)(unsigned char)name[0] << 24) | ((uint32_t)(unsigned char)name[1] << 16) |
           ((uint32_t)(unsigned char)name[2] << 8) | (unsigned char)name[3];
}

int title_options_parse(int argc, const wchar_t *const *argv, char *error, unsigned capacity)
{
    TitleOptions options = {0};
    int requires_debug = 0;
    for (int i = 1; i < argc; i++)
    {
        const wchar_t *argument = argv[i];
        if (!wcscmp(argument, L"--debug"))
        {
            options.debug = 1;
        }
        else if (!wcscmp(argument, L"--nofmv"))
        {
            options.nofmv = 1;
        }
        else if (!wcscmp(argument, L"--noaudio"))
        {
            options.noaudio = 1;
        }
        else if (!wcscmp(argument, L"--help"))
        {
            options.help = 1;
        }
        else if (!wcscmp(argument, L"--trace"))
        {
            options.trace = 1;
            requires_debug = 1;
        }
        else if (!wcscmp(argument, L"--profiler"))
        {
            options.profiler = 1;
            requires_debug = 1;
        }
        else if (!wcscmp(argument, L"--fpsgraph"))
        {
            options.fps_graph = 1;
            requires_debug = 1;
        }
        else if (!wcscmp(argument, L"--level"))
        {
            if (++i >= argc || wcslen(argv[i]) != 4)
            {
                snprintf(error, capacity, "--level needs a four-character scene ID, e.g. H001.");
                return 0;
            }
            for (int j = 0; j < 4; j++)
            {
                wchar_t c = argv[i][j];
                if (c >= L'a' && c <= L'z')
                {
                    c -= L'a' - L'A';
                }
                if ((j == 0 && (c < L'A' || c > L'Z')) || (j > 0 && (c < L'0' || c > L'9')))
                {
                    snprintf(error, capacity, "Invalid level ID. Use IDs such as H001 or W020.");
                    return 0;
                }
                options.level[j] = (char)c;
            }
            requires_debug = 1;
        }
        else if (!wcscmp(argument, L"--loadsaveslot"))
        {
            if (++i >= argc || wcslen(argv[i]) != 1 || argv[i][0] < L'1' || argv[i][0] > L'3')
            {
                snprintf(error, capacity, "--loadsaveslot needs a slot from 1 to 3.");
                return 0;
            }
            options.save_slot = argv[i][0] - L'0';
            requires_debug = 1;
        }
        else
        {
            snprintf(error, capacity, "Unknown argument: %ls. Use --help for options.", argument);
            return 0;
        }
    }
    if (requires_debug && !options.debug && !options.help)
    {
        snprintf(error, capacity, "Level, save and diagnostic arguments require --debug.");
        return 0;
    }
    title_options = options;
    return 1;
}

int title_options_init(void)
{
    int argc;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char error[512];
    if (!argv)
    {
        return -1;
    }
    int ok = title_options_parse(argc, (const wchar_t *const *)argv, error, sizeof error);
    LocalFree(argv);
    if (!ok)
    {
        fprintf(stderr, "[OPTIONS] %s\n", error);
        MessageBoxA(NULL, error, "DogGame: argument error", MB_OK | MB_ICONERROR);
        return -1;
    }
    if (title_options.help)
    {
        MessageBoxA(NULL, usage, "DogGame options", MB_OK);
        return 1;
    }
    if (title_options.level[0])
    {
        char path[MAX_PATH];
        snprintf(path, sizeof path, "game/%c%c/%s.HIP", title_options.level[0] + 32, title_options.level[1],
                 title_options.level);
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
        {
            snprintf(error, sizeof error, "Level %s was not found: %s", title_options.level, path);
            MessageBoxA(NULL, error, "DogGame: level unavailable", MB_OK | MB_ICONERROR);
            return -1;
        }
    }
    if (title_options.debug)
    {
        /* GUI launches have no console; retain diagnostics for manual captures. */
        FILE *log = _fsopen("debug-render.log", "w", _SH_DENYNO);
        if (log)
        {
            if (_fileno(stderr) < 0)
            {
                FILE *stream;
                freopen_s(&stream, "NUL", "w", stderr);
            }
            _dup2(_fileno(log), _fileno(stderr));
            fclose(log);
        }
        setvbuf(stderr, NULL, _IONBF, 0);
        _putenv_s("RECOMP_DEBUG", "1");
        _putenv_s("RECOMP_APU_STATS", "1");
        if (!getenv("RECOMP_KEYBOARD"))
        {
            _putenv_s("RECOMP_KEYBOARD", "1");
        }
        /* Prepared before renderer startup; manual actions do not depend on changing cached getenv pointers. */
        _putenv_s("RECOMP_FRAME_TRACE", "debug-frame.trigger");
        _putenv_s("RECOMP_PROFILE_GPU", "1");
        DeleteFileA("debug-frame.trigger");
    }
    if (title_options.noaudio)
    {
        extern volatile int g_audio_muted;
        g_audio_muted = 1;
    }
    fprintf(stderr, "[OPTIONS] debug=%d nofmv=%d noaudio=%d level=%s save=%d\n", title_options.debug,
            title_options.nofmv, title_options.noaudio, title_options.level, title_options.save_slot);
    title_debug_init();
    return 0;
}
