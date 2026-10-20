#include "title_renderer_settings.h"
#include <windows.h>
#include <wchar.h>

static int renderer_settings_path(wchar_t path[MAX_PATH])
{
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        return 0;
    }
    wchar_t *slash = wcsrchr(path, L'\\');
    if (!slash || (size_t)(slash - path) + 13 >= MAX_PATH)
    {
        return 0;
    }
    wcscpy(slash + 1, L"display.ini");
    return 1;
}

int title_renderer_settings_prefer_vulkan(void)
{
    wchar_t path[MAX_PATH], value[32];
    if (!renderer_settings_path(path))
    {
        return 1;
    }
    GetPrivateProfileStringW(L"Display", L"Renderer", L"Vulkan", value, 32, path);
    return _wcsicmp(value, L"DirectX11") != 0;
}

int title_renderer_settings_save(int vulkan)
{
    wchar_t path[MAX_PATH];
    return renderer_settings_path(path) &&
        WritePrivateProfileStringW(L"Display", L"Renderer", vulkan ? L"Vulkan" : L"DirectX11", path);
}

